
#include <gtest/gtest.h>                // for AssertHelper, ASSERT_TRUE, etc
#include <stddef.h>                     // for size_t
#include <algorithm>                    // for find
#include <iostream>                     // for operator<<, ostream, cerr, etc
#include <stk_io/IossBridge.hpp>        // for put_io_part_attribute
#include <stk_mesh/base/BulkData.hpp>   // for BulkData, etc
#include <stk_mesh/base/Comm.hpp>       // for comm_mesh_counts
#include <stk_mesh/base/CreateFaces.hpp>  // for create_faces
#include <stk_mesh/base/ElemElemGraph.hpp>  // for process_killed_elements, etc
#include <stk_mesh/base/GetEntities.hpp>  // for get_selected_entities, etc
#include <stk_mesh/base/MetaData.hpp>   // for MetaData
#include <stk_topology/topology.hpp>    // for topology, etc
#include <stk_unit_test_utils/ioUtils.hpp>  // for fill_mesh_using_stk_io, etc
#include <stk_util/parallel/Parallel.hpp>  // for parallel_machine_size, etc
#include <vector>                       // for vector
#include "UnitTestElementDeathUtils.hpp"  // for deactivate_elements, etc
#include "gtest/gtest-message.h"        // for Message
#include "mpi.h"                        // for ompi_communicator_t, etc
#include "stk_mesh/base/Bucket.hpp"     // for Bucket
#include "stk_mesh/base/BulkDataInlinedMethods.hpp"
#include "stk_mesh/base/Entity.hpp"     // for Entity, operator<<
#include "stk_mesh/base/EntityKey.hpp"  // for EntityKey, operator<<
#include "stk_mesh/base/Part.hpp"       // for Part
#include "stk_mesh/base/Selector.hpp"   // for Selector, etc
#include "stk_mesh/base/Types.hpp"      // for EntityVector, PartVector, etc
#include "stk_unit_test_utils/unittestMeshUtils.hpp"







namespace
{



void test_active_part_membership(stk::mesh::BulkData& bulkData, stk::mesh::EntityVector& skin_faces_of_elem2, stk::mesh::Part& active)
{
    for(stk::topology::rank_t rank = stk::topology::BEGIN_RANK; rank < bulkData.mesh_meta_data().entity_rank_count(); rank++)
    {
        const stk::mesh::BucketVector &buckets = bulkData.get_buckets(rank, bulkData.mesh_meta_data().locally_owned_part());
        for(const stk::mesh::Bucket *bucket : buckets)
        {
            for(stk::mesh::Entity entity : *bucket)
            {
                bool skin_face_of_elem2 = false;
                if(rank == stk::topology::FACE_RANK)
                {
                    stk::mesh::EntityVector::iterator iter = std::find(skin_faces_of_elem2.begin(), skin_faces_of_elem2.end(), entity);
                    if(iter != skin_faces_of_elem2.end())
                    {
                        skin_face_of_elem2 = true;
                    }
                }

                if((rank == stk::topology::ELEM_RANK && bulkData.identifier(entity) == 2) || skin_face_of_elem2)
                {
                    EXPECT_FALSE(bucket->member(active))<< " is entity inactive: " << bulkData.entity_key(entity);
                }
                else
                {
                    EXPECT_TRUE(bucket->member(active)) << " is entity active" << bulkData.entity_key(entity);
                }
            }
        }
    }
}

void test_face_membership_for_death(stk::mesh::BulkData& bulkData, stk::mesh::EntityVector& internal_faces_of_elem2, stk::mesh::PartVector& boundary_mesh_parts)
{
    const stk::mesh::BucketVector &buckets = bulkData.get_buckets(stk::topology::FACE_RANK, bulkData.mesh_meta_data().locally_owned_part());
    for(const stk::mesh::Bucket *bucket : buckets)
    {
        for(stk::mesh::Entity entity : *bucket)
        {
            bool internal_face_of_elem2 = false;
            stk::mesh::EntityVector::iterator iter = std::find(internal_faces_of_elem2.begin(), internal_faces_of_elem2.end(), entity);
            if(iter!=internal_faces_of_elem2.end())
            {
                internal_face_of_elem2 = true;
            }

            if(internal_face_of_elem2)
            {
                EXPECT_TRUE(bucket->member_all(boundary_mesh_parts)) << " is entity in boundary parts: " << bulkData.entity_key(entity);
            }
            else
            {
                EXPECT_FALSE(bucket->member_all(boundary_mesh_parts)) << " is entity not in boundary parts" << bulkData.entity_key(entity);
            }
        }
    }
}

TEST(ElementDeath, replicate_random_death_test)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;
    if(stk::parallel_machine_size(comm) == 4)
    {
        unsigned spatialDim = 3;

        stk::mesh::MetaData meta(spatialDim);
        stk::mesh::Part& faces_part = meta.declare_part_with_topology("surface_5", stk::topology::QUAD_4);
        stk::mesh::Part& death_1_part = meta.declare_part("death_1", stk::topology::FACE_RANK);
        stk::mesh::PartVector boundary_mesh_parts {&faces_part, &death_1_part};
        stk::mesh::BulkData bulkData(meta, comm, stk::mesh::BulkData::NO_AUTO_AURA);

        stk::mesh::Part& active = meta.declare_part("active");
        stk::unit_test_util::generate_mesh_from_serial_spec_and_load_in_parallel_with_auto_decomp("2x2x1", bulkData, "cyclic");

        stk::mesh::create_faces(bulkData);

        std::vector<size_t> mesh_counts;
        stk::mesh::comm_mesh_counts(bulkData, mesh_counts);
        ASSERT_EQ(20u, mesh_counts[stk::topology::FACE_RANK]);
        stk::unit_test_util::put_mesh_into_part(bulkData, active);

        boundary_mesh_parts.push_back(&active);

        stk::mesh::EntityVector elems;
        stk::mesh::get_entities(bulkData, stk::topology::ELEM_RANK, elems);
        ASSERT_EQ(1u, elems.size());
        stk::mesh::EntityId goldId = bulkData.parallel_rank()+1;
        ASSERT_EQ(goldId, bulkData.identifier(elems[0]));

        stk::mesh::EntityVector elements_to_kill;
        if (bulkData.parallel_rank() == 3)
        {
            elements_to_kill.push_back(elems[0]);
        }
        boundary_mesh_parts.push_back(&active);

        stk::mesh::ElemElemGraph graph(bulkData, active);
        ElementDeathUtils::deactivate_elements(elements_to_kill, bulkData,  active);
        EXPECT_NO_THROW(stk::mesh::process_killed_elements(bulkData, graph, elements_to_kill, active, boundary_mesh_parts, &boundary_mesh_parts));

        stk::mesh::Selector sel = death_1_part;
        stk::mesh::EntityVector faces;
        stk::mesh::get_selected_entities(sel, bulkData.buckets(stk::topology::FACE_RANK), faces);

        std::vector<size_t> gold_values = { 0, 1, 1, 2 };
        ASSERT_EQ(gold_values[bulkData.parallel_rank()], faces.size());

        stk::mesh::Entity node14 = bulkData.get_entity(stk::topology::NODE_RANK,14);
        EXPECT_TRUE(bulkData.is_valid(node14));
        EXPECT_TRUE(bulkData.bucket(node14).member(death_1_part));

        elements_to_kill.clear();
        if(bulkData.parallel_rank() == 2)
        {
            elements_to_kill.push_back(elems[0]);
        }

        ElementDeathUtils::deactivate_elements(elements_to_kill, bulkData,  active);
        EXPECT_NO_THROW(stk::mesh::process_killed_elements(bulkData, graph, elements_to_kill, active, boundary_mesh_parts, &boundary_mesh_parts));

        stk::mesh::comm_mesh_counts(bulkData, mesh_counts);
        ASSERT_EQ(20u, mesh_counts[stk::topology::FACE_RANK]);
    }
}

TEST(ElementDeath, keep_faces_after_element_death_after_calling_create_faces)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;

    if(stk::parallel_machine_size(comm) <= 2)
    {
        unsigned spatialDim = 3;

        stk::mesh::MetaData meta(spatialDim);
        stk::mesh::Part& faces_part = meta.declare_part_with_topology("surface_5", stk::topology::QUAD_4);
        stk::mesh::PartVector boundary_mesh_parts {&faces_part};
        stk::io::put_io_part_attribute(faces_part);
        stk::mesh::BulkData bulkData(meta, comm);

        stk::mesh::Part& active = meta.declare_part("active"); // can't specify rank, because it gets checked against size of rank_names

        ASSERT_TRUE(active.primary_entity_rank() == stk::topology::INVALID_RANK);

        stk::unit_test_util::fill_mesh_using_stk_io("generated:1x1x4", bulkData, comm);

        stk::mesh::create_faces(bulkData);

        stk::unit_test_util::put_mesh_into_part(bulkData, active);

        stk::mesh::ElemElemGraph graph(bulkData, active);

        size_t num_gold_edges = 6 / bulkData.parallel_size();
        ASSERT_EQ(num_gold_edges, graph.num_edges());

        stk::mesh::EntityVector deactivated_elems;

        stk::mesh::EntityId elem2Id = 2;
        stk::mesh::EntityId elem3Id = 3;

        stk::mesh::Entity elem2 = bulkData.get_entity(stk::topology::ELEM_RANK, elem2Id);
        stk::mesh::Entity elem3 = bulkData.get_entity(stk::topology::ELEM_RANK, elem3Id);

        std::vector<size_t> entity_counts;
        stk::mesh::comm_mesh_counts(bulkData, entity_counts);
        ASSERT_TRUE(entity_counts[stk::topology::FACE_RANK] == 21);

        {
            stk::mesh::EntityVector skin_faces_of_elem2;
            stk::mesh::EntityVector internal_faces_of_elem2;
            if(bulkData.is_valid(elem2) && bulkData.bucket(elem2).owned())
            {
                deactivated_elems.push_back(elem2);
                unsigned num_faces = bulkData.num_faces(elem2);
                const stk::mesh::Entity* faces = bulkData.begin_faces(elem2);
                for(unsigned j=0;j<num_faces;++j)
                {
                    if(bulkData.num_elements(faces[j])==1)
                    {
                        skin_faces_of_elem2.push_back(faces[j]);
                    }
                    else
                    {
                        internal_faces_of_elem2.push_back(faces[j]);
                    }
                }
            }

            boundary_mesh_parts.push_back(&active);

            ElementDeathUtils::deactivate_elements(deactivated_elems, bulkData,  active);

            stk::mesh::process_killed_elements(bulkData, graph, deactivated_elems, active, boundary_mesh_parts, &boundary_mesh_parts);

            test_active_part_membership(bulkData, skin_faces_of_elem2, active);

            stk::mesh::Entity face_between_elem2_and_elem3 = ElementDeathUtils::get_face_between_element_ids(graph, bulkData, elem2Id, elem3Id);
            ASSERT_TRUE(bulkData.is_valid(face_between_elem2_and_elem3));

            test_face_membership_for_death(bulkData, internal_faces_of_elem2, boundary_mesh_parts);
        }

        stk::mesh::comm_mesh_counts(bulkData, entity_counts);

        ASSERT_TRUE(entity_counts[stk::topology::FACE_RANK] == 21);

        // now kill 3

        {
            deactivated_elems.clear();

            stk::mesh::EntityVector skin_faces_of_elem3;
            stk::mesh::EntityVector internal_faces_of_elem3;
            if(bulkData.is_valid(elem3) && bulkData.bucket(elem3).owned())
            {
                deactivated_elems.push_back(elem3);
                unsigned num_faces = bulkData.num_faces(elem3);
                const stk::mesh::Entity* faces = bulkData.begin_faces(elem3);
                for(unsigned j=0;j<num_faces;++j)
                {
                    if(bulkData.num_elements(faces[j])==1)
                    {
                        skin_faces_of_elem3.push_back(faces[j]);
                    }
                    else
                    {
                        internal_faces_of_elem3.push_back(faces[j]);
                    }
                }
            }

            boundary_mesh_parts.push_back(&active);

            ElementDeathUtils::deactivate_elements(deactivated_elems, bulkData,  active);

            stk::mesh::EntityId face_id;

            stk::mesh::process_killed_elements(bulkData, graph, deactivated_elems, active, boundary_mesh_parts, &boundary_mesh_parts);

            stk::mesh::Entity face_between_elem2_and_elem3 = ElementDeathUtils::get_face_between_element_ids(graph, bulkData, elem2Id, elem3Id);
            EXPECT_TRUE(bulkData.is_valid(face_between_elem2_and_elem3));
            face_id = bulkData.identifier(face_between_elem2_and_elem3);
            ASSERT_FALSE(bulkData.bucket(face_between_elem2_and_elem3).member(active));

            stk::mesh::Entity face_23 = bulkData.get_entity(stk::topology::FACE_RANK, face_id);
            EXPECT_TRUE(bulkData.is_valid(face_23));
        }

        stk::mesh::comm_mesh_counts(bulkData, entity_counts);
        ASSERT_TRUE(entity_counts[stk::topology::FACE_RANK] == 21);
    }
}

TEST(ElementDeath, keep_faces_after_element_death_without_calling_create_faces)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;

    if(stk::parallel_machine_size(comm) <= 2)
    {
        unsigned spatialDim = 3;

        stk::mesh::MetaData meta(spatialDim);
        stk::mesh::Part& faces_part = meta.declare_part_with_topology("surface_5", stk::topology::QUAD_4);
        stk::mesh::PartVector boundary_mesh_parts {&faces_part};
        stk::io::put_io_part_attribute(faces_part);
        stk::mesh::BulkData bulkData(meta, comm);

        stk::mesh::Part& active = meta.declare_part("active"); // can't specify rank, because it gets checked against size of rank_names

        ASSERT_TRUE(active.primary_entity_rank() == stk::topology::INVALID_RANK);

        stk::unit_test_util::fill_mesh_using_stk_io("generated:1x1x4", bulkData, comm);

        stk::unit_test_util::put_mesh_into_part(bulkData, active);

        stk::mesh::ElemElemGraph graph(bulkData, active);

        size_t num_gold_edges = 6 / bulkData.parallel_size();
        ASSERT_EQ(num_gold_edges, graph.num_edges());

        stk::mesh::EntityVector deactivated_elems;

        stk::mesh::EntityId elem2Id = 2;
        stk::mesh::EntityId elem3Id = 3;

        stk::mesh::Entity elem2 = bulkData.get_entity(stk::topology::ELEM_RANK, elem2Id);
        stk::mesh::Entity elem3 = bulkData.get_entity(stk::topology::ELEM_RANK, elem3Id);

        std::vector<size_t> entity_counts;
        stk::mesh::comm_mesh_counts(bulkData, entity_counts);
        ASSERT_TRUE(entity_counts[stk::topology::FACE_RANK] == 0);

        {
            stk::mesh::EntityVector skin_faces_of_elem2;
            stk::mesh::EntityVector internal_faces_of_elem2;
            if(bulkData.is_valid(elem2) && bulkData.bucket(elem2).owned())
            {
                deactivated_elems.push_back(elem2);
                unsigned num_faces = bulkData.num_faces(elem2);
                const stk::mesh::Entity* faces = bulkData.begin_faces(elem2);
                for(unsigned j=0;j<num_faces;++j)
                {
                    if(bulkData.num_elements(faces[j])==1)
                    {
                        skin_faces_of_elem2.push_back(faces[j]);
                    }
                    else
                    {
                        internal_faces_of_elem2.push_back(faces[j]);
                    }
                }
            }

            boundary_mesh_parts.push_back(&active);

            ElementDeathUtils::deactivate_elements(deactivated_elems, bulkData,  active);

            test_active_part_membership(bulkData, skin_faces_of_elem2, active);

            stk::mesh::process_killed_elements(bulkData, graph, deactivated_elems, active, boundary_mesh_parts, &boundary_mesh_parts);

            stk::mesh::Entity face_between_elem2_and_elem3 = ElementDeathUtils::get_face_between_element_ids(graph, bulkData, elem2Id, elem3Id);

            ASSERT_TRUE(bulkData.is_valid(face_between_elem2_and_elem3));
        }

        stk::mesh::comm_mesh_counts(bulkData, entity_counts);

        ASSERT_TRUE(entity_counts[stk::topology::FACE_RANK] == 2);

        // now kill 3

        {
            deactivated_elems.clear();

            stk::mesh::EntityVector skin_faces_of_elem3;
            stk::mesh::EntityVector internal_faces_of_elem3;
            if(bulkData.is_valid(elem3) && bulkData.bucket(elem3).owned())
            {
                deactivated_elems.push_back(elem3);
                unsigned num_faces = bulkData.num_faces(elem3);
                const stk::mesh::Entity* faces = bulkData.begin_faces(elem3);
                for(unsigned j=0;j<num_faces;++j)
                {
                    if(bulkData.num_elements(faces[j])==1)
                    {
                        skin_faces_of_elem3.push_back(faces[j]);
                    }
                    else
                    {
                        internal_faces_of_elem3.push_back(faces[j]);
                    }
                }
            }

            boundary_mesh_parts.push_back(&active);

            ElementDeathUtils::deactivate_elements(deactivated_elems, bulkData,  active);

            stk::mesh::Entity face_between_elem2_and_elem3 = ElementDeathUtils::get_face_between_element_ids(graph, bulkData, elem2Id, elem3Id);

            stk::mesh::process_killed_elements(bulkData, graph, deactivated_elems, active, boundary_mesh_parts, &boundary_mesh_parts);

            EXPECT_FALSE(bulkData.is_valid(face_between_elem2_and_elem3));
        }

        stk::mesh::comm_mesh_counts(bulkData, entity_counts);
        ASSERT_TRUE(entity_counts[stk::topology::FACE_RANK] == 2);
    }
}

class BulkDataTester : public stk::mesh::BulkData
{
public:
    BulkDataTester(stk::mesh::MetaData &mesh_meta_data, MPI_Comm comm) :
            stk::mesh::BulkData(mesh_meta_data, comm)
    {
    }

    virtual ~BulkDataTester()
    {
    }

    void set_sorting_by_face()
    {
        m_shouldSortFacesByNodeIds = true;
    }
};


void kill_element(stk::mesh::Entity element, stk::mesh::BulkData& bulkData, stk::mesh::Part& active, stk::mesh::Part& skin)
{
    stk::mesh::ElemElemGraph graph(bulkData, active);
    stk::mesh::EntityVector deactivated_elems;
    if(bulkData.is_valid(element) && bulkData.parallel_owner_rank(element) == bulkData.parallel_rank())
    {
        deactivated_elems.push_back(element);
    }

    stk::mesh::PartVector boundary_mesh_parts={&active, &skin};
    ElementDeathUtils::deactivate_elements(deactivated_elems, bulkData,  active);
    stk::mesh::process_killed_elements(bulkData, graph, deactivated_elems, active, boundary_mesh_parts, &boundary_mesh_parts);
}

stk::mesh::EntityVector get_entities(stk::mesh::BulkData& bulkData, const stk::mesh::ConstPartVector& parts)
{
    stk::mesh::EntityVector entities;
    stk::mesh::Selector sel = stk::mesh::selectIntersection(parts);
    stk::mesh::get_selected_entities(sel, bulkData.buckets(stk::topology::FACE_RANK), entities);
    return entities;
}

void compare_faces(const stk::mesh::BulkData& bulkData, const std::vector<size_t> &num_gold_skinned_faces, const stk::mesh::EntityVector& skinned_faces, const stk::mesh::EntityVector &active_faces)
{
    EXPECT_EQ(num_gold_skinned_faces[bulkData.parallel_rank()], skinned_faces.size());
    EXPECT_EQ(num_gold_skinned_faces[bulkData.parallel_rank()], active_faces.size());

    for(size_t i=0;i<skinned_faces.size();++i)
    {
        if (bulkData.identifier(skinned_faces[i]) != bulkData.identifier(active_faces[i]))
        {
            std::cerr << "Skinned faces: ";
            for(size_t j=0;j<skinned_faces.size();++j)
            {
                std::cerr << skinned_faces[j] << "\t";
            }
            std::cerr << std::endl;

            std::cerr << "active faces: ";
            for(size_t j=0;j<active_faces.size();++j)
            {
                std::cerr << active_faces[j] << "\t";
            }
            std::cerr << std::endl;
            break;
        }
    }
}

void compare_skin(const std::vector<size_t>& num_gold_skinned_faces, stk::mesh::BulkData& bulkData, const stk::mesh::Part& skin, const stk::mesh::Part& active)
{
    stk::mesh::EntityVector skinned_faces = get_entities(bulkData, {&skin, &active});
    stk::mesh::EntityVector active_faces  = get_entities(bulkData, {&active} );
    compare_faces(bulkData, num_gold_skinned_faces, skinned_faces, active_faces);
}

TEST(ElementDeath, compare_death_and_skin_mesh)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;

     if(stk::parallel_machine_size(comm) == 1)
     {
         unsigned spatialDim = 3;

         stk::mesh::MetaData meta(spatialDim);
         stk::mesh::Part& skin  = meta.declare_part_with_topology("skin", stk::topology::QUAD_4);
         stk::io::put_io_part_attribute(skin);
         BulkDataTester bulkData(meta, comm);
         bulkData.set_sorting_by_face();

         stk::mesh::Part& active = meta.declare_part("active"); // can't specify rank, because it gets checked against size of rank_names
         stk::unit_test_util::fill_mesh_using_stk_io("generated:1x1x4", bulkData, comm);
         stk::unit_test_util::put_mesh_into_part(bulkData, active);

         ElementDeathUtils::skin_boundary(bulkData, active, {&skin, &active});

         std::vector<size_t> num_gold_skinned_faces = { 18 };
         compare_skin(num_gold_skinned_faces, bulkData, skin, active);

         stk::mesh::Entity element1 = bulkData.get_entity(stk::topology::ELEM_RANK, 1);
         kill_element(element1, bulkData, active, skin);

         ElementDeathUtils::skin_part(bulkData, active, {&skin, &active});

         num_gold_skinned_faces[0] = 14;
         compare_skin(num_gold_skinned_faces, bulkData, skin, active);
     }
}


} // end namespace


