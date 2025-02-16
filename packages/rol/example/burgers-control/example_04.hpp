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

/*! \file  example_03.cpp
    \brief Shows how to solve a steady Burgers' optimal control problem using
           full-space methods.
*/

#include "ROL_Types.hpp"
#include "ROL_Vector.hpp"
#include "ROL_BoundConstraint.hpp"
#include "ROL_EqualityConstraint_SimOpt.hpp"
#include "ROL_Objective_SimOpt.hpp"

#include "Teuchos_LAPACK.hpp"

template<class Real>
class L2VectorPrimal;

template<class Real>
class L2VectorDual;

template<class Real>
class H1VectorPrimal;

template<class Real>
class H1VectorDual;

template<class Real>
class BurgersFEM {
private:
  int nx_;
  Real dx_;
  Real nu_;
  Real nl_;
  Real u0_;
  Real u1_;
  Real f_;
  Real cH1_;
  Real cL2_;

private:
  void update(std::vector<Real> &u, const std::vector<Real> &s, const Real alpha=1.0) const {
    for (unsigned i=0; i<u.size(); i++) {
      u[i] += alpha*s[i];
    }
  }

  void axpy(std::vector<Real> &out, const Real a, const std::vector<Real> &x, const std::vector<Real> &y) const {
    for (unsigned i=0; i < x.size(); i++) {
      out[i] = a*x[i] + y[i];
    }
  }

  void scale(std::vector<Real> &u, const Real alpha=0.0) const {
    for (unsigned i=0; i<u.size(); i++) {
      u[i] *= alpha;
    }
  }

  void linear_solve(std::vector<Real> &u, std::vector<Real> &dl, std::vector<Real> &d, std::vector<Real> &du, 
              const std::vector<Real> &r, const bool transpose = false) const {
    if ( r.size() == 1 ) {
      u.resize(1,r[0]/d[0]);
    }
    else if ( r.size() == 2 ) {
      u.resize(2,0.0);
      Real det = d[0]*d[1] - dl[0]*du[0];
      u[0] = (d[1]*r[0] - du[0]*r[1])/det;
      u[1] = (d[0]*r[1] - dl[0]*r[0])/det;
    }
    else {
      u.assign(r.begin(),r.end());
      // Perform LDL factorization
      Teuchos::LAPACK<int,Real> lp;
      std::vector<Real> du2(r.size()-2,0.0);
      std::vector<int> ipiv(r.size(),0);
      int info;
      int dim  = r.size();
      int ldb  = r.size();
      int nhrs = 1;
      lp.GTTRF(dim,&dl[0],&d[0],&du[0],&du2[0],&ipiv[0],&info);
      char trans = 'N';
      if ( transpose ) { 
        trans = 'T';
      }
      lp.GTTRS(trans,dim,nhrs,&dl[0],&d[0],&du[0],&du2[0],&ipiv[0],&u[0],ldb,&info);
    }
  }

public:
  BurgersFEM(int nx = 128, Real nu = 1.e-2, Real nl = 1.0,
             Real u0 = 1.0, Real u1 = 0.0, Real f = 0.0,
             Real cH1 = 1.0, Real cL2 = 1.0) 
    : nx_(nx), dx_(1.0/((Real)nx+1.0)),
      nu_(nu), nl_(nl), u0_(u0), u1_(u1), f_(f),
      cH1_(cH1), cL2_(cL2) {}

  int num_dof(void) const {
    return nx_;
  }

  Real mesh_spacing(void) const {
    return dx_;
  }

  /***************************************************************************/
  /*********************** L2 INNER PRODUCT FUNCTIONS ************************/
  /***************************************************************************/
  // Compute L2 inner product
  Real compute_L2_dot(const std::vector<Real> &x, const std::vector<Real> &y) const {
    Real ip = 0.0;
    Real c = (((int)x.size()==nx_) ? 4.0 : 2.0);
    for (unsigned i=0; i<x.size(); i++) {
      if ( i == 0 ) {
        ip += dx_/6.0*(c*x[i] + x[i+1])*y[i];
      }
      else if ( i == x.size()-1 ) {
        ip += dx_/6.0*(x[i-1] + c*x[i])*y[i];
      }
      else {
        ip += dx_/6.0*(x[i-1] + 4.0*x[i] + x[i+1])*y[i];
      }
    }
    return ip;
  }

  // compute L2 norm
  Real compute_L2_norm(const std::vector<Real> &r) const {
    return std::sqrt(compute_L2_dot(r,r));
  }

  // Apply L2 Reisz operator
  void apply_mass(std::vector<Real> &Mu, const std::vector<Real> &u ) const {
    Mu.resize(u.size(),0.0);
    Real c = (((int)u.size()==nx_) ? 4.0 : 2.0);
    for (unsigned i=0; i<u.size(); i++) {
      if ( i == 0 ) {
        Mu[i] = dx_/6.0*(c*u[i] + u[i+1]);
      }
      else if ( i == u.size()-1 ) {
        Mu[i] = dx_/6.0*(u[i-1] + c*u[i]);
      }
      else {
        Mu[i] = dx_/6.0*(u[i-1] + 4.0*u[i] + u[i+1]);
      }
    }
  }

  // Apply L2 inverse Reisz operator
  void apply_inverse_mass(std::vector<Real> &Mu, const std::vector<Real> &u) const {
    unsigned nx = u.size();
    // Build mass matrix
    std::vector<Real> dl(nx-1,dx_/6.0);
    std::vector<Real> d(nx,2.0*dx_/3.0);
    std::vector<Real> du(nx-1,dx_/6.0);
    if ( (int)nx != nx_ ) {
      d[   0] = dx_/3.0;
      d[nx-1] = dx_/3.0;
    }
    linear_solve(Mu,dl,d,du,u);
  }

  void test_inverse_mass(std::ostream &outStream = std::cout) {
    // State Mass Matrix
    std::vector<Real> u(nx_,0.0), Mu(nx_,0.0), iMMu(nx_,0.0), diff(nx_,0.0);
    for (int i = 0; i < nx_; i++) {
      u[i] = 2.0*(Real)rand()/(Real)RAND_MAX - 1.0;
    }
    apply_mass(Mu,u);
    apply_inverse_mass(iMMu,Mu);
    axpy(diff,-1.0,iMMu,u);
    Real error = compute_L2_norm(diff);
    Real normu = compute_L2_norm(u);
    outStream << "Test Inverse State Mass Matrix\n";
    outStream << "  ||u - inv(M)Mu|| = " << error << "\n";
    outStream << "  ||u||            = " << normu << "\n";
    outStream << "  Relative Error   = " << error/normu << "\n";
    outStream << "\n";
    // Control Mass Matrix
    u.resize(nx_+2,0.0); Mu.resize(nx_+2,0.0); iMMu.resize(nx_+2,0.0); diff.resize(nx_+2,0.0);
    for (int i = 0; i < nx_+2; i++) {
      u[i] = 2.0*(Real)rand()/(Real)RAND_MAX - 1.0;
    }
    apply_mass(Mu,u);
    apply_inverse_mass(iMMu,Mu);
    axpy(diff,-1.0,iMMu,u);
    error = compute_L2_norm(diff);
    normu = compute_L2_norm(u);
    outStream << "Test Inverse Control Mass Matrix\n";
    outStream << "  ||z - inv(M)Mz|| = " << error << "\n";
    outStream << "  ||z||            = " << normu << "\n";
    outStream << "  Relative Error   = " << error/normu << "\n";
    outStream << "\n";
  }

  /***************************************************************************/
  /*********************** H1 INNER PRODUCT FUNCTIONS ************************/
  /***************************************************************************/
  // Compute H1 inner product
  Real compute_H1_dot(const std::vector<Real> &x, const std::vector<Real> &y) const {
    Real ip = 0.0;
    for (int i=0; i<nx_; i++) {
      if ( i == 0 ) {
        ip += cL2_*dx_/6.0*(4.0*x[i] + x[i+1])*y[i]; // Mass term
        ip += cH1_*(2.0*x[i] - x[i+1])/dx_*y[i];     // Stiffness term
      }
      else if ( i == nx_-1 ) {
        ip += cL2_*dx_/6.0*(x[i-1] + 4.0*x[i])*y[i]; // Mass term
        ip += cH1_*(2.0*x[i] - x[i-1])/dx_*y[i];     // Stiffness term
      }
      else {
        ip += cL2_*dx_/6.0*(x[i-1] + 4.0*x[i] + x[i+1])*y[i]; // Mass term
        ip += cH1_*(2.0*x[i] - x[i-1] - x[i+1])/dx_*y[i];     // Stiffness term
      }
    }
    return ip;
  }

  // compute H1 norm
  Real compute_H1_norm(const std::vector<Real> &r) const {
    return std::sqrt(compute_H1_dot(r,r));
  }

  // Apply H2 Reisz operator
  void apply_H1(std::vector<Real> &Mu, const std::vector<Real> &u ) const {
    Mu.resize(nx_,0.0);
    for (int i=0; i<nx_; i++) {
      if ( i == 0 ) {
        Mu[i] = cL2_*dx_/6.0*(4.0*u[i] + u[i+1])
              + cH1_*(2.0*u[i] - u[i+1])/dx_;
      }
      else if ( i == nx_-1 ) {
        Mu[i] = cL2_*dx_/6.0*(u[i-1] + 4.0*u[i])
              + cH1_*(2.0*u[i] - u[i-1])/dx_;
      }
      else {
        Mu[i] = cL2_*dx_/6.0*(u[i-1] + 4.0*u[i] + u[i+1])
              + cH1_*(2.0*u[i] - u[i-1] - u[i+1])/dx_;
      }
    }
  }

  // Apply H1 inverse Reisz operator
  void apply_inverse_H1(std::vector<Real> &Mu, const std::vector<Real> &u) const {
    // Build mass matrix
    std::vector<Real> dl(nx_-1,cL2_*dx_/6.0   - cH1_/dx_);
    std::vector<Real> d(nx_,2.0*(cL2_*dx_/3.0 + cH1_/dx_));
    std::vector<Real> du(nx_-1,cL2_*dx_/6.0   - cH1_/dx_);
    linear_solve(Mu,dl,d,du,u);
  }

  void test_inverse_H1(std::ostream &outStream = std::cout) {
    std::vector<Real> u(nx_,0.0), Mu(nx_,0.0), iMMu(nx_,0.0), diff(nx_,0.0);
    for (int i = 0; i < nx_; i++) {
      u[i] = 2.0*(Real)rand()/(Real)RAND_MAX - 1.0;
    }
    apply_H1(Mu,u);
    apply_inverse_H1(iMMu,Mu);
    axpy(diff,-1.0,iMMu,u);
    Real error = compute_H1_norm(diff);
    Real normu = compute_H1_norm(u);
    outStream << "Test Inverse State H1 Matrix\n";
    outStream << "  ||u - inv(M)Mu|| = " << error << "\n";
    outStream << "  ||u||            = " << normu << "\n";
    outStream << "  Relative Error   = " << error/normu << "\n";
    outStream << "\n";
  }

  /***************************************************************************/
  /*********************** PDE RESIDUAL AND SOLVE ****************************/
  /***************************************************************************/
  // Compute current PDE residual
  void compute_residual(std::vector<Real> &r, const std::vector<Real> &u, 
                  const std::vector<Real> &z) const {
    r.clear();
    r.resize(nx_,0.0);
    for (int i=0; i<nx_; i++) {
      // Contribution from stiffness term
      if ( i==0 ) {
        r[i] = nu_/dx_*(2.0*u[i]-u[i+1]);
      }
      else if (i==nx_-1) {
        r[i] = nu_/dx_*(2.0*u[i]-u[i-1]);
      }
      else {
        r[i] = nu_/dx_*(2.0*u[i]-u[i-1]-u[i+1]);
      }
      // Contribution from nonlinear term
      if (i<nx_-1){
        r[i] += nl_*u[i+1]*(u[i]+u[i+1])/6.0;
      }
      if (i>0) {
        r[i] -= nl_*u[i-1]*(u[i-1]+u[i])/6.0;
      }
      // Contribution from control
      r[i] -= dx_/6.0*(z[i]+4.0*z[i+1]+z[i+2]);
      // Contribution from right-hand side
      r[i] -= dx_*f_;
    }
    // Contribution from Dirichlet boundary terms
    r[0]     -= nl_*(u0_*u[    0]/6.0 + u0_*u0_/6.0) + nu_*u0_/dx_;
    r[nx_-1] += nl_*(u1_*u[nx_-1]/6.0 + u1_*u1_/6.0) - nu_*u1_/dx_;
  }

  // Solve PDE
  void solve(std::vector<Real> &u, const std::vector<Real> &z) const {
    // Compute residual and residual norm
    std::vector<Real> r(u.size(),0.0);
    compute_residual(r,u,z);
    Real rnorm = compute_L2_norm(r);
    // Define tolerances
    Real rtol  = 1.e2*ROL::ROL_EPSILON;
    Real maxit = 500;
    // Initialize Jacobian storage
    std::vector<Real> d(nx_,0.0);
    std::vector<Real> dl(nx_-1,0.0);
    std::vector<Real> du(nx_-1,0.0);
    // Iterate Newton's method
    Real alpha = 1.0, tmp = 0.0;
    std::vector<Real> s(nx_,0.0);
    std::vector<Real> utmp(nx_,0.0);
    for (int i=0; i<maxit; i++) {
      //std::cout << i << "  " << rnorm << "\n";
      // Get Jacobian
      compute_pde_jacobian(dl,d,du,u);
      // Solve Newton system
      linear_solve(s,dl,d,du,r);
      // Perform line search
      tmp = rnorm;
      alpha = 1.0;
      utmp.assign(u.begin(),u.end());
      update(utmp,s,-alpha);
      compute_residual(r,utmp,z);
      rnorm = compute_L2_norm(r); 
      while ( rnorm > (1.0-1.e-4*alpha)*tmp && alpha > std::sqrt(ROL::ROL_EPSILON) ) {
        alpha /= 2.0;
        utmp.assign(u.begin(),u.end());
        update(utmp,s,-alpha);
        compute_residual(r,utmp,z);
        rnorm = compute_L2_norm(r); 
      }
      // Update iterate
      u.assign(utmp.begin(),utmp.end());
      if ( rnorm < rtol ) {
        break;
      }
    }
  }

  /***************************************************************************/
  /*********************** PDE JACOBIAN FUNCTIONS ****************************/
  /***************************************************************************/
  // Build PDE Jacobian trigiagonal matrix
  void compute_pde_jacobian(std::vector<Real> &dl, // Lower diagonal
                            std::vector<Real> &d,  // Diagonal
                            std::vector<Real> &du, // Upper diagonal
                      const std::vector<Real> &u) const { // State variable
    // Get Diagonal and Off-Diagonal Entries of linear PDE Jacobian
    d.clear();
    d.resize(nx_,nu_*2.0/dx_);
    dl.clear();
    dl.resize(nx_-1,-nu_/dx_);
    du.clear();
    du.resize(nx_-1,-nu_/dx_);
    // Contribution from nonlinearity
    for (int i=0; i<nx_; i++) {
      if (i<nx_-1) {
        dl[i] += nl_*(-2.0*u[i]-u[i+1])/6.0;
        d[i]  += nl_*u[i+1]/6.0;
      }
      if (i>0) {
        d[i]    -= nl_*u[i-1]/6.0;
        du[i-1] += nl_*(u[i-1]+2.0*u[i])/6.0;
      }
    }
    // Contribution from Dirichlet boundary conditions
    d[    0] -= nl_*u0_/6.0;
    d[nx_-1] += nl_*u1_/6.0;
  }

  // Apply PDE Jacobian to a vector
  void apply_pde_jacobian(std::vector<Real> &jv,
                    const std::vector<Real> &v,
                    const std::vector<Real> &u,
                    const std::vector<Real> &z) const {
    // Fill jv
    for (int i = 0; i < nx_; i++) {
      jv[i] = nu_/dx_*2.0*v[i];
      if ( i > 0 ) {
        jv[i] += -nu_/dx_*v[i-1]-nl_*(u[i-1]/6.0*v[i]+(u[i]+2.0*u[i-1])/6.0*v[i-1]);
      }
      if ( i < nx_-1 ) {
        jv[i] += -nu_/dx_*v[i+1]+nl_*(u[i+1]/6.0*v[i]+(u[i]+2.0*u[i+1])/6.0*v[i+1]);
      }
    }
    jv[    0] -= nl_*u0_/6.0*v[0];
    jv[nx_-1] += nl_*u1_/6.0*v[nx_-1];
  }

  // Apply inverse PDE Jacobian to a vector
  void apply_inverse_pde_jacobian(std::vector<Real> &ijv,
                            const std::vector<Real> &v,
                            const std::vector<Real> &u,
                            const std::vector<Real> &z) const {
    // Get PDE Jacobian
    std::vector<Real> d(nx_,0.0);
    std::vector<Real> dl(nx_-1,0.0);
    std::vector<Real> du(nx_-1,0.0);
    compute_pde_jacobian(dl,d,du,u);
    // Solve solve state sensitivity system at current time step
    linear_solve(ijv,dl,d,du,v);
  }

  // Apply adjoint PDE Jacobian to a vector
  void apply_adjoint_pde_jacobian(std::vector<Real> &ajv,
                            const std::vector<Real> &v,
                            const std::vector<Real> &u,
                            const std::vector<Real> &z) const {
    // Fill jvp
    for (int i = 0; i < nx_; i++) {
      ajv[i] = nu_/dx_*2.0*v[i];
      if ( i > 0 ) {
        ajv[i] += -nu_/dx_*v[i-1]-nl_*(u[i-1]/6.0*v[i]
                  -(u[i-1]+2.0*u[i])/6.0*v[i-1]);
      }
      if ( i < nx_-1 ) {
        ajv[i] += -nu_/dx_*v[i+1]+nl_*(u[i+1]/6.0*v[i]
                  -(u[i+1]+2.0*u[i])/6.0*v[i+1]);
      }
    }
    ajv[    0] -= nl_*u0_/6.0*v[0];
    ajv[nx_-1] += nl_*u1_/6.0*v[nx_-1];
  }

  // Apply inverse adjoint PDE Jacobian to a vector
  void apply_inverse_adjoint_pde_jacobian(std::vector<Real> &iajv,
                                    const std::vector<Real> &v,
                                    const std::vector<Real> &u,
                                    const std::vector<Real> &z) const {
    // Get PDE Jacobian
    std::vector<Real> d(nx_,0.0);
    std::vector<Real> du(nx_-1,0.0);
    std::vector<Real> dl(nx_-1,0.0);
    compute_pde_jacobian(dl,d,du,u);
    // Solve solve adjoint system at current time step
    linear_solve(iajv,dl,d,du,v,true);
  }

  /***************************************************************************/
  /*********************** CONTROL JACOBIAN FUNCTIONS ************************/
  /***************************************************************************/
  // Apply control Jacobian to a vector
  void apply_control_jacobian(std::vector<Real> &jv,
                        const std::vector<Real> &v,
                        const std::vector<Real> &u,
                        const std::vector<Real> &z) const {
    for (int i=0; i<nx_; i++) {
      // Contribution from control
      jv[i] = -dx_/6.0*(v[i]+4.0*v[i+1]+v[i+2]);
    }
  }

  // Apply adjoint control Jacobian to a vector
  void apply_adjoint_control_jacobian(std::vector<Real> &jv,
                                const std::vector<Real> &v,
                                const std::vector<Real> &u,
                                const std::vector<Real> &z) const {
    for (int i=0; i<nx_+2; i++) {
      if ( i == 0 ) {
        jv[i] = -dx_/6.0*v[i];
      }
      else if ( i == 1 ) {
        jv[i] = -dx_/6.0*(4.0*v[i-1]+v[i]);
      }
      else if ( i == nx_ ) {
        jv[i] = -dx_/6.0*(4.0*v[i-1]+v[i-2]);
      }
      else if ( i == nx_+1 ) {
        jv[i] = -dx_/6.0*v[i-2];
      }
      else {
        jv[i] = -dx_/6.0*(v[i-2]+4.0*v[i-1]+v[i]);
      }
    }
  }

  /***************************************************************************/
  /*********************** AJDOINT HESSIANS **********************************/
  /***************************************************************************/
  void apply_adjoint_pde_hessian(std::vector<Real> &ahwv,
                           const std::vector<Real> &w,
                           const std::vector<Real> &v,
                           const std::vector<Real> &u,
                           const std::vector<Real> &z) const {
    for (int i=0; i<nx_; i++) {
      // Contribution from nonlinear term
      ahwv[i] = 0.0;
      if (i<nx_-1){
        ahwv[i] += (w[i]*v[i+1] - w[i+1]*(2.0*v[i]+v[i+1]))/6.0;
      }
      if (i>0) {
        ahwv[i] += (w[i-1]*(v[i-1]+2.0*v[i]) - w[i]*v[i-1])/6.0;
      }
    }
    //ahwv.assign(u.size(),0.0);
  }
  void apply_adjoint_pde_control_hessian(std::vector<Real> &ahwv,
                                   const std::vector<Real> &w,
                                   const std::vector<Real> &v,
                                   const std::vector<Real> &u,
                                   const std::vector<Real> &z) {
    ahwv.assign(u.size(),0.0);
  }
  void apply_adjoint_control_pde_hessian(std::vector<Real> &ahwv,
                                   const std::vector<Real> &w,
                                   const std::vector<Real> &v,
                                   const std::vector<Real> &u,
                                   const std::vector<Real> &z) {
    ahwv.assign(z.size(),0.0);
  }
  void apply_adjoint_control_hessian(std::vector<Real> &ahwv,
                               const std::vector<Real> &w,
                               const std::vector<Real> &v,
                               const std::vector<Real> &u,
                               const std::vector<Real> &z) {
    ahwv.assign(z.size(),0.0);
  }
};

template<class Real>
class L2VectorPrimal : public ROL::Vector<Real> {
private:
  Teuchos::RCP<std::vector<Real> > vec_;
  Teuchos::RCP<BurgersFEM<Real> > fem_;

  mutable Teuchos::RCP<L2VectorDual<Real> > dual_vec_;

public:
  L2VectorPrimal(const Teuchos::RCP<std::vector<Real> > & vec,
                 const Teuchos::RCP<BurgersFEM<Real> > &fem)
    : vec_(vec), fem_(fem), dual_vec_(Teuchos::null) {}

  void set( const ROL::Vector<Real> &x ) {
    const L2VectorPrimal &ex = Teuchos::dyn_cast<const L2VectorPrimal>(x);
    const std::vector<Real>& xval = *ex.getVector();
    std::copy(xval.begin(),xval.end(),vec_->begin());
  }

  void plus( const ROL::Vector<Real> &x ) {
    const L2VectorPrimal &ex = Teuchos::dyn_cast<const L2VectorPrimal>(x);
    const std::vector<Real>& xval = *ex.getVector();
    unsigned dimension  = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] += xval[i];
    }
  }

  void scale( const Real alpha ) {
    unsigned dimension = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] *= alpha;
    }
  }

  Real dot( const ROL::Vector<Real> &x ) const {
    const L2VectorPrimal & ex = Teuchos::dyn_cast<const L2VectorPrimal>(x);
    const std::vector<Real>& xval = *ex.getVector();
    return fem_->compute_L2_dot(xval,*vec_);
  }

  Real norm() const {
    Real val = 0;
    val = std::sqrt( dot(*this) );
    return val;
  }

  Teuchos::RCP<ROL::Vector<Real> > clone() const {
    return Teuchos::rcp( new L2VectorPrimal( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
  }

  Teuchos::RCP<const std::vector<Real> > getVector() const {
    return vec_;
  }

  Teuchos::RCP<std::vector<Real> > getVector() {
    return vec_;
  }

  Teuchos::RCP<ROL::Vector<Real> > basis( const int i ) const {
    Teuchos::RCP<L2VectorPrimal> e
      = Teuchos::rcp( new L2VectorPrimal( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
    (*e->getVector())[i] = 1.0;
    return e;
  }

  int dimension() const {
    return vec_->size();
  }

  const ROL::Vector<Real>& dual() const {
    dual_vec_ = Teuchos::rcp(new L2VectorDual<Real>(
      Teuchos::rcp(new std::vector<Real>(*vec_)),fem_));

    fem_->apply_mass(*(Teuchos::rcp_const_cast<std::vector<Real> >(dual_vec_->getVector())),*vec_);
    return *dual_vec_;
  }

};

template<class Real>
class L2VectorDual : public ROL::Vector<Real> {
private:
  Teuchos::RCP<std::vector<Real> > vec_;
  Teuchos::RCP<BurgersFEM<Real> > fem_;

  mutable Teuchos::RCP<L2VectorPrimal<Real> > dual_vec_;

public:
  L2VectorDual(const Teuchos::RCP<std::vector<Real> > & vec,
               const Teuchos::RCP<BurgersFEM<Real> > &fem)
    : vec_(vec), fem_(fem), dual_vec_(Teuchos::null) {}

  void set( const ROL::Vector<Real> &x ) {
    const L2VectorDual &ex = Teuchos::dyn_cast<const L2VectorDual>(x);
    const std::vector<Real>& xval = *ex.getVector();
    std::copy(xval.begin(),xval.end(),vec_->begin());
  }

  void plus( const ROL::Vector<Real> &x ) {
    const L2VectorDual &ex = Teuchos::dyn_cast<const L2VectorDual>(x);
    const std::vector<Real>& xval = *ex.getVector();
    unsigned dimension  = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] += xval[i];
    }
  }

  void scale( const Real alpha ) {
    unsigned dimension = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] *= alpha;
    }
  }

  Real dot( const ROL::Vector<Real> &x ) const {
    const L2VectorDual & ex = Teuchos::dyn_cast<const L2VectorDual>(x);
    const std::vector<Real>& xval = *ex.getVector();
    unsigned dimension = vec_->size();
    std::vector<Real> Mx(dimension,0.0);
    fem_->apply_inverse_mass(Mx,xval);
    Real val = 0.0;
    for (unsigned i = 0; i < dimension; i++) {
      val += Mx[i]*(*vec_)[i];
    }
    return val;
  }

  Real norm() const {
    Real val = 0;
    val = std::sqrt( dot(*this) );
    return val;
  }

  Teuchos::RCP<ROL::Vector<Real> > clone() const {
    return Teuchos::rcp( new L2VectorDual( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
  }

  Teuchos::RCP<const std::vector<Real> > getVector() const {
    return vec_;
  }

  Teuchos::RCP<std::vector<Real> > getVector() {
    return vec_;
  }

  Teuchos::RCP<ROL::Vector<Real> > basis( const int i ) const {
    Teuchos::RCP<L2VectorDual> e
      = Teuchos::rcp( new L2VectorDual( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
    (*e->getVector())[i] = 1.0;
    return e;
  }

  int dimension() const {
    return vec_->size();
  }

  const ROL::Vector<Real>& dual() const {
    dual_vec_ = Teuchos::rcp(new L2VectorPrimal<Real>(
      Teuchos::rcp(new std::vector<Real>(*vec_)),fem_));

    fem_->apply_inverse_mass(*(Teuchos::rcp_const_cast<std::vector<Real> >(dual_vec_->getVector())),*vec_);
    return *dual_vec_;
  }

};

template<class Real>
class H1VectorPrimal : public ROL::Vector<Real> {
private:
  Teuchos::RCP<std::vector<Real> > vec_;
  Teuchos::RCP<BurgersFEM<Real> > fem_;

  mutable Teuchos::RCP<H1VectorDual<Real> > dual_vec_;

public:
  H1VectorPrimal(const Teuchos::RCP<std::vector<Real> > & vec,
                 const Teuchos::RCP<BurgersFEM<Real> > &fem)
    : vec_(vec), fem_(fem), dual_vec_(Teuchos::null) {}

  void set( const ROL::Vector<Real> &x ) {
    const H1VectorPrimal &ex = Teuchos::dyn_cast<const H1VectorPrimal>(x);
    const std::vector<Real>& xval = *ex.getVector();
    std::copy(xval.begin(),xval.end(),vec_->begin());
  }

  void plus( const ROL::Vector<Real> &x ) {
    const H1VectorPrimal &ex = Teuchos::dyn_cast<const H1VectorPrimal>(x);
    const std::vector<Real>& xval = *ex.getVector();
    unsigned dimension  = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] += xval[i];
    }
  }

  void scale( const Real alpha ) {
    unsigned dimension = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] *= alpha;
    }
  }

  Real dot( const ROL::Vector<Real> &x ) const {
    const H1VectorPrimal & ex = Teuchos::dyn_cast<const H1VectorPrimal>(x);
    const std::vector<Real>& xval = *ex.getVector();
    return fem_->compute_H1_dot(xval,*vec_);
  }

  Real norm() const {
    Real val = 0;
    val = std::sqrt( dot(*this) );
    return val;
  }

  Teuchos::RCP<ROL::Vector<Real> > clone() const {
    return Teuchos::rcp( new H1VectorPrimal( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
  }

  Teuchos::RCP<const std::vector<Real> > getVector() const {
    return vec_;
  }

  Teuchos::RCP<std::vector<Real> > getVector() {
    return vec_;
  }

  Teuchos::RCP<ROL::Vector<Real> > basis( const int i ) const {
    Teuchos::RCP<H1VectorPrimal> e
      = Teuchos::rcp( new H1VectorPrimal( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
    (*e->getVector())[i] = 1.0;
    return e;
  }

  int dimension() const {
    return vec_->size();
  }

  const ROL::Vector<Real>& dual() const {
    dual_vec_ = Teuchos::rcp(new H1VectorDual<Real>(
      Teuchos::rcp(new std::vector<Real>(*vec_)),fem_));

    fem_->apply_H1(*(Teuchos::rcp_const_cast<std::vector<Real> >(dual_vec_->getVector())),*vec_);
    return *dual_vec_;
  }

};

template<class Real>
class H1VectorDual : public ROL::Vector<Real> {
private:
  Teuchos::RCP<std::vector<Real> > vec_;
  Teuchos::RCP<BurgersFEM<Real> > fem_;

  mutable Teuchos::RCP<H1VectorPrimal<Real> > dual_vec_;

public:
  H1VectorDual(const Teuchos::RCP<std::vector<Real> > & vec,
               const Teuchos::RCP<BurgersFEM<Real> > &fem)
    : vec_(vec), fem_(fem), dual_vec_(Teuchos::null) {}

  void set( const ROL::Vector<Real> &x ) {
    const H1VectorDual &ex = Teuchos::dyn_cast<const H1VectorDual>(x);
    const std::vector<Real>& xval = *ex.getVector();
    std::copy(xval.begin(),xval.end(),vec_->begin());
  }

  void plus( const ROL::Vector<Real> &x ) {
    const H1VectorDual &ex = Teuchos::dyn_cast<const H1VectorDual>(x);
    const std::vector<Real>& xval = *ex.getVector();
    unsigned dimension  = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] += xval[i];
    }
  }

  void scale( const Real alpha ) {
    unsigned dimension = vec_->size();
    for (unsigned i=0; i<dimension; i++) {
      (*vec_)[i] *= alpha;
    }
  }

  Real dot( const ROL::Vector<Real> &x ) const {
    const H1VectorDual & ex = Teuchos::dyn_cast<const H1VectorDual>(x);
    const std::vector<Real>& xval = *ex.getVector();
    unsigned dimension = vec_->size();
    std::vector<Real> Mx(dimension,0.0);
    fem_->apply_inverse_H1(Mx,xval);
    Real val = 0.0;
    for (unsigned i = 0; i < dimension; i++) {
      val += Mx[i]*(*vec_)[i];
    }
    return val;
  }

  Real norm() const {
    Real val = 0;
    val = std::sqrt( dot(*this) );
    return val;
  }

  Teuchos::RCP<ROL::Vector<Real> > clone() const {
    return Teuchos::rcp( new H1VectorDual( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
  }

  Teuchos::RCP<const std::vector<Real> > getVector() const {
    return vec_;
  }

  Teuchos::RCP<std::vector<Real> > getVector() {
    return vec_;
  }

  Teuchos::RCP<ROL::Vector<Real> > basis( const int i ) const {
    Teuchos::RCP<H1VectorDual> e
      = Teuchos::rcp( new H1VectorDual( Teuchos::rcp(new std::vector<Real>(vec_->size(),0.0)),fem_));
    (*e->getVector())[i] = 1.0;
    return e;
  }

  int dimension() const {
    return vec_->size();
  }

  const ROL::Vector<Real>& dual() const {
    dual_vec_ = Teuchos::rcp(new H1VectorPrimal<Real>(
      Teuchos::rcp(new std::vector<Real>(*vec_)),fem_));

    fem_->apply_inverse_H1(*(Teuchos::rcp_const_cast<std::vector<Real> >(dual_vec_->getVector())),*vec_);
    return *dual_vec_;
  }

};

template<class Real>
class EqualityConstraint_BurgersControl : public ROL::EqualityConstraint_SimOpt<Real> {
private:

  typedef H1VectorPrimal<Real> PrimalStateVector;
  typedef H1VectorDual<Real> DualStateVector;
  
  typedef L2VectorPrimal<Real> PrimalControlVector;
  typedef L2VectorDual<Real> DualControlVector;
  
  typedef H1VectorDual<Real> PrimalConstraintVector;
  typedef H1VectorPrimal<Real> DualConstraintVector;

  Teuchos::RCP<BurgersFEM<Real> > fem_;
  bool useHessian_;

public:
  EqualityConstraint_BurgersControl(Teuchos::RCP<BurgersFEM<Real> > &fem, bool useHessian = true)
   : fem_(fem), useHessian_(useHessian) {}

  void value(ROL::Vector<Real> &c, const ROL::Vector<Real> &u, 
                  const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > cp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<PrimalConstraintVector>(c)).getVector());
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->compute_residual(*cp,*up,*zp);
  }

  using ROL::EqualityConstraint_SimOpt<Real>::value;

  void solve(ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > up =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<PrimalStateVector>(u)).getVector());
    up->assign(up->size(),z.norm()/up->size());
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->solve(*up,*zp);
  }

  void applyJacobian_1(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                       const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<PrimalConstraintVector>(jv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->apply_pde_jacobian(*jvp,*vp,*up,*zp);
  }

  void applyJacobian_2(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                       const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<PrimalConstraintVector>(jv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->apply_control_jacobian(*jvp,*vp,*up,*zp);
  }

  void applyInverseJacobian_1(ROL::Vector<Real> &ijv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > ijvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<PrimalStateVector>(ijv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<PrimalConstraintVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->apply_inverse_pde_jacobian(*ijvp,*vp,*up,*zp);
  }

  void applyAdjointJacobian_1(ROL::Vector<Real> &ajv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                              const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualStateVector>(ajv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<DualConstraintVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->apply_adjoint_pde_jacobian(*jvp,*vp,*up,*zp);
  }

  void applyAdjointJacobian_2(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualControlVector>(jv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<DualConstraintVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->apply_adjoint_control_jacobian(*jvp,*vp,*up,*zp);
  }

  void applyInverseAdjointJacobian_1(ROL::Vector<Real> &iajv, const ROL::Vector<Real> &v,
                                     const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > iajvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualConstraintVector>(iajv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<DualStateVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    fem_->apply_inverse_adjoint_pde_jacobian(*iajvp,*vp,*up,*zp);
  }

  void applyAdjointHessian_11(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    if ( useHessian_ ) {
      Teuchos::RCP<std::vector<Real> > ahwvp =
        Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualStateVector>(ahwv)).getVector());
      Teuchos::RCP<const std::vector<Real> > wp =
        (Teuchos::dyn_cast<DualConstraintVector>(const_cast<ROL::Vector<Real> &>(w))).getVector();
      Teuchos::RCP<const std::vector<Real> > vp =
        (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
      Teuchos::RCP<const std::vector<Real> > up =
        (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
      Teuchos::RCP<const std::vector<Real> > zp =
        (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
      fem_->apply_adjoint_pde_hessian(*ahwvp,*wp,*vp,*up,*zp);
    }
    else {
      ahwv.zero();
    }
  }
  
  void applyAdjointHessian_12(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    if ( useHessian_ ) {
      Teuchos::RCP<std::vector<Real> > ahwvp =
        Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualControlVector>(ahwv)).getVector());
      Teuchos::RCP<const std::vector<Real> > wp =
        (Teuchos::dyn_cast<DualConstraintVector>(const_cast<ROL::Vector<Real> &>(w))).getVector();
      Teuchos::RCP<const std::vector<Real> > vp =
        (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
      Teuchos::RCP<const std::vector<Real> > up =
        (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
      Teuchos::RCP<const std::vector<Real> > zp =
        (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
      fem_->apply_adjoint_control_pde_hessian(*ahwvp,*wp,*vp,*up,*zp);
    }
    else {
      ahwv.zero();
    }
  }
  void applyAdjointHessian_21(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    if ( useHessian_ ) {
      Teuchos::RCP<std::vector<Real> > ahwvp =
        Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualStateVector>(ahwv)).getVector());
      Teuchos::RCP<const std::vector<Real> > wp =
        (Teuchos::dyn_cast<DualConstraintVector>(const_cast<ROL::Vector<Real> &>(w))).getVector();
      Teuchos::RCP<const std::vector<Real> > vp =
        (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
      Teuchos::RCP<const std::vector<Real> > up =
        (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
      Teuchos::RCP<const std::vector<Real> > zp =
        (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
      fem_->apply_adjoint_pde_control_hessian(*ahwvp,*wp,*vp,*up,*zp);
    }
    else {
      ahwv.zero();
    }
  }
  void applyAdjointHessian_22(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    if ( useHessian_ ) {
      Teuchos::RCP<std::vector<Real> > ahwvp =
        Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualControlVector>(ahwv)).getVector());
      Teuchos::RCP<const std::vector<Real> > wp =
        (Teuchos::dyn_cast<DualConstraintVector>(const_cast<ROL::Vector<Real> &>(w))).getVector();
      Teuchos::RCP<const std::vector<Real> > vp =
        (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();
      Teuchos::RCP<const std::vector<Real> > up =
        (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
      Teuchos::RCP<const std::vector<Real> > zp =
        (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
      fem_->apply_adjoint_control_hessian(*ahwvp,*wp,*vp,*up,*zp);
    }
    else {
      ahwv.zero();
    }
  }
};

template<class Real>
class Objective_BurgersControl : public ROL::Objective_SimOpt<Real> {
private:

  typedef H1VectorPrimal<Real> PrimalStateVector;
  typedef H1VectorDual<Real> DualStateVector;
  
  typedef L2VectorPrimal<Real> PrimalControlVector;
  typedef L2VectorDual<Real> DualControlVector;

  Real alpha_; // Penalty Parameter
  Teuchos::RCP<BurgersFEM<Real> > fem_;
  Teuchos::RCP<ROL::Vector<Real> > ud_;
  Teuchos::RCP<ROL::Vector<Real> > diff_;

public:
  Objective_BurgersControl(const Teuchos::RCP<BurgersFEM<Real> > &fem, 
                           const Teuchos::RCP<ROL::Vector<Real> > &ud,
                           Real alpha = 1.e-4) : alpha_(alpha), fem_(fem), ud_(ud) {
    diff_ = ud_->clone();
  }

  Real value( const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();
    Teuchos::RCP<const std::vector<Real> > udp =
      (Teuchos::dyn_cast<L2VectorPrimal<Real> >(const_cast<ROL::Vector<Real> &>(*ud_))).getVector();

    std::vector<Real> diff(udp->size(),0.0);
    for (unsigned i = 0; i < udp->size(); i++) {
      diff[i] = (*up)[i] - (*udp)[i];
    }
    return 0.5*(fem_->compute_L2_dot(diff,diff) + alpha_*fem_->compute_L2_dot(*zp,*zp));
  }

  void gradient_1( ROL::Vector<Real> &g, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<std::vector<Real> > gp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualStateVector>(g)).getVector());
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > udp =
      (Teuchos::dyn_cast<L2VectorPrimal<Real> >(const_cast<ROL::Vector<Real> &>(*ud_))).getVector();

    std::vector<Real> diff(udp->size(),0.0);
    for (unsigned i = 0; i < udp->size(); i++) {
      diff[i] = (*up)[i] - (*udp)[i];
    }
    fem_->apply_mass(*gp,diff);
  }

  void gradient_2( ROL::Vector<Real> &g, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<std::vector<Real> > gp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualControlVector>(g)).getVector());
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(z))).getVector();

    fem_->apply_mass(*gp,*zp);
    for (unsigned i = 0; i < zp->size(); i++) {
      (*gp)[i] *= alpha_;
    }
  }

  void hessVec_11( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<std::vector<Real> > hvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualStateVector>(hv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<PrimalStateVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();

    fem_->apply_mass(*hvp,*vp);
  }

  void hessVec_12( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    hv.zero();
  }

  void hessVec_21( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    hv.zero();
  }

  void hessVec_22( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<std::vector<Real> > hvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<DualControlVector>(hv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<PrimalControlVector>(const_cast<ROL::Vector<Real> &>(v))).getVector();

    fem_->apply_mass(*hvp,*vp);
    for (unsigned i = 0; i < vp->size(); i++) {
      (*hvp)[i] *= alpha_;
    }
  }
};

template<class Real>
class L2BoundConstraint : public ROL::BoundConstraint<Real> {
private:
  int dim_;
  std::vector<Real> x_lo_;
  std::vector<Real> x_up_;
  Real min_diff_;
  Real scale_;
  Teuchos::RCP<BurgersFEM<Real> > fem_;

  void cast_vector(Teuchos::RCP<std::vector<Real> > &xvec,
                   ROL::Vector<Real> &x) const {
    try {
      xvec = Teuchos::rcp_const_cast<std::vector<Real> >(
               (Teuchos::dyn_cast<L2VectorPrimal<Real> >(x)).getVector());
    }
    catch (std::exception &e) {
      xvec = Teuchos::rcp_const_cast<std::vector<Real> >(
               (Teuchos::dyn_cast<L2VectorDual<Real> >(x)).getVector());
    }
  }

  void cast_const_vector(Teuchos::RCP<const std::vector<Real> > &xvec,
                   const ROL::Vector<Real> &x) const {
    try {
      xvec = (Teuchos::dyn_cast<L2VectorPrimal<Real> >(
               const_cast<ROL::Vector<Real> &>(x))).getVector();
    }
    catch (std::exception &e) {
      xvec = (Teuchos::dyn_cast<L2VectorDual<Real> >(
               const_cast<ROL::Vector<Real> &>(x))).getVector();
    }
  }

  void axpy(std::vector<Real> &out, const Real a,
      const std::vector<Real> &x, const std::vector<Real> &y) const{
    out.resize(dim_,0.0);
    for (unsigned i = 0; i < dim_; i++) {
      out[i] = a*x[i] + y[i];
    }
  }

  void projection(std::vector<Real> &x) {
    for ( int i = 0; i < dim_; i++ ) {
      x[i] = std::max(x_lo_[i],std::min(x_up_[i],x[i]));
    }
  }

public:
  L2BoundConstraint(std::vector<Real> &l, std::vector<Real> &u,
              const Teuchos::RCP<BurgersFEM<Real> > &fem, Real scale = 1.0)
    : x_lo_(l), x_up_(u), scale_(scale), fem_(fem) {
    dim_ = x_lo_.size();
    for ( int i = 0; i < dim_; i++ ) {
      if ( i == 0 ) {
        min_diff_ = x_up_[i] - x_lo_[i];
      }
      else {
        min_diff_ = ( (min_diff_ < (x_up_[i] - x_lo_[i])) ? min_diff_ : (x_up_[i] - x_lo_[i]) );
      }
    }
    min_diff_ *= 0.5;
  }

  bool isFeasible( const ROL::Vector<Real> &x ) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    bool val = true;
    int  cnt = 1;
    for ( int i = 0; i < dim_; i++ ) {
      if ( (*ex)[i] >= x_lo_[i] && (*ex)[i] <= x_up_[i] ) { cnt *= 1; }
      else                                                { cnt *= 0; }
    }
    if ( cnt == 0 ) { val = false; }
    return val;
  }

  void project( ROL::Vector<Real> &x ) {
    Teuchos::RCP<std::vector<Real> > ex; cast_vector(ex,x);
    projection(*ex);
  }

  void pruneLowerActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneUpperActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] >= x_up_[i]-epsn) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn) ||
           ((*ex)[i] >= x_up_[i]-epsn) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneLowerActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &g, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<const std::vector<Real> > eg; cast_const_vector(eg,g);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn && (*eg)[i] > 0.0) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneUpperActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &g, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<const std::vector<Real> > eg; cast_const_vector(eg,g);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] >= x_up_[i]-epsn && (*eg)[i] < 0.0) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &g, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<const std::vector<Real> > eg; cast_const_vector(eg,g);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn && (*eg)[i] > 0.0) ||
           ((*ex)[i] >= x_up_[i]-epsn && (*eg)[i] < 0.0) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void setVectorToUpperBound( ROL::Vector<Real> &u ) {
    Teuchos::RCP<std::vector<Real> > us = Teuchos::rcp( new std::vector<Real>(dim_,0.0) );
    us->assign(x_up_.begin(),x_up_.end());
    Teuchos::RCP<ROL::Vector<Real> > up = Teuchos::rcp( new L2VectorPrimal<Real>(us,fem_) );
    u.set(*up);
  }

  void setVectorToLowerBound( ROL::Vector<Real> &l ) {
    Teuchos::RCP<std::vector<Real> > ls = Teuchos::rcp( new std::vector<Real>(dim_,0.0) );
    ls->assign(x_lo_.begin(),x_lo_.end());
    Teuchos::RCP<ROL::Vector<Real> > lp = Teuchos::rcp( new L2VectorPrimal<Real>(ls,fem_) );
    l.set(*lp);
  }
};

template<class Real>
class H1BoundConstraint : public ROL::BoundConstraint<Real> {
private:
  int dim_;
  std::vector<Real> x_lo_;
  std::vector<Real> x_up_;
  Real min_diff_;
  Real scale_;
  Teuchos::RCP<BurgersFEM<Real> > fem_;

  void cast_vector(Teuchos::RCP<std::vector<Real> > &xvec,
                   ROL::Vector<Real> &x) const {
    try {
      xvec = Teuchos::rcp_const_cast<std::vector<Real> >(
               (Teuchos::dyn_cast<H1VectorPrimal<Real> >(x)).getVector());
    }
    catch (std::exception &e) {
      xvec = Teuchos::rcp_const_cast<std::vector<Real> >(
               (Teuchos::dyn_cast<H1VectorDual<Real> >(x)).getVector());
    }
  }

  void cast_const_vector(Teuchos::RCP<const std::vector<Real> > &xvec,
                   const ROL::Vector<Real> &x) const {
    try {
      xvec = (Teuchos::dyn_cast<H1VectorPrimal<Real> >(
               const_cast<ROL::Vector<Real> &>(x))).getVector();
    }
    catch (std::exception &e) {
      xvec = (Teuchos::dyn_cast<H1VectorDual<Real> >(
               const_cast<ROL::Vector<Real> &>(x))).getVector();
    }
  }

  void axpy(std::vector<Real> &out, const Real a,
      const std::vector<Real> &x, const std::vector<Real> &y) const{
    out.resize(dim_,0.0);
    for (unsigned i = 0; i < dim_; i++) {
      out[i] = a*x[i] + y[i];
    }
  }

  void projection(std::vector<Real> &x) {
    for ( int i = 0; i < dim_; i++ ) {
      x[i] = std::max(x_lo_[i],std::min(x_up_[i],x[i]));
    }
  }

public:
  H1BoundConstraint(std::vector<Real> &l, std::vector<Real> &u,
              const Teuchos::RCP<BurgersFEM<Real> > &fem, Real scale = 1.0)
    : x_lo_(l), x_up_(u), scale_(scale), fem_(fem) {
    dim_ = x_lo_.size();
    for ( int i = 0; i < dim_; i++ ) {
      if ( i == 0 ) {
        min_diff_ = x_up_[i] - x_lo_[i];
      }
      else {
        min_diff_ = ( (min_diff_ < (x_up_[i] - x_lo_[i])) ? min_diff_ : (x_up_[i] - x_lo_[i]) );
      }
    }
    min_diff_ *= 0.5;
  }

  bool isFeasible( const ROL::Vector<Real> &x ) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    bool val = true;
    int  cnt = 1;
    for ( int i = 0; i < dim_; i++ ) {
      if ( (*ex)[i] >= x_lo_[i] && (*ex)[i] <= x_up_[i] ) { cnt *= 1; }
      else                                                { cnt *= 0; }
    }
    if ( cnt == 0 ) { val = false; }
    return val;
  }

  void project( ROL::Vector<Real> &x ) {
    Teuchos::RCP<std::vector<Real> > ex; cast_vector(ex,x);
    projection(*ex);
  }

  void pruneLowerActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneUpperActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] >= x_up_[i]-epsn) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn) ||
           ((*ex)[i] >= x_up_[i]-epsn) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneLowerActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &g, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<const std::vector<Real> > eg; cast_const_vector(eg,g);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn && (*eg)[i] > 0.0) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneUpperActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &g, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<const std::vector<Real> > eg; cast_const_vector(eg,g);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] >= x_up_[i]-epsn && (*eg)[i] < 0.0) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void pruneActive(ROL::Vector<Real> &v, const ROL::Vector<Real> &g, const ROL::Vector<Real> &x, Real eps) {
    Teuchos::RCP<const std::vector<Real> > ex; cast_const_vector(ex,x);
    Teuchos::RCP<const std::vector<Real> > eg; cast_const_vector(eg,g);
    Teuchos::RCP<std::vector<Real> > ev; cast_vector(ev,v);
    Real epsn = std::min(scale_*eps,min_diff_);
    for ( int i = 0; i < dim_; i++ ) {
      if ( ((*ex)[i] <= x_lo_[i]+epsn && (*eg)[i] > 0.0) ||
           ((*ex)[i] >= x_up_[i]-epsn && (*eg)[i] < 0.0) ) {
        (*ev)[i] = 0.0;
      }
    }
  }

  void setVectorToUpperBound( ROL::Vector<Real> &u ) {
    Teuchos::RCP<std::vector<Real> > us = Teuchos::rcp( new std::vector<Real>(dim_,0.0) );
    us->assign(x_up_.begin(),x_up_.end());
    Teuchos::RCP<ROL::Vector<Real> > up = Teuchos::rcp( new H1VectorPrimal<Real>(us,fem_) );
    u.set(*up);
  }

  void setVectorToLowerBound( ROL::Vector<Real> &l ) {
    Teuchos::RCP<std::vector<Real> > ls = Teuchos::rcp( new std::vector<Real>(dim_,0.0) );
    ls->assign(x_lo_.begin(),x_lo_.end());
    Teuchos::RCP<ROL::Vector<Real> > lp = Teuchos::rcp( new H1VectorPrimal<Real>(ls,fem_) );
    l.set(*lp);
  }
};
