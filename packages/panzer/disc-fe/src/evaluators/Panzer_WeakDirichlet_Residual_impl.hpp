// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
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
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#ifndef PANZER_WEAKDIRICHLET_RESIDUAL_IMPL_HPP
#define PANZER_WEAKDIRICHLET_RESIDUAL_IMPL_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "Panzer_BasisIRLayout.hpp"
#include "Panzer_Workset_Utilities.hpp"
#include "Intrepid_FunctionSpaceTools.hpp"
#include "Teuchos_RCP.hpp"

namespace panzer {

//**********************************************************************
PHX_EVALUATOR_CTOR(WeakDirichletResidual,p)
{
  std::string residual_name = p.get<std::string>("Residual Name");
  std::string flux_name = p.get<std::string>("Flux Name");
  std::string normal_name = p.get<std::string>("Normal Name");
  std::string normal_dot_flux_name = normal_name + " dot " + flux_name;
  std::string dof_name = p.get<std::string>("DOF Name"); 
  std::string value_name = p.get<std::string>("Value Name");
  std::string sigma_name = p.get<std::string>("Sigma Name");
  
  const Teuchos::RCP<const panzer::PureBasis> basis =
    p.get< Teuchos::RCP<const panzer::PureBasis> >("Basis");

  const Teuchos::RCP<const panzer::IntegrationRule> ir = 
    p.get< Teuchos::RCP<const panzer::IntegrationRule> >("IR");


  residual = PHX::MDField<ScalarT>(residual_name, basis->functional);
  normal_dot_flux_plus_pen = PHX::MDField<ScalarT>(normal_dot_flux_name, ir->dl_scalar);
  flux = PHX::MDField<ScalarT>(flux_name, ir->dl_vector);
  normal = PHX::MDField<ScalarT>(normal_name, ir->dl_vector);
  dof = PHX::MDField<ScalarT>(dof_name, ir->dl_scalar);
  value = PHX::MDField<ScalarT>(value_name, ir->dl_scalar);
  sigma = PHX::MDField<ScalarT>(sigma_name, ir->dl_scalar);

  this->addEvaluatedField(residual);
  this->addEvaluatedField(normal_dot_flux_plus_pen);
  this->addDependentField(normal);
  this->addDependentField(flux);
  this->addDependentField(dof);
  this->addDependentField(value);
  this->addDependentField(sigma);
 
  basis_name = panzer::basisIRLayout(basis,*ir)->name();

  std::string n = "Weak Dirichlet Residual Evaluator";
  this->setName(n);
}

//**********************************************************************
PHX_POST_REGISTRATION_SETUP(WeakDirichletResidual,sd,fm)
{
  this->utils.setFieldData(residual,fm);
  this->utils.setFieldData(normal_dot_flux_plus_pen,fm);
  this->utils.setFieldData(flux,fm);
  this->utils.setFieldData(normal,fm);
  this->utils.setFieldData(dof,fm);
  this->utils.setFieldData(value,fm);
  this->utils.setFieldData(sigma,fm);

  num_ip = flux.dimension(1);
  num_dim = flux.dimension(2);

  TEUCHOS_ASSERT(flux.dimension(1) == normal.dimension(1));
  TEUCHOS_ASSERT(flux.dimension(2) == normal.dimension(2));

  basis_index = panzer::getBasisIndex(basis_name, (*sd.worksets_)[0], this->wda);
}

//**********************************************************************
PHX_EVALUATE_FIELDS(WeakDirichletResidual,workset)
{ 
  for (std::size_t cell = 0; cell < workset.num_cells; ++cell) {
    for (std::size_t ip = 0; ip < num_ip; ++ip) {
      normal_dot_flux_plus_pen(cell,ip) = ScalarT(0.0);
      for (std::size_t dim = 0; dim < num_dim; ++dim) {
	normal_dot_flux_plus_pen(cell,ip) += normal(cell,ip,dim) * flux(cell,ip,dim);
      }
      normal_dot_flux_plus_pen(cell,ip) += sigma(cell,ip) * (dof(cell,ip) - value(cell,ip)); 
    }
  }

  if(workset.num_cells>0)
    Intrepid::FunctionSpaceTools::
      integrate<ScalarT>(residual, normal_dot_flux_plus_pen, 
			 (this->wda(workset).bases[basis_index])->weighted_basis_scalar, 
			 Intrepid::COMP_BLAS);
  
}

//**********************************************************************

}

#endif
