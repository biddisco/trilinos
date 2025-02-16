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
// @HEADER
// ***********************************************************************
//         Zoltan2: Sandia Partitioning Ordering & Coloring Library
// ***********************************************************************

/*! \file XpetraMultiVectorInput.cpp
 *  \brief Test of Zoltan2::XpetraMultiVectorAdapter
 *  \todo test with weights
 */

#include <string>

#include <Zoltan2_XpetraMultiVectorAdapter.hpp>
#include <Zoltan2_InputTraits.hpp>
#include <Zoltan2_TestHelpers.hpp>

#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_Comm.hpp>
#include <Teuchos_CommHelpers.hpp>

using namespace std;
using Teuchos::RCP;
using Teuchos::rcp;
using Teuchos::rcp_const_cast;
using Teuchos::Comm;
using Teuchos::DefaultComm;

typedef Tpetra::MultiVector<zscalar_t, zlno_t, zgno_t, znode_t> tvector_t;
typedef Xpetra::MultiVector<zscalar_t, zlno_t, zgno_t, znode_t> xvector_t;

template <typename User>
int verifyInputAdapter(
  Zoltan2::XpetraMultiVectorAdapter<User> &ia, tvector_t &vector, int nvec,
    int wdim, zscalar_t **weights, int *strides)
{
  RCP<const Comm<int> > comm = vector.getMap()->getComm();
  int fail = 0, gfail=0;

  if (!fail && ia.getNumEntriesPerID() !=nvec) 
    fail = 42;

  if (!fail && ia.getNumWeightsPerID() !=wdim) 
    fail = 41;

  size_t length = vector.getLocalLength();

  if (!fail && ia.getLocalNumIDs() != length)
    fail = 4;

  gfail = globalFail(comm, fail);

  if (!gfail){
    const zgno_t *vtxIds=NULL;
    const zscalar_t *vals=NULL;
    int stride;

    size_t nvals = ia.getLocalNumIDs();
    if (nvals != vector.getLocalLength())
      fail = 8;

    ia.getIDsView(vtxIds);

    for (int v=0; v < nvec; v++){
      ia.getEntriesView(vals, stride, v);

      if (!fail && stride != 1)
        fail = 10;

      // TODO check the values returned
    }

    gfail = globalFail(comm, fail);
  }

  if (!gfail && wdim){
    const zscalar_t *wgt =NULL;
    int stride;

    for (int w=0; !fail && w < wdim; w++){
      ia.getWeightsView(wgt, stride, w);

      if (!fail && stride != strides[w])
        fail = 101;

      for (size_t v=0; !fail && v < vector.getLocalLength(); v++){
        if (wgt[v*stride] != weights[w][v*stride])
          fail=102;
      }
    }

    gfail = globalFail(comm, fail);
  }

  return gfail;
}

int main(int argc, char *argv[])
{
  Teuchos::GlobalMPISession session(&argc, &argv);
  RCP<const Comm<int> > comm = DefaultComm<int>::getComm();
  int rank = comm->getRank();
  int fail = 0, gfail=0;

  // Create object that can give us test Tpetra, Xpetra
  // and Epetra vectors for testing.

  RCP<UserInputForTests> uinput;

  try{
    uinput = 
      rcp(new UserInputForTests(testDataFilePath,std::string("simple"), comm, true));
  }
  catch(std::exception &e){
    TEST_FAIL_AND_EXIT(*comm, 0, string("input ")+e.what(), 1);
  }

  RCP<tvector_t> tV;     // original vector (for checking)
  RCP<tvector_t> newV;   // migrated vector

  int nVec = 2;

  tV = rcp(new tvector_t(uinput->getUITpetraCrsGraph()->getRowMap(), nVec));
  tV->randomize();

  size_t vlen = tV->getLocalLength();

  // To test migration in the input adapter we need a Solution object.

  RCP<const Zoltan2::Environment> env = rcp(new Zoltan2::Environment);

  int nWeights = 1;

  typedef Zoltan2::XpetraMultiVectorAdapter<tvector_t> ia_t;
  typedef Zoltan2::PartitioningSolution<ia_t> soln_t;
  typedef ia_t::part_t part_t;

  part_t *p = new part_t [vlen];
  memset(p, 0, sizeof(part_t) * vlen);
  ArrayRCP<part_t> solnParts(p, 0, vlen, true);

  soln_t solution(env, comm, nWeights);
  solution.setParts(solnParts);

  std::vector<const zscalar_t *> emptyWeights;
  std::vector<int> emptyStrides;

  /////////////////////////////////////////////////////////////
  // User object is Tpetra::MultiVector, no weights
  if (!gfail){ 
    RCP<const tvector_t> ctV = rcp_const_cast<const tvector_t>(tV);
    RCP<Zoltan2::XpetraMultiVectorAdapter<tvector_t> > tVInput;
  
    try {
      tVInput = 
        rcp(new Zoltan2::XpetraMultiVectorAdapter<tvector_t>(ctV, 
          emptyWeights, emptyStrides));
    }
    catch (std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0, 
        string("XpetraMultiVectorAdapter ")+e.what(), 1);
    }
  
    if (rank==0){
      std::cout << "Constructed with ";
      std::cout  << "Tpetra::MultiVector" << std::endl;
    }
    
    fail = verifyInputAdapter<tvector_t>(*tVInput, *tV, nVec, 0, NULL, NULL);
  
    gfail = globalFail(comm, fail);
  
    if (!gfail){
      tvector_t *vMigrate = NULL;
      try{
        tVInput->applyPartitioningSolution(*tV, vMigrate, solution);
        newV = rcp(vMigrate);
      }
      catch (std::exception &e){
        fail = 11;
      }

      gfail = globalFail(comm, fail);
  
      if (!gfail){
        RCP<const tvector_t> cnewV = rcp_const_cast<const tvector_t>(newV);
        RCP<Zoltan2::XpetraMultiVectorAdapter<tvector_t> > newInput;
        try{
          newInput = rcp(new Zoltan2::XpetraMultiVectorAdapter<tvector_t>(
            cnewV, emptyWeights, emptyStrides));
        }
        catch (std::exception &e){
          TEST_FAIL_AND_EXIT(*comm, 0, 
            string("XpetraMultiVectorAdapter 2 ")+e.what(), 1);
        }
  
        if (rank==0){
          std::cout << "Constructed with ";
          std::cout << "Tpetra::MultiVector migrated to proc 0" << std::endl;
        }
        fail = verifyInputAdapter<tvector_t>(*newInput, *newV, nVec, 0, NULL, NULL);
        if (fail) fail += 100;
        gfail = globalFail(comm, fail);
      }
    }
    if (gfail){
      printFailureCode(comm, fail);
    }
  }

  /////////////////////////////////////////////////////////////
  // User object is Xpetra::MultiVector
  if (!gfail){ 
    RCP<tvector_t> tMV = 
        rcp(new tvector_t(uinput->getUITpetraCrsGraph()->getRowMap(), nVec));
    tMV->randomize();
    RCP<xvector_t> xV = Zoltan2::XpetraTraits<tvector_t>::convertToXpetra(tMV);
    RCP<const xvector_t> cxV = rcp_const_cast<const xvector_t>(xV);
    RCP<Zoltan2::XpetraMultiVectorAdapter<xvector_t> > xVInput;
  
    try {
      xVInput = 
        rcp(new Zoltan2::XpetraMultiVectorAdapter<xvector_t>(cxV, 
          emptyWeights, emptyStrides));
    }
    catch (std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0, 
        string("XpetraMultiVectorAdapter 3 ")+e.what(), 1);
    }
  
    if (rank==0){
      std::cout << "Constructed with ";
      std::cout << "Xpetra::MultiVector" << std::endl;
    }
    fail = verifyInputAdapter<xvector_t>(*xVInput, *tV, nVec, 0, NULL, NULL);
  
    gfail = globalFail(comm, fail);
  
    if (!gfail){
      xvector_t *vMigrate =NULL;
      try{
        xVInput->applyPartitioningSolution(*xV, vMigrate, solution);
      }
      catch (std::exception &e){
        fail = 11;
      }
  
      gfail = globalFail(comm, fail);
  
      if (!gfail){
        RCP<const xvector_t> cnewV(vMigrate);
        RCP<Zoltan2::XpetraMultiVectorAdapter<xvector_t> > newInput;
        try{
          newInput = 
            rcp(new Zoltan2::XpetraMultiVectorAdapter<xvector_t>(
              cnewV, emptyWeights, emptyStrides));
        }
        catch (std::exception &e){
          TEST_FAIL_AND_EXIT(*comm, 0, 
            string("XpetraMultiVectorAdapter 4 ")+e.what(), 1);
        }
  
        if (rank==0){
          std::cout << "Constructed with ";
          std::cout << "Xpetra::MultiVector migrated to proc 0" << std::endl;
        }
        fail = verifyInputAdapter<xvector_t>(*newInput, *newV, nVec, 0, NULL, NULL);
        if (fail) fail += 100;
        gfail = globalFail(comm, fail);
      }
    }
    if (gfail){
      printFailureCode(comm, fail);
    }
  }

#ifdef HAVE_EPETRA_DATA_TYPES
  /////////////////////////////////////////////////////////////
  // User object is Epetra_MultiVector
  typedef Epetra_MultiVector evector_t;
  if (!gfail){ 
    RCP<evector_t> eV = 
        rcp(new Epetra_MultiVector(uinput->getUIEpetraCrsGraph()->RowMap(),
                                   nVec));
    eV->Random();
    RCP<const evector_t> ceV = rcp_const_cast<const evector_t>(eV);
    RCP<Zoltan2::XpetraMultiVectorAdapter<evector_t> > eVInput;
  
    try {
      eVInput = 
        rcp(new Zoltan2::XpetraMultiVectorAdapter<evector_t>(ceV,
              emptyWeights, emptyStrides));
    }
    catch (std::exception &e){
      TEST_FAIL_AND_EXIT(*comm, 0, 
        string("XpetraMultiVectorAdapter 5 ")+e.what(), 1);
    }
  
    if (rank==0){
      std::cout << "Constructed with ";
      std::cout << "Epetra_MultiVector" << std::endl;
    }
    fail = verifyInputAdapter<evector_t>(*eVInput, *tV, nVec, 0, NULL, NULL);
  
    gfail = globalFail(comm, fail);
  
    if (!gfail){
      evector_t *vMigrate =NULL;
      try{
        eVInput->applyPartitioningSolution(*eV, vMigrate, solution);
      }
      catch (std::exception &e){
        fail = 11;
      }
  
      gfail = globalFail(comm, fail);
  
      if (!gfail){
        RCP<const evector_t> cnewV(vMigrate, true);
        RCP<Zoltan2::XpetraMultiVectorAdapter<evector_t> > newInput;
        try{
          newInput = 
            rcp(new Zoltan2::XpetraMultiVectorAdapter<evector_t>(cnewV, 
              emptyWeights, emptyStrides));
        }
        catch (std::exception &e){
          TEST_FAIL_AND_EXIT(*comm, 0, 
            string("XpetraMultiVectorAdapter 6 ")+e.what(), 1);
        }
  
        if (rank==0){
          std::cout << "Constructed with ";
          std::cout << "Epetra_MultiVector migrated to proc 0" << std::endl;
        }
        fail = verifyInputAdapter<evector_t>(*newInput, *newV, nVec, 0, NULL, NULL);
        if (fail) fail += 100;
        gfail = globalFail(comm, fail);
      }
    }
    if (gfail){
      printFailureCode(comm, fail);
    }
  }
#endif

  /////////////////////////////////////////////////////////////
  // DONE

  if (rank==0)
    std::cout << "PASS" << std::endl;
}
