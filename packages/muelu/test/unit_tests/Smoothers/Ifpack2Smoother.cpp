// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
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
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#include "Teuchos_UnitTestHarness.hpp"
#include "MueLu_TestHelpers.hpp"
#include "MueLu_TestHelpersSmoothers.hpp"

#include "MueLu_Ifpack2Smoother.hpp"
#include "MueLu_Utilities.hpp"

#include "MueLu_UseDefaultTypes.hpp"

/*
   Comments about tests with hard coded results:
   1) Chebyshev smoothing must pass for any number of processors.
   2) Gauss-Seidel must pass for 1 and 4 processors.
   3) For any processor count except 1 and 4, the Gauss-Seidel test will
   report "passing", but this is only because the Teuchos test macro is skipped.
   */

namespace MueLuTests {

#include "MueLu_UseShortNames.hpp"

  // this namespace already has:  #include "MueLu_UseShortNames.hpp"
  using namespace TestHelpers::Smoothers;

  TEUCHOS_UNIT_TEST(Ifpack2Smoother, NotSetup)
  {
    MUELU_TEST_ONLY_FOR(Xpetra::UseTpetra) {

      Ifpack2Smoother smoother("RELAXATION", Teuchos::ParameterList());
      testApplyNoSetup(smoother, out, success);

    }
  }

  // Tests interface to Ifpack2's Gauss-Seidel preconditioner.
  TEUCHOS_UNIT_TEST(Ifpack2Smoother, HardCodedResult_GaussSeidel)
  {
    MUELU_TEST_ONLY_FOR(Xpetra::UseTpetra) {

      Teuchos::ParameterList paramList;
      paramList.set("relaxation: type", "Gauss-Seidel");
      paramList.set("relaxation: sweeps", (int) 1);
      paramList.set("relaxation: damping factor", (double) 1.0);
      paramList.set("relaxation: zero starting solution", false);

      Ifpack2Smoother smoother("RELAXATION", paramList);

      Teuchos::ScalarTraits<SC>::magnitudeType residualNorms = testApply_A125_X1_RHS0(smoother, out, success);

      RCP<const Teuchos::Comm<int> > comm = TestHelpers::Parameters::getDefaultComm();
      switch (comm->getSize()) {
        case 1:
        case 4:
          TEST_FLOATING_EQUALITY(residualNorms,5.773502691896257e-01,1e-12);
          break;
        default:
          out << "Pass/Fail is checked only for 1 and 4 processes." << std::endl;
          break;
      } // switch

    }
  } // GaussSeidel

  // Tests interface to Ifpack2's Gauss-Seidel preconditioner.
  TEUCHOS_UNIT_TEST(Ifpack2Smoother, HardCodedResult_GaussSeidel2)
  {
    MUELU_TEST_ONLY_FOR(Xpetra::UseTpetra) {

      Teuchos::ParameterList paramList;
      paramList.set("relaxation: type", "Gauss-Seidel");
      paramList.set("relaxation: sweeps", (int) 10);
      paramList.set("relaxation: damping factor", (double) 1.0);
      paramList.set("relaxation: zero starting solution", false);

      Ifpack2Smoother smoother("RELAXATION",paramList);

      Teuchos::ScalarTraits<SC>::magnitudeType residualNorms = testApply_A125_X1_RHS0(smoother, out, success);

      RCP<const Teuchos::Comm<int> > comm = TestHelpers::Parameters::getDefaultComm();
      switch (comm->getSize()) {
        case 1:
          TEST_FLOATING_EQUALITY(residualNorms, 8.326553652741774e-02, 1e-12);
          break;
        case 4:
          TEST_FLOATING_EQUALITY(residualNorms, 8.326553653078517e-02, 1e-12);
          break;
        default:
          out << "Pass/Fail is checked only for 1 and 4 processes." << std::endl;
          break;
      } // switch

    }
  } // GaussSeidel

  // Tests interface to Ifpack2's Chebyshev preconditioner
  TEUCHOS_UNIT_TEST(Ifpack2Smoother, HardCodedResult_Chebyshev)
  {
    MUELU_TEST_ONLY_FOR(Xpetra::UseTpetra) {

      Teuchos::ParameterList paramList;
      paramList.set("chebyshev: degree", (int) 3);
      paramList.set("chebyshev: max eigenvalue", (double) 1.98476);
      paramList.set("chebyshev: min eigenvalue", (double) 1.0);
      paramList.set("chebyshev: ratio eigenvalue", (double) 20);
      paramList.set("chebyshev: zero starting solution", false);
      Ifpack2Smoother smoother("CHEBYSHEV",paramList);

      Teuchos::ScalarTraits<SC>::magnitudeType residualNorms = testApply_A125_X1_RHS0(smoother, out, success);

      TEST_FLOATING_EQUALITY(residualNorms, 5.269156e-01, 1e-7);  // Compare to residual reported by ML

    }
  } // Chebyshev

  // Tests interface to Ifpack2's ILU(0) preconditioner.
  TEUCHOS_UNIT_TEST(Ifpack2Smoother, HardCodedResult_ILU)
  {
    MUELU_TEST_ONLY_FOR(Xpetra::UseTpetra) {

      //FIXME this will probably fail in parallel b/c it becomes block Jacobi

      Teuchos::ParameterList paramList;
      Ifpack2Smoother smoother("ILUT",paramList);

      Teuchos::ScalarTraits<SC>::magnitudeType residualNorms = testApply_A125_X0_RandomRHS(smoother, out, success);

      RCP<const Teuchos::Comm<int> > comm = TestHelpers::Parameters::getDefaultComm();
      if (comm->getSize() == 1) {
        TEST_EQUALITY(residualNorms < 1e-10, true);
      } else {
        out << "Pass/Fail is only checked in serial." << std::endl;
      }

    }
  } // ILU

  // Tests two sweeps of ILUT in Ifpack2
  TEUCHOS_UNIT_TEST(Ifpack2Smoother, ILU_TwoSweeps)
  {
    MUELU_TEST_ONLY_FOR(Xpetra::UseTpetra) {

      //FIXME this will probably fail in parallel b/c it becomes block Jacobi

      Teuchos::ParameterList paramList;
      Ifpack2Smoother smoother("ILUT",paramList);

      //I don't use the testApply infrastructure because it has no provision for an initial guess.
      Teuchos::RCP<Matrix> A = TestHelpers::TestFactory<SC, LO, GO, NO>::Build1DPoisson(125);
      Level level; TestHelpers::TestFactory<SC,LO,GO,NO>::createSingleLevelHierarchy(level);
      level.Set("A", A);
      smoother.Setup(level);

      RCP<MultiVector> X   = MultiVectorFactory::Build(A->getDomainMap(),1);
      RCP<MultiVector> RHS = MultiVectorFactory::Build(A->getRangeMap(),1);

      // Random X
      X->setSeed(846930886);
      X->randomize();

      // Normalize X
      Array<Teuchos::ScalarTraits<SC>::magnitudeType> norms(1); X->norm2(norms);
      X->scale(1/norms[0]);

      // Compute RHS corresponding to X
      A->apply(*X,*RHS, Teuchos::NO_TRANS,(SC)1.0,(SC)0.0);

      // Reset X to 0
      X->putScalar((SC) 0.0);

      RHS->norm2(norms);
      out << "||RHS|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << norms[0] << std::endl;

      out << "solve with zero initial guess" << std::endl;
      Teuchos::Array<Teuchos::ScalarTraits<SC>::magnitudeType> initialNorms(1); X->norm2(initialNorms);
      out << "  ||X_initial|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << initialNorms[0] << std::endl;

      smoother.Apply(*X, *RHS, true);  //zero initial guess

      Teuchos::Array<Teuchos::ScalarTraits<SC>::magnitudeType> finalNorms(1); X->norm2(finalNorms);
      Teuchos::Array<Teuchos::ScalarTraits<SC>::magnitudeType> residualNorm1 = Utilities::ResidualNorm(*A, *X, *RHS);
      out << "  ||Residual_final|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(20) << residualNorm1[0] << std::endl;
      out << "  ||X_final|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << finalNorms[0] << std::endl;

      out << "solve with random initial guess" << std::endl;
      X->randomize();
      X->norm2(initialNorms);
      out << "  ||X_initial|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << initialNorms[0] << std::endl;

      smoother.Apply(*X, *RHS, false); //nonzero initial guess

      X->norm2(finalNorms);
      Teuchos::Array<Teuchos::ScalarTraits<SC>::magnitudeType> residualNorm2 = Utilities::ResidualNorm(*A, *X, *RHS);
      out << "  ||Residual_final|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(20) << residualNorm2[0] << std::endl;
      out << "  ||X_final|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << finalNorms[0] << std::endl;

      RCP<const Teuchos::Comm<int> > comm = TestHelpers::Parameters::getDefaultComm();
      if (comm->getSize() == 1) {
        //TEST_EQUALITY(residualNorms < 1e-10, true);
        TEST_EQUALITY(residualNorm1[0] != residualNorm2[0], true);
      } else {
        out << "Pass/Fail is only checked in serial." << std::endl;
      }

    }
  } // ILU

} // namespace MueLuTests
