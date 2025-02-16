// @HEADER
//
// ***********************************************************************
//
//   Zoltan2: A package of combinatorial algorithms for scientific computing
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Karen Devine      (kddevin@sandia.gov)
//                    Erik Boman        (egboman@sandia.gov)
//                    Siva Rajamanickam (srajama@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
//
// Basic test of the XpetraTraits definitions.
//
// TODO - a real test would figure out if the migrated objects are
// the same as the original, here we just look at them on stdout.
// TODO look at number of diagonals and max number of entries in
//   Tpetra and Xpetra migrated graphs.  They're garbage.

#include <Zoltan2_XpetraTraits.hpp>
#include <Zoltan2_TestHelpers.hpp>

#include <string>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_Array.hpp>
#include <Teuchos_ArrayRCP.hpp>
#include <Teuchos_Comm.hpp>
#include <Teuchos_VerboseObject.hpp>
#include <Tpetra_CrsMatrix.hpp>
#include <Tpetra_Vector.hpp>

#ifdef HAVE_ZOLTAN_EPETRA
#include <Xpetra_EpetraUtils.hpp>
#endif

using namespace std;
using std::string;
using Teuchos::RCP;
using Teuchos::ArrayRCP;
using Teuchos::ArrayView;
using Teuchos::Array;
using Teuchos::rcp;
using Teuchos::Comm;

ArrayRCP<zgno_t> roundRobinMap(
    const RCP<const Xpetra::Map<zlno_t, zgno_t, znode_t> > &m)
{
  const RCP<const Comm<int> > &comm = m->getComm();
  int proc = comm->getRank();
  int nprocs = comm->getSize();
  zgno_t base = m->getMinAllGlobalIndex();
  zgno_t max = m->getMaxAllGlobalIndex();
  size_t globalrows = m->getGlobalNumElements();
  if (globalrows != size_t(max - base + 1)){
    TEST_FAIL_AND_EXIT(*comm, 0,
      string("Map is invalid for test - fix test"), 1);
  }
  RCP<Array<zgno_t> > mygids = rcp(new Array<zgno_t>);
  zgno_t firstzgno_t = proc;
  if (firstzgno_t < base){
    zgno_t n = base % proc;
    if (n>0)
      firstzgno_t = base - n + proc;
    else
      firstzgno_t = base;
  }
  for (zgno_t gid=firstzgno_t; gid <= max; gid+=nprocs){
    (*mygids).append(gid);
  }

  ArrayRCP<zgno_t> newIdArcp = Teuchos::arcp(mygids);

  return newIdArcp;
}

int main(int argc, char *argv[])
{
  Teuchos::GlobalMPISession session(&argc, &argv);
  RCP<const Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();
  int rank = comm->getRank();

  Teuchos::RCP<Teuchos::FancyOStream> outStream =
    Teuchos::VerboseObjectBase::getDefaultOStream();
  Teuchos::EVerbosityLevel v=Teuchos::VERB_EXTREME;

  typedef Tpetra::CrsMatrix<zscalar_t,zlno_t,zgno_t,znode_t> tmatrix_t;
  typedef Tpetra::CrsGraph<zlno_t,zgno_t,znode_t> tgraph_t;
  typedef Tpetra::Vector<zscalar_t,zlno_t,zgno_t,znode_t> tvector_t;
  typedef Tpetra::MultiVector<zscalar_t,zlno_t,zgno_t,znode_t> tmvector_t;
  typedef Xpetra::CrsMatrix<zscalar_t,zlno_t,zgno_t,znode_t> xmatrix_t;
  typedef Xpetra::CrsGraph<zlno_t,zgno_t,znode_t> xgraph_t;
  typedef Xpetra::Vector<zscalar_t,zlno_t,zgno_t,znode_t> xvector_t;
  typedef Xpetra::MultiVector<zscalar_t,zlno_t,zgno_t,znode_t> xmvector_t;
  typedef Xpetra::TpetraMap<zlno_t,zgno_t,znode_t> xtmap_t;

  // Create object that can give us test Tpetra and Xpetra input.

  RCP<UserInputForTests> uinput;

  try{
    uinput =
      rcp(new UserInputForTests(testDataFilePath,std::string("simple"), comm, true));
  }
  catch(std::exception &e){
    TEST_FAIL_AND_EXIT(*comm, 0, string("input ")+e.what(), 1);
  }

  /////////////////////////////////////////////////////////////////
  //   Tpetra::CrsMatrix
  //   Tpetra::CrsGraph
  //   Tpetra::Vector
  //   Tpetra::MultiVector
  /////////////////////////////////////////////////////////////////

  // XpetraTraits<Tpetra::CrsMatrix<zscalar_t, zlno_t, zgno_t, znode_t> >
  {
    RCP<tmatrix_t> M;

    try{
      M = uinput->getUITpetraCrsMatrix();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getTpetraCrsMatrix ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Tpetra matrix " << M->getGlobalNumRows()
        << " x " << M->getGlobalNumCols() << std::endl;

    M->describe(*outStream,v);

    RCP<const xtmap_t> xmap(new xtmap_t(M->getRowMap()));

    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const tmatrix_t> newM;
    try{
      newM = Zoltan2::XpetraTraits<tmatrix_t>::doMigration(*M,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<tmatrix_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Tpetra matrix" << std::endl;

    newM->describe(*outStream,v);
  }

  // XpetraTraits<Tpetra::CrsGraph<zscalar_t, zlno_t, zgno_t, znode_t> >
  {
    RCP<tgraph_t> G;

    try{
      G = uinput->getUITpetraCrsGraph();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getTpetraCrsGraph ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Tpetra graph" << std::endl;

    G->describe(*outStream,v);

    RCP<const xtmap_t> xmap(new xtmap_t(G->getRowMap()));
    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const tgraph_t> newG;
    try{
      newG = Zoltan2::XpetraTraits<tgraph_t>::doMigration(*G,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<tgraph_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Tpetra graph" << std::endl;

    newG->describe(*outStream,v);
  }

  // XpetraTraits<Tpetra::Vector<zscalar_t, zlno_t, zgno_t, znode_t>>
  {
    RCP<tvector_t> V;

    try{
      V = rcp(new tvector_t(uinput->getUITpetraCrsGraph()->getRowMap(),  1));
      V->randomize();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getTpetraVector")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Tpetra vector" << std::endl;

    V->describe(*outStream,v);

    RCP<const xtmap_t> xmap(new xtmap_t(V->getMap()));
    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const tvector_t> newV;
    try{
      newV = Zoltan2::XpetraTraits<tvector_t>::doMigration(*V,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<tvector_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Tpetra vector" << std::endl;

    newV->describe(*outStream,v);
  }

  // XpetraTraits<Tpetra::MultiVector<zscalar_t, zlno_t, zgno_t, znode_t>>
  {
    RCP<tmvector_t> MV;

    try{
      MV = rcp(new tmvector_t(uinput->getUITpetraCrsGraph()->getRowMap(), 3));
      MV->randomize();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getTpetraMultiVector")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Tpetra multivector" << std::endl;

    MV->describe(*outStream,v);

    RCP<const xtmap_t> xmap(new xtmap_t(MV->getMap()));
    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const tmvector_t> newMV;
    try{
      newMV = Zoltan2::XpetraTraits<tmvector_t>::doMigration(*MV,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<tmvector_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Tpetra multivector" << std::endl;

    newMV->describe(*outStream,v);
  }

  /////////////////////////////////////////////////////////////////
  //   Xpetra::CrsMatrix
  //   Xpetra::CrsGraph
  //   Xpetra::Vector
  //   Xpetra::MultiVector
  /////////////////////////////////////////////////////////////////

  // XpetraTraits<Xpetra::CrsMatrix<zscalar_t, zlno_t, zgno_t, znode_t> >
  {
    RCP<xmatrix_t> M;

    try{
      M = uinput->getUIXpetraCrsMatrix();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getXpetraCrsMatrix ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Xpetra matrix" << std::endl;

    M->describe(*outStream,v);

    ArrayRCP<zgno_t> newRowIds = roundRobinMap(M->getRowMap());

    zgno_t localNumRows = newRowIds.size();

    RCP<const xmatrix_t> newM;
    try{
      newM = Zoltan2::XpetraTraits<xmatrix_t>::doMigration(*M,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<xmatrix_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Xpetra matrix" << std::endl;

    newM->describe(*outStream,v);
  }

  // XpetraTraits<Xpetra::CrsGraph<zscalar_t, zlno_t, zgno_t, znode_t> >
  {
    RCP<xgraph_t> G;

    try{
      G = uinput->getUIXpetraCrsGraph();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getXpetraCrsGraph ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Xpetra graph" << std::endl;

    G->describe(*outStream,v);

    ArrayRCP<zgno_t> newRowIds = roundRobinMap(G->getRowMap());

    zgno_t localNumRows = newRowIds.size();

    RCP<const xgraph_t> newG;
    try{
      newG = Zoltan2::XpetraTraits<xgraph_t>::doMigration(*G,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<xgraph_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Xpetra graph" << std::endl;

    newG->describe(*outStream,v);
  }

  // XpetraTraits<Xpetra::Vector<zscalar_t, zlno_t, zgno_t, znode_t>>
  {
    RCP<xvector_t> V;

    try{
      RCP<tvector_t> tV =
          rcp(new tvector_t(uinput->getUITpetraCrsGraph()->getRowMap(),  1));
      tV->randomize();
      V = Zoltan2::XpetraTraits<tvector_t>::convertToXpetra(tV);
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getXpetraVector")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Xpetra vector" << std::endl;

    V->describe(*outStream,v);

    ArrayRCP<zgno_t> newRowIds = roundRobinMap(V->getMap());

    zgno_t localNumRows = newRowIds.size();

    RCP<const xvector_t> newV;
    try{
      newV = Zoltan2::XpetraTraits<xvector_t>::doMigration(*V,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<xvector_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Xpetra vector" << std::endl;

    newV->describe(*outStream,v);
  }

  // XpetraTraits<Xpetra::MultiVector<zscalar_t, zlno_t, zgno_t, znode_t>>
  {
    RCP<xmvector_t> MV;

    try{
      RCP<tmvector_t> tMV =
          rcp(new tmvector_t(uinput->getUITpetraCrsGraph()->getRowMap(), 3));
      tMV->randomize();
      MV = Zoltan2::XpetraTraits<tmvector_t>::convertToXpetra(tMV);
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getXpetraMultiVector")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Xpetra multivector" << std::endl;

    MV->describe(*outStream,v);

    ArrayRCP<zgno_t> newRowIds = roundRobinMap(MV->getMap());

    zgno_t localNumRows = newRowIds.size();

    RCP<const xmvector_t> newMV;
    try{
      newMV = Zoltan2::XpetraTraits<xmvector_t>::doMigration(*MV,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<xmvector_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Xpetra multivector" << std::endl;

    newMV->describe(*outStream,v);
  }

#ifdef HAVE_EPETRA_DATA_TYPES
  /////////////////////////////////////////////////////////////////
  //   Epetra_CrsMatrix
  //   Epetra_CrsGraph
  //   Epetra_Vector
  //   Epetra_MultiVector
  /////////////////////////////////////////////////////////////////

  typedef Epetra_CrsMatrix ematrix_t;
  typedef Epetra_CrsGraph egraph_t;
  typedef Epetra_Vector evector_t;
  typedef Epetra_MultiVector emvector_t;
  typedef Xpetra::EpetraMapT<zgno_t, znode_t> xemap_t;
  typedef Epetra_BlockMap emap_t;

  // Create object that can give us test Epetra input.

  RCP<UserInputForTests> euinput;

  try{
    euinput =
      rcp(new UserInputForTests(testDataFilePath,std::string("simple"), comm, true));
  }
  catch(std::exception &e){
    TEST_FAIL_AND_EXIT(*comm, 0, string("epetra input ")+e.what(), 1);
  }

  // XpetraTraits<Epetra_CrsMatrix>
  {
    RCP<ematrix_t> M;

    try{
      M = euinput->getUIEpetraCrsMatrix();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getEpetraCrsMatrix ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Epetra matrix" << std::endl;

    M->Print(std::cout);

    RCP<const emap_t> emap = Teuchos::rcpFromRef(M->RowMap());
    RCP<const xemap_t> xmap(new xemap_t(emap));

    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const ematrix_t> newM;
    try{
      newM = Zoltan2::XpetraTraits<ematrix_t>::doMigration(*M,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<ematrix_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Epetra matrix" << std::endl;

    newM->Print(std::cout);
  }

  // XpetraTraits<Epetra_CrsGraph>
  {
    RCP<egraph_t> G;

    try{
      G = euinput->getUIEpetraCrsGraph();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getEpetraCrsGraph ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Epetra graph" << std::endl;

    G->Print(std::cout);

    RCP<const emap_t> emap = Teuchos::rcpFromRef(G->RowMap());
    RCP<const xemap_t> xmap(new xemap_t(emap));
    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const egraph_t> newG;
    try{
      newG = Zoltan2::XpetraTraits<egraph_t>::doMigration(*G,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<egraph_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Epetra graph" << std::endl;

    newG->Print(std::cout);
  }

  // XpetraTraits<Epetra_Vector>
  {
    RCP<evector_t> V;

    try{
      V = rcp(new Epetra_Vector(euinput->getUIEpetraCrsGraph()->RowMap()));
      V->Random();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getEpetraVector")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Epetra vector" << std::endl;

    V->Print(std::cout);

    RCP<const emap_t> emap = Teuchos::rcpFromRef(V->Map());
    RCP<const xemap_t> xmap(new xemap_t(emap));
    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const evector_t> newV;
    try{
      newV = Zoltan2::XpetraTraits<evector_t>::doMigration(*V,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<evector_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Epetra vector" << std::endl;

    newV->Print(std::cout);
  }

  // XpetraTraits<Epetra_MultiVector>
  {
    RCP<emvector_t> MV;

    try{
      MV =
        rcp(new Epetra_MultiVector(euinput->getUIEpetraCrsGraph()->RowMap(),3));
      MV->Random();
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string("getEpetraMultiVector")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Original Epetra multivector" << std::endl;

    MV->Print(std::cout);

    RCP<const emap_t> emap = Teuchos::rcpFromRef(MV->Map());
    RCP<const xemap_t> xmap(new xemap_t(emap));
    ArrayRCP<zgno_t> newRowIds = roundRobinMap(xmap);

    zgno_t localNumRows = newRowIds.size();

    RCP<const emvector_t> newMV;
    try{
      newMV = Zoltan2::XpetraTraits<emvector_t>::doMigration(*MV,
        localNumRows, newRowIds.getRawPtr());
    }
    catch(std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0,
        string(" Zoltan2::XpetraTraits<emvector_t>::doMigration ")+e.what(), 1);
    }

    if (rank== 0)
      std::cout << "Migrated Epetra multivector" << std::endl;

    newMV->Print(std::cout);
  }
#endif   // have epetra data types (int, int, double)

  /////////////////////////////////////////////////////////////////
  // DONE
  /////////////////////////////////////////////////////////////////

  if (rank==0)
    std::cout << "PASS" << std::endl;
}

