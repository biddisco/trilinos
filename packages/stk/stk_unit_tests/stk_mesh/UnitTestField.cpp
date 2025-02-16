// Copyright (c) 2013, Sandia Corporation.
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of Sandia Corporation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#include <iostream>                     // for ostream, operator<<, etc
#include <stdexcept>                    // for runtime_error
#include <stk_mesh/base/BulkData.hpp>   // for BulkData
#include <stk_mesh/base/CoordinateSystems.hpp>  // for Cartesian, etc
#include <stk_mesh/base/GetEntities.hpp>  // for count_selected_entities
#include <stk_mesh/base/MetaData.hpp>   // for MetaData, put_field
#include <stk_util/parallel/Parallel.hpp>  // for ParallelMachine
#include <gtest/gtest.h>
#include <string>                       // for operator==, string, etc
#include <vector>                       // for vector
#include "Shards_Array.hpp"
#include "stk_mesh/base/Field.hpp"      // for Field
#include "stk_mesh/base/FieldBase.hpp"  // for field_bytes_per_entity, etc
#include "stk_mesh/base/Part.hpp"       // for Part
#include "stk_mesh/base/Selector.hpp"   // for operator<<, Selector, etc
#include "stk_mesh/base/Types.hpp"      // for PartVector, BucketVector, etc
#include "stk_topology/topology.hpp"    // for topology, etc
#include <stk_io/StkMeshIoBroker.hpp>   // for StkMeshIoBroker
#include "Ioss_DBUsage.h"               // for DatabaseUsage::READ_MODEL
#include "Ioss_Field.h"                 // for Field, etc
#include "Ioss_IOFactory.h"             // for IOFactory
#include "Ioss_NodeBlock.h"             // for NodeBlock
#include "Ioss_ElementBlock.h"
#include "Ioss_Property.h"              // for Property
#include "Ioss_Region.h"                // for Region, NodeBlockContainer
#include "stk_io/DatabasePurpose.hpp"   // for DatabasePurpose::READ_MESH, etc

namespace stk { namespace mesh { class Bucket; } }


using stk::mesh::MetaData;

namespace {

const stk::topology::rank_t NODE_RANK = stk::topology::NODE_RANK;

typedef shards::ArrayDimTag::size_type size_type;

SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( ATAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( BTAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( CTAG )

TEST(UnitTestField, testCartesian)
{
  // Test the Cartesian array dimension tag

  const stk::mesh::Cartesian& cartesian_tag = stk::mesh::Cartesian::tag();

  std::string to_str = cartesian_tag.to_string(3 /*size*/, 1 /*idx*/);
  std::string expected_str("y");
  ASSERT_EQ( (to_str == expected_str), true);

  //should throw if we supply a size < 3:
  ASSERT_THROW( cartesian_tag.to_string(2 /*size*/, 1 /*idx*/),
                        std::runtime_error );

  size_type expected_idx = 1;
  size_type idx = cartesian_tag.to_index(3 /*size*/, "y" /*dim*/);
  ASSERT_EQ( idx, expected_idx );

  //should throw if we supply a "z" along with size==2:
  ASSERT_THROW( cartesian_tag.to_index(2 /*size*/, "z" /*dim*/),
                        std::runtime_error );
}

TEST(UnitTestField, testCylindrical)
{
  // Test the Cylindrical array dimension tag

  const stk::mesh::Cylindrical& cylindrical_tag =stk::mesh::Cylindrical::tag();

  std::string to_str = cylindrical_tag.to_string(3 /*size*/, 1 /*idx*/);
  std::string expected_str("a");
  ASSERT_EQ( (to_str == expected_str), true );

  //should throw if we supply a size < 3:
  ASSERT_THROW( cylindrical_tag.to_string(2 /*size*/, 1 /*idx*/),
                        std::runtime_error );

  size_type expected_idx = 1;
  size_type idx = cylindrical_tag.to_index(3 /*size*/, "a" /*dim*/);
  ASSERT_EQ( idx, expected_idx );

  //should throw if we supply a "z" along with size==2:
  ASSERT_THROW( cylindrical_tag.to_index(2 /*size*/, "z" /*dim*/),
                        std::runtime_error );
}

TEST(UnitTestField, testFullTensor)
{
  // Test the FullTensor array dimension tag

  const stk::mesh::FullTensor&  fulltensor_tag = stk::mesh::FullTensor::tag();

  std::string to_str = fulltensor_tag.to_string(9 /*size*/, 1 /*idx*/);
  std::string expected_str("yy");
  ASSERT_EQ( (to_str == expected_str), true );

  //should throw if we supply a size < 9:
  ASSERT_THROW( fulltensor_tag.to_string(2 /*size*/, 1 /*idx*/),
                        std::runtime_error );

  size_type expected_idx = 6;
  size_type idx = fulltensor_tag.to_index(9 /*size*/, "yx" /*dim*/);
  ASSERT_EQ( idx, expected_idx );

  //should throw if we supply a "zz" along with size==2:
  ASSERT_THROW( fulltensor_tag.to_index(2 /*size*/, "zz" /*dim*/),
                        std::runtime_error );
}

TEST(UnitTestField, testSymmetricTensor)
{
  // Test the SymmetricTensor array dimension tag

  const stk::mesh::SymmetricTensor& symmetrictensor_tag =
    stk::mesh::SymmetricTensor::tag();

  std::string to_str = symmetrictensor_tag.to_string(9 /*size*/, 1 /*idx*/);
  std::string expected_str("yy");
  ASSERT_EQ( (to_str == expected_str), true);

  //should throw if we supply a size < 9:
  ASSERT_THROW( symmetrictensor_tag.to_string(2 /*size*/, 1 /*idx*/),
                        std::runtime_error );

  size_type expected_idx = 1;
  size_type idx = symmetrictensor_tag.to_index(6 /*size*/, "yy" /*dim*/);
  ASSERT_EQ( idx, expected_idx );

  //should throw if we supply a "xz" along with size==5:
  ASSERT_THROW( symmetrictensor_tag.to_index(5 /*size*/, "xz" /*dim*/),
                        std::runtime_error );
}

TEST(UnitTestField, testFieldMaxSize)
{
  stk::ParallelMachine pm = MPI_COMM_SELF ;
  std::ostringstream oss; // to test printing of things w/out spamming cout

  // specifications for some test fields
  typedef stk::mesh::Field<double>                rank_zero_field;
  typedef stk::mesh::Field<double,ATAG>           rank_one_field;
  typedef stk::mesh::Field<double,ATAG,BTAG>      rank_two_field;
  typedef stk::mesh::Field<double,ATAG,BTAG,CTAG> rank_three_field;

  const std::string name0("test_field_0");
  const std::string name1("test_field_1");
  const std::string name2("test_field_2");
  const std::string name3("test_field_3");

  const int spatial_dimension = 3;
  stk::mesh::MetaData meta_data( spatial_dimension );
  stk::mesh::BulkData bulk_data( meta_data , pm );

  rank_zero_field  & f0 = meta_data.declare_field< rank_zero_field >( NODE_RANK, name0 );
  rank_one_field   & f1 = meta_data.declare_field< rank_one_field >(  NODE_RANK, name1 );
  rank_two_field   & f2 = meta_data.declare_field< rank_two_field >(  NODE_RANK, name2 );
  rank_three_field & f3 = meta_data.declare_field< rank_three_field >( NODE_RANK, name3 );

  stk::mesh::Part & p0 = meta_data.declare_part("P0", NODE_RANK );
  stk::mesh::Part & p1 = meta_data.declare_part("P1", NODE_RANK );
  stk::mesh::Part & p2 = meta_data.declare_part("P2", NODE_RANK );
  stk::mesh::Part & p3 = meta_data.declare_part("P3", NODE_RANK );

  stk::mesh::put_field( f0 , p0 );
  stk::mesh::put_field( f1 , p1 , 10 );
  stk::mesh::put_field( f2 , p2 , 10 , 20 );
  stk::mesh::put_field( f3 , p3 , 10 , 20 , 30 );

  meta_data.commit();

  // SCALAR FIELDS:
  EXPECT_EQ( f0.max_size(stk::topology::NODE_RANK), 1u );
  EXPECT_EQ( f0.max_size(stk::topology::EDGE_RANK), 0u );
  EXPECT_EQ( f0.max_size(stk::topology::FACE_RANK), 0u );
  EXPECT_EQ( f0.max_size(stk::topology::ELEMENT_RANK), 0u );

  EXPECT_EQ( f1.max_size(stk::topology::NODE_RANK), 10u );
  EXPECT_EQ( f1.max_size(stk::topology::EDGE_RANK), 0u );
  EXPECT_EQ( f1.max_size(stk::topology::FACE_RANK), 0u );
  EXPECT_EQ( f1.max_size(stk::topology::ELEMENT_RANK), 0u );

  EXPECT_EQ( f2.max_size(stk::topology::NODE_RANK), 200u );
  EXPECT_EQ( f2.max_size(stk::topology::EDGE_RANK), 0u );
  EXPECT_EQ( f2.max_size(stk::topology::FACE_RANK), 0u );
  EXPECT_EQ( f2.max_size(stk::topology::ELEMENT_RANK), 0u );

  EXPECT_EQ( f3.max_size(stk::topology::NODE_RANK), 6000u );
  EXPECT_EQ( f3.max_size(stk::topology::EDGE_RANK), 0u );
  EXPECT_EQ( f3.max_size(stk::topology::FACE_RANK), 0u );
  EXPECT_EQ( f3.max_size(stk::topology::ELEMENT_RANK), 0u );

  EXPECT_EQ( f0.field_array_rank(), 0u ); // Field Rank NOT entity rank
  EXPECT_EQ( f1.field_array_rank(), 1u );
  EXPECT_EQ( f2.field_array_rank(), 2u );
  EXPECT_EQ( f3.field_array_rank(), 3u );

}

TEST(UnitTestField, testFieldWithSelector)
{
  stk::ParallelMachine pm = MPI_COMM_SELF ;
  std::ostringstream oss; // to test printing of things w/out spamming cout

  // specifications for test field
  typedef stk::mesh::Field<double>    rank_zero_field ;

  const std::string name0("test_field_0");

  const int spatial_dimension = 3;
  stk::mesh::MetaData meta_data( spatial_dimension );
  stk::mesh::BulkData bulk_data( meta_data , pm );

  rank_zero_field  & f0 = meta_data.declare_field< rank_zero_field >( NODE_RANK, name0 );

  stk::mesh::Part & p0 = meta_data.declare_part("P0", NODE_RANK );
  stk::mesh::Part & p1 = meta_data.declare_part("P1", NODE_RANK );

  stk::mesh::Selector select_p0 = p0;
  std::cout <<"select_p0: "<< select_p0 << std::endl;

  stk::mesh::put_field( f0 , select_p0 );

  stk::mesh::print( oss , "  " , f0 );

  meta_data.commit();

  bulk_data.modification_begin();

  // Declare 10 nodes on each part

  for ( unsigned i = 1 ; i < 11 ; ++i ) {
    bulk_data.declare_entity( NODE_RANK , i ,
                              std::vector< stk::mesh::Part * >( 1 , & p0 ) );
  }

  for ( unsigned i = 11 ; i < 21 ; ++i ) {
    bulk_data.declare_entity( NODE_RANK , i ,
                              std::vector< stk::mesh::Part * >( 1 , & p1 ) );
  }

  const stk::mesh::BucketVector & node_buckets =
    bulk_data.buckets( NODE_RANK );

  unsigned num = stk::mesh::count_selected_entities(select_p0, node_buckets);

  ASSERT_EQ( 10u, num );

  stk::mesh::Selector select_f0 = stk::mesh::selectField(f0);

  std::cout <<"select_f0: "<< select_f0 << std::endl;

  unsigned num_f0 = stk::mesh::count_selected_entities(select_f0, node_buckets);
  ASSERT_EQ(10u, num_f0);

  stk::mesh::BucketVector const& f0_buckets = bulk_data.get_buckets(NODE_RANK, select_p0);
  unsigned num_buckets = f0_buckets.size();
  ASSERT_EQ(1u, num_buckets);

  for(stk::mesh::Bucket* b : f0_buckets) {
    unsigned f0_size = field_bytes_per_entity(f0, *b);
    ASSERT_EQ(8u, f0_size);
  }
}

TEST(UnitTestField, testFieldWithSelectorAnd)
{
  stk::ParallelMachine pm = MPI_COMM_SELF ;
  std::ostringstream oss; // to test printing of things w/out spamming cout

  typedef stk::mesh::Field<double,shards::ArrayDimension>           rank_one_field ;
  // specifications for test field

  const std::string name0("test_field_0");

  const int spatial_dimension = 3;
  stk::mesh::MetaData meta_data( spatial_dimension );
  stk::mesh::BulkData bulk_data( meta_data , pm );

  rank_one_field  & f0 = meta_data.declare_field< rank_one_field >( stk::topology::ELEMENT_RANK, name0 );

  stk::mesh::EntityRank elem_rank = stk::topology::ELEMENT_RANK;
  stk::mesh::Part & elements = meta_data.declare_part("Elements", elem_rank);
  stk::mesh::Part & hex8s = meta_data.declare_part("Hex8", elem_rank );
  stk::mesh::Part & tet4s = meta_data.declare_part("Tet4", elem_rank );

  stk::mesh::Selector elem_hex_selector = elements & hex8s;
  stk::mesh::Selector elem_tet_selector = elements & tet4s;
  std::cout <<"elem_hex_selector: "<< elem_hex_selector << std::endl;
  std::cout <<"elem_tet_selector: "<< elem_tet_selector << std::endl;

  stk::mesh::put_field( f0 , elem_hex_selector, 8u );
  stk::mesh::put_field( f0 , elem_tet_selector, 4u );

  stk::mesh::print( oss , "  " , f0 );

  meta_data.commit();

  bulk_data.modification_begin();

  // Declare 10 elements on each part

  stk::mesh::PartVector parts;
  parts.push_back(&elements);
  parts.push_back(&hex8s);

  for ( unsigned i = 1 ; i < 11 ; ++i ) {
    bulk_data.declare_entity( elem_rank , i , parts );
  }

  parts.clear();
  parts.push_back(&elements);
  parts.push_back(&tet4s);

  for ( unsigned i = 11 ; i < 21 ; ++i ) {
    bulk_data.declare_entity( elem_rank , i , parts );
  }

  {
    stk::mesh::BucketVector const& f0_buckets = bulk_data.get_buckets(elem_rank, elem_hex_selector);

    for(stk::mesh::Bucket* b : f0_buckets) {
      unsigned f0_size = field_bytes_per_entity(f0, *b);
      ASSERT_EQ(64u, f0_size);
    }
  }

  {
    stk::mesh::BucketVector const& f0_buckets = bulk_data.get_buckets(elem_rank, elem_tet_selector);

    for(stk::mesh::Bucket* b : f0_buckets) {
      unsigned f0_size = field_bytes_per_entity(f0, *b);
      ASSERT_EQ(32u, f0_size);
    }
  }
}


TEST(UnitTestField, testFieldWithSelectorInvalid)
{
  stk::ParallelMachine pm = MPI_COMM_SELF ;
  std::ostringstream oss; // to test printing of things w/out spamming cout

  typedef stk::mesh::Field<double,shards::ArrayDimension>           rank_one_field ;
  // specifications for test field

  const std::string name0("test_field_0");

  const int spatial_dimension = 3;
  stk::mesh::MetaData meta_data( spatial_dimension );
  stk::mesh::BulkData bulk_data( meta_data , pm );

  rank_one_field  & f0 = meta_data.declare_field< rank_one_field >( stk::topology::ELEMENT_RANK, name0 );

  stk::mesh::EntityRank elem_rank = stk::topology::ELEMENT_RANK;
  stk::mesh::Part & hex8s = meta_data.declare_part("Hex8", elem_rank );

  stk::mesh::Part & universal_part = meta_data.universal_part();
  stk::mesh::Selector elem_hexA_selector = hex8s;
  stk::mesh::Selector elem_hexB_selector = universal_part & hex8s;

  std::cout <<"elem_hexA_selector: "<< elem_hexA_selector << std::endl;
  std::cout <<"elem_hexB_selector: "<< elem_hexB_selector << std::endl;

  stk::mesh::put_field( f0 , elem_hexA_selector, 8u );
  ASSERT_THROW(
    stk::mesh::put_field( f0 , elem_hexA_selector, 4u ),
    std::runtime_error
  );
  stk::mesh::put_field( f0 , elem_hexB_selector, 4u );

  stk::mesh::print( oss , "  " , f0 );

  meta_data.commit();

  bulk_data.modification_begin();

  stk::mesh::PartVector parts;
  parts.push_back(&hex8s);
  ASSERT_THROW(
    bulk_data.declare_entity( elem_rank , 1 , parts ),
    std::runtime_error
  );

}

SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( ATAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( BTAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( CTAG )


TEST(UnitTestField, writeFieldsWithSameName)
{
    std::string mesh_name = "mesh_fields_with_same_name.e";
    MPI_Comm communicator = MPI_COMM_WORLD;
    const std::string fieldName = "MyFieldForElementsAndNodes";
    const double nodeInitialValue = 1.25;
    const double elemInitialValue = 8.39;
    const double time = 1.0;

    // Create the mesh with fields with the same name
    {
        stk::io::StkMeshIoBroker stkIo(communicator);

        const std::string generatedFileName = "generated:4x4x16";
        size_t index = stkIo.add_mesh_database(generatedFileName, stk::io::READ_MESH);
        stkIo.set_active_mesh(index);
        stkIo.create_input_mesh();

        stk::mesh::Field<double> &nodeField = stkIo.meta_data().declare_field<stk::mesh::Field<double> >(stk::topology::NODE_RANK, fieldName, 1);
        stk::mesh::put_field(nodeField, stkIo.meta_data().universal_part(), &nodeInitialValue);

        stk::mesh::Field<double> &elemField = stkIo.meta_data().declare_field<stk::mesh::Field<double> >(stk::topology::ELEMENT_RANK, fieldName, 1);
        stk::mesh::put_field(elemField, stkIo.meta_data().universal_part(), &elemInitialValue);

        stkIo.populate_bulk_data();

        size_t fh = stkIo.create_output_mesh(mesh_name, stk::io::WRITE_RESULTS);
        stkIo.write_output_mesh(fh);
        stkIo.add_field(fh, nodeField);
        stkIo.add_field(fh, elemField);
        stkIo.begin_output_step(fh, time);
        stkIo.write_defined_output_fields(fh);
        stkIo.end_output_step(fh);
    }

    // Verify that the fields were written out to disk properly
    {
        Ioss::DatabaseIO *resultsDb = Ioss::IOFactory::create("exodus", mesh_name, Ioss::READ_MODEL, communicator);
        Ioss::Region results(resultsDb);
        const int goldNumSteps = 1;
        EXPECT_EQ(goldNumSteps, results.get_property("state_count").get_int());
        // Should be 1 nodal field on database named "disp";
        Ioss::NodeBlock *nb = results.get_node_blocks()[0];
        const unsigned goldNumNodeFields = 1;
        EXPECT_EQ(goldNumNodeFields, nb->field_count(Ioss::Field::TRANSIENT));
        EXPECT_TRUE(nb->field_exists(fieldName));

        Ioss::ElementBlock *eb = results.get_element_blocks()[0];
        const unsigned goldNumElemFields = 1;
        EXPECT_EQ(goldNumElemFields, eb->field_count(Ioss::Field::TRANSIENT));
        EXPECT_TRUE(eb->field_exists(fieldName));

        const int step = 1;
        double db_time = results.begin_state(step);
        EXPECT_EQ(time, db_time);

        std::vector<double> nodeFieldData;
        nb->get_field_data(fieldName, nodeFieldData);
        for (size_t node = 0; node < nodeFieldData.size(); node++) {
            EXPECT_EQ(nodeInitialValue, nodeFieldData[node]);
        }

        std::vector<double> elemFieldData;
        eb->get_field_data(fieldName, elemFieldData);
        for (size_t elem = 0; elem < elemFieldData.size(); elem++) {
            EXPECT_EQ(elemInitialValue, elemFieldData[elem]);
        }

        results.end_state(step);
    }

    // Verify that we can read the mesh back into memory correctly
    {
        stk::io::StkMeshIoBroker stkIo(communicator);

        size_t index = stkIo.add_mesh_database(mesh_name, stk::io::READ_MESH);
        stkIo.set_active_mesh(index);
        stkIo.create_input_mesh();
        const double badInitialData = -1.2345;

        stk::mesh::Field<double> &nodeField = stkIo.meta_data().declare_field<stk::mesh::Field<double> >(stk::topology::NODE_RANK, fieldName, 1);
        stk::mesh::put_field(nodeField, stkIo.meta_data().universal_part(), &badInitialData);

        stk::mesh::Field<double> &elemField = stkIo.meta_data().declare_field<stk::mesh::Field<double> >(stk::topology::ELEMENT_RANK, fieldName, 1);
        stk::mesh::put_field(elemField, stkIo.meta_data().universal_part(), &badInitialData);

        stkIo.populate_bulk_data();
        stkIo.add_input_field(stk::io::MeshField(nodeField, fieldName));
        stkIo.add_input_field(stk::io::MeshField(elemField, fieldName));
        stkIo.read_defined_input_fields(time);
        stk::mesh::BulkData &mesh = stkIo.bulk_data();
        stk::mesh::MetaData &metaData = stkIo.meta_data();

        const stk::mesh::BucketVector &nodeBuckets = mesh.get_buckets(stk::topology::NODE_RANK, metaData.locally_owned_part());
        for (size_t bucket_i=0 ; bucket_i<nodeBuckets.size() ; ++bucket_i) {
            stk::mesh::Bucket &nodeBucket = *nodeBuckets[bucket_i];
            for (size_t node_i=0 ; node_i<nodeBucket.size() ; ++node_i) {
                double * nodeData = field_data(nodeField,nodeBucket.bucket_id(),node_i);
                EXPECT_EQ(nodeInitialValue, *nodeData);
            }
        }

        const stk::mesh::BucketVector &elemBuckets = mesh.get_buckets(stk::topology::ELEM_RANK, metaData.locally_owned_part());
        for (size_t bucket_i=0 ; bucket_i<elemBuckets.size() ; ++bucket_i) {
            stk::mesh::Bucket &elemBucket = *elemBuckets[bucket_i];
            for (size_t elem_i=0 ; elem_i<elemBucket.size() ; ++elem_i) {
                double * elemData = field_data(elemField,elemBucket.bucket_id(),elem_i);
                EXPECT_EQ(elemInitialValue, *elemData);
            }
        }

        // Test Field accessor functions:
        stk::mesh::Field<double> *myTemplatedField = stk::mesh::get_field_by_name<stk::mesh::Field<double> >(fieldName, metaData);
        ASSERT_TRUE(myTemplatedField != NULL);
        EXPECT_TRUE( &nodeField == myTemplatedField);
        stk::mesh::FieldBase *myFieldBase = stk::mesh::get_field_by_name(fieldName, metaData);
        ASSERT_TRUE(myFieldBase != NULL);
        EXPECT_TRUE( &nodeField == myFieldBase);
    }

    unlink(mesh_name.c_str());
}



} //namespace <anonymous>

