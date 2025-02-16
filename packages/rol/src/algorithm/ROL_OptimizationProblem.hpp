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

#ifndef ROL_OPTIMIZATIONPROBLEM_HPP
#define ROL_OPTIMIZATIONPROBLEM_HPP

#include "Teuchos_ParameterList.hpp"
#include "ROL_Objective.hpp"
#include "ROL_Vector.hpp"
#include "ROL_BoundConstraint.hpp"
#include "ROL_EqualityConstraint.hpp"

namespace ROL {

template<class Real>
class OptimizationProblem {
private:
  Teuchos::RCP<Objective<Real> > obj_;
  Teuchos::RCP<Vector<Real> > sol_;
  Teuchos::RCP<BoundConstraint<Real> > bnd_;
  Teuchos::RCP<EqualityConstraint<Real> > con_;
  Teuchos::RCP<Vector<Real> > mul_;

public:
  virtual ~OptimizationProblem(void) {}

  OptimizationProblem(void)
    : obj_(Teuchos::null), sol_(Teuchos::null), bnd_(Teuchos::null),
      con_(Teuchos::null), mul_(Teuchos::null) {}

  OptimizationProblem(Teuchos::RCP<Objective<Real> > &obj,
                      Teuchos::RCP<Vector<Real> > &sol,
                      Teuchos::RCP<BoundConstraint<Real> > &bnd = Teuchos::null)
    : obj_(obj), sol_(sol), bnd_(bnd), con_(Teuchos::null), mul_(Teuchos::null) {}

  OptimizationProblem(Teuchos::RCP<Objective<Real> > &obj,
                      Teuchos::RCP<Vector<Real> > &sol,
                      Teuchos::RCP<EqualityConstraint<Real> > &con,
                      Teuchos::RCP<Vector<Real> > &mul)
    : obj_(obj), sol_(sol), bnd_(Teuchos::null), con_(con), mul_(mul) {}

  OptimizationProblem(Teuchos::RCP<Objective<Real> > &obj,
                      Teuchos::RCP<Vector<Real> > &sol,
                      Teuchos::RCP<BoundConstraint<Real> > &bnd,
                      Teuchos::RCP<EqualityConstraint<Real> > &con,
                      Teuchos::RCP<Vector<Real> > &mul)
    : obj_(obj), sol_(sol), bnd_(bnd), con_(con), mul_(mul) {}

  Teuchos::RCP<Objective<Real> > getObjective(void) {
    return obj_;
  }

  void setObjective(Teuchos::RCP<Objective<Real> > &obj) {
    obj_ = obj;
  }

  Teuchos::RCP<Vector<Real> > getSolutionVector(void) {
    return sol_;
  }

  void setSolutionVector(Teuchos::RCP<Vector<Real> > &sol) {
    sol_ = sol;
  }

  Teuchos::RCP<BoundConstraint<Real> > getBoundConstraint(void) {
    return bnd_;
  }

  void setBoundConstraint(Teuchos::RCP<BoundConstraint<Real> > &bnd) {
    bnd_ = bnd;
  }

  Teuchos::RCP<EqualityConstraint<Real> > getEqualityConstraint(void) {
    return con_;
  }

  void setEqualityConstraint(Teuchos::RCP<EqualityConstraint<Real> > &con) {
    con_ = con;
  }

  Teuchos::RCP<Vector<Real> > getMultiplierVector(void) {
    return mul_;
  }

  void setMultiplierVector(Teuchos::RCP<Vector<Real> > &mul) {
    mul_ = mul;
  }

  std::vector<std::vector<Real> > checkObjectiveGradient( const Vector<Real> &d,
                                                          const bool printToStream = true,
                                                          std::ostream & outStream = std::cout,
                                                          const int numSteps = ROL_NUM_CHECKDERIV_STEPS,
                                                          const int order = 1 ) {
    return obj_->checkGradient(*sol_,d,printToStream,outStream,numSteps,order);
  }

  std::vector<std::vector<Real> > checkObjectiveHessVec( const Vector<Real> &v,
                                                         const bool printToStream = true,
                                                         std::ostream & outStream = std::cout,
                                                         const int numSteps = ROL_NUM_CHECKDERIV_STEPS,
                                                         const int order = 1 ) {
    return obj_->checkHessVec(*sol_,v,printToStream,outStream,numSteps,order);
  }

};
}
#endif
