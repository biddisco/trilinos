#include "ioUtils.hpp"
#include "stk_io/StkMeshIoBroker.hpp"
#include "stk_mesh/base/BulkData.hpp"   // for BulkData
#include "stk_mesh/base/MetaData.hpp"

namespace stk
{
namespace unit_test_util
{

void fill_mesh_using_stk_io_with_auto_decomp(const std::string &meshSpec, stk::mesh::BulkData &bulkData, stk::ParallelMachine communicator)
{
    stk::io::StkMeshIoBroker exodusFileReader(communicator);
    exodusFileReader.property_add(Ioss::Property("DECOMPOSITION_METHOD", "RCB"));

    exodusFileReader.set_bulk_data(bulkData);
    exodusFileReader.add_mesh_database(meshSpec, stk::io::READ_MESH);
    exodusFileReader.create_input_mesh();
    exodusFileReader.populate_bulk_data();
}

void fill_mesh_using_stk_io(const std::string &meshSpec, stk::mesh::BulkData &bulkData, stk::ParallelMachine communicator)
{
    stk::io::StkMeshIoBroker exodusFileReader(communicator);

    exodusFileReader.set_bulk_data(bulkData);
    exodusFileReader.add_mesh_database(meshSpec, stk::io::READ_MESH);
    exodusFileReader.create_input_mesh();
    exodusFileReader.populate_bulk_data();
}

void write_mesh_using_stk_io(const std::string &filename,
                             stk::mesh::BulkData &bulkData,
                             stk::ParallelMachine communicator,
                             stk::io::DatabasePurpose databasePurpose)
{
    stk::io::StkMeshIoBroker exodusFileWriter(communicator);

    exodusFileWriter.set_bulk_data(bulkData);
    size_t output_file_index = exodusFileWriter.create_output_mesh(filename, databasePurpose);
    exodusFileWriter.write_output_mesh(output_file_index);
}


void generated_mesh_to_file_in_serial(const std::string &meshSizeSpec, const std::string &fileName)
{
    stk::ParallelMachine communicator = MPI_COMM_WORLD;
    const int procId = stk::parallel_machine_rank(communicator);
    if (procId == 0)
    {
        const int spatialDimension = 3;
        stk::mesh::MetaData meta(spatialDimension);
        stk::mesh::BulkData stkMesh(meta,MPI_COMM_SELF,stk::mesh::BulkData::NO_AUTO_AURA);
        stk::io::StkMeshIoBroker broker;
        broker.set_bulk_data(stkMesh);
        broker.add_mesh_database("generated:"+meshSizeSpec, stk::io::READ_MESH);
        broker.create_input_mesh();
        broker.populate_bulk_data();
        size_t output_file_index = broker.create_output_mesh(fileName, stk::io::WRITE_RESULTS);
        broker.write_output_mesh(output_file_index);
    }
}

void read_from_serial_file_and_decompose(const std::string& fileName, stk::mesh::BulkData &mesh, const std::string &decompositionMethod)
{
    stk::io::StkMeshIoBroker broker;
    broker.set_bulk_data(mesh);
    broker.property_add(Ioss::Property("DECOMPOSITION_METHOD", decompositionMethod));
    broker.add_mesh_database(fileName, stk::io::READ_MESH);
    broker.create_input_mesh();
    broker.populate_bulk_data();
    unlink(fileName.c_str());
}

void generate_mesh_from_serial_spec_and_load_in_parallel_with_auto_decomp(const std::string &meshSizeSpec, stk::mesh::BulkData &mesh, const std::string &decompositionMethod)
{
    // meshSizeSpec should NOT include generated:, just "2x2x1" for example.
    // decomposition methods: "linear", "rcb", "rib", "hsfc", "block", "cyclic", "random", "kway", "geom_kway", "metis_sfc"
    const std::string tempFilename = "exodus_" + meshSizeSpec + ".e";
    generated_mesh_to_file_in_serial(meshSizeSpec,tempFilename);

    read_from_serial_file_and_decompose(tempFilename, mesh, decompositionMethod);
}


} // namespace unit_test_util
} // namespace stk

