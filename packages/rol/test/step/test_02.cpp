// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

/*! \file  test_02.cpp
    \brief Test trust region.
*/

#define USE_HESSVEC 1

#include "ROL_TestObjectives.hpp"
#include "ROL_Algorithm.hpp"
#include "ROL_TrustRegionStep.hpp"
#include "ROL_StatusTest.hpp"
#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"

#include <iostream>

typedef double RealT;

int main(int argc, char *argv[]) {

  Teuchos::GlobalMPISession mpiSession(&argc, &argv);

  // This little trick lets us print to std::cout only if a (dummy) command-line argument is provided.
  int iprint     = argc - 1;
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  int errorFlag  = 0;

  // *** Test body.

  try {

    std::string filename = "input.xml";
    Teuchos::RCP<Teuchos::ParameterList> parlist = Teuchos::rcp( new Teuchos::ParameterList() );
    Teuchos::updateParametersFromXmlFile( filename, parlist.ptr() );

    // Define Status Test
    Teuchos::RCP<ROL::StatusTest<RealT> > status = Teuchos::rcp(new ROL::StatusTest<RealT>(*parlist));

    // Loop Through Test Objectives
    for ( ROL::ETestObjectives objFunc = ROL::TESTOBJECTIVES_ROSENBROCK; objFunc < ROL::TESTOBJECTIVES_LAST; objFunc++ ) {
      *outStream << "\n\n" << ROL::ETestObjectivesToString(objFunc) << "\n\n";

      // Initial Guess Vector 
      Teuchos::RCP<std::vector<RealT> > x0_rcp = Teuchos::rcp( new std::vector<RealT> );
      ROL::StdVector<RealT> x0(x0_rcp);

      // Exact Solution Vector
      Teuchos::RCP<std::vector<RealT> > z_rcp = Teuchos::rcp( new std::vector<RealT> );
      ROL::StdVector<RealT> z(z_rcp);

      // Get Objective Function
      Teuchos::RCP<ROL::Objective<RealT> > obj = Teuchos::null;
      ROL::getTestObjectives<RealT>(obj,x0,z,objFunc);

      // Get Dimension of Problem
      int dim = 
        Teuchos::rcp_const_cast<std::vector<RealT> >((Teuchos::dyn_cast<ROL::StdVector<RealT> >(x0)).getVector())->size();
      parlist->sublist("General").sublist("Krylov").set("Iteration Limit", 2*dim);

      // Iteration Vector
      Teuchos::RCP<std::vector<RealT> > x_rcp = Teuchos::rcp( new std::vector<RealT> (dim, 0.0) );
      ROL::StdVector<RealT> x(x_rcp);
      x.set(x0);

      // Error Vector
      Teuchos::RCP<std::vector<RealT> > e_rcp = Teuchos::rcp( new std::vector<RealT> (dim, 0.0) );
      ROL::StdVector<RealT> e(e_rcp);
      e.zero();

      for ( ROL::ETrustRegion tr = ROL::TRUSTREGION_CAUCHYPOINT; tr < ROL::TRUSTREGION_LAST; tr++ ) {
        *outStream << "\n\n" << ROL::ETrustRegionToString(tr) << "\n\n";
        parlist->sublist("Step").sublist("Trust Region").set("Subproblem Solver", ETrustRegionToString(tr));
        if ( tr == ROL::TRUSTREGION_DOGLEG || tr == ROL::TRUSTREGION_DOUBLEDOGLEG ) {
          parlist->sublist("General").sublist("Secant").set("Secant Hessian-Times-A-Vector", true);
        } 
        else {
          parlist->sublist("General").sublist("Secant").set("Secant Hessian-Times-A-Vector", false);
        }

        // Define Step
        Teuchos::RCP<ROL::TrustRegionStep<RealT> > step = Teuchos::rcp(new ROL::TrustRegionStep<RealT>(*parlist));

        // Define Algorithm
        ROL::Algorithm<RealT> algo(step,status,false);

        // Run Algorithm
        x.set(x0);
        algo.run(x, *obj, true, *outStream);

        // Compute Error 
        e.set(x);
        e.axpy(-1.0,z);
        *outStream << "\nNorm of Error: " << e.norm() << "\n";
      }
    }
  }
  catch (std::logic_error err) {
    *outStream << err.what() << "\n";
    errorFlag = -1000;
  }; // end try

  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  return 0;

}

