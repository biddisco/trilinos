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

#ifndef PANZER_SCATTER_RESIDUAL_EPETRA_SG_IMPL_HPP
#define PANZER_SCATTER_RESIDUAL_EPETRA_SG_IMPL_HPP

#include "Panzer_config.hpp"
#ifdef HAVE_STOKHOS

#include "Panzer_SGEpetraLinearObjContainer.hpp"
#include "Panzer_PtrFromStlVector.hpp"
#include "Phalanx_DataLayout_MDALayout.hpp"

#include "Epetra_Vector.h"
#include "Epetra_Map.h"
#include "Epetra_CrsMatrix.h"

// **********************************************************************
// Specialization: SGResidual
// **********************************************************************

template<typename TRAITS,typename LO,typename GO>
panzer::ScatterResidual_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
ScatterResidual_Epetra(const Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > & indexer,
                       const Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > & cIndexer,
                       const Teuchos::ParameterList& p,bool useDiscreteAdjoint)
  : globalIndexer_(indexer)
  , globalDataKey_("Residual Scatter Container")
  , useDiscreteAdjoint_(useDiscreteAdjoint)
{
  TEUCHOS_ASSERT(cIndexer==Teuchos::null);

  std::string scatterName = p.get<std::string>("Scatter Name");
  scatterHolder_ =
    Teuchos::rcp(new PHX::Tag<ScalarT>(scatterName,Teuchos::rcp(new PHX::MDALayout<Dummy>(0))));

  // get names to be evaluated
  const std::vector<std::string>& names =
    *(p.get< Teuchos::RCP< std::vector<std::string> > >("Dependent Names"));

  // grab map from evaluated names to field names
  fieldMap_ = p.get< Teuchos::RCP< std::map<std::string,std::string> > >("Dependent Map");

  Teuchos::RCP<PHX::DataLayout> dl =
    p.get< Teuchos::RCP<const panzer::PureBasis> >("Basis")->functional;

  // build the vector of fields that this is dependent on
  scatterFields_.resize(names.size());
  for (std::size_t eq = 0; eq < names.size(); ++eq) {
    scatterFields_[eq] = PHX::MDField<ScalarT,Cell,NODE>(names[eq],dl);

    // tell the field manager that we depend on this field
    this->addDependentField(scatterFields_[eq]);
  }

  // this is what this evaluator provides
  this->addEvaluatedField(*scatterHolder_);

  if (p.isType<std::string>("Global Data Key"))
     globalDataKey_ = p.get<std::string>("Global Data Key");

  this->setName(scatterName+" Scatter Residual (SGResidual)");
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::ScatterResidual_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
preEvaluate(typename TRAITS::PreEvalData d)
{
  // extract linear object container
  sgEpetraContainer_ = Teuchos::rcp_dynamic_cast<SGEpetraLinearObjContainer>(d.gedc.getDataObject(globalDataKey_));

  TEUCHOS_ASSERT(sgEpetraContainer_!=Teuchos::null);
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::ScatterResidual_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
postRegistrationSetup(typename TRAITS::SetupData d,
		      PHX::FieldManager<TRAITS>& fm)
{
  fieldIds_.resize(scatterFields_.size());
  // load required field numbers for fast use
  for(std::size_t fd=0;fd<scatterFields_.size();++fd) {
    // get field ID from DOF manager
    std::string fieldName = fieldMap_->find(scatterFields_[fd].fieldTag().name())->second;
    fieldIds_[fd] = globalIndexer_->getFieldNum(fieldName);

    // fill field data object
    this->utils.setFieldData(scatterFields_[fd],fm);
  }
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::ScatterResidual_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
evaluateFields(typename TRAITS::EvalData workset)
{
   std::vector<GO> GIDs;
   std::vector<int> LIDs;

   // for convenience pull out some objects from workset
   std::string blockId = this->wda(workset).block_id;
   const std::vector<std::size_t> & localCellIds = this->wda(workset).cell_local_ids;

   Teuchos::RCP<SGEpetraLinearObjContainer> sgEpetraContainer = sgEpetraContainer_;
   Teuchos::RCP<Epetra_Vector> r_template = (*sgEpetraContainer->begin())->get_f();
   const Epetra_BlockMap & map = r_template->Map();

   // NOTE: A reordering of these loops will likely improve performance
   //       The "getGIDFieldOffsets may be expensive.  However the
   //       "getElementGIDs" can be cheaper. However the lookup for LIDs
   //       may be more expensive!

   // scatter operation for each cell in workset
   for(std::size_t worksetCellIndex=0;worksetCellIndex<localCellIds.size();++worksetCellIndex) {
      std::size_t cellLocalId = localCellIds[worksetCellIndex];

      globalIndexer_->getElementGIDs(cellLocalId,GIDs,blockId);

      // caculate the local IDs for this element
      LIDs.resize(GIDs.size());
      for(std::size_t i=0;i<GIDs.size();i++)
         LIDs[i] = map.LID(GIDs[i]);

      // loop over each field to be scattered
      for (std::size_t fieldIndex = 0; fieldIndex < scatterFields_.size(); fieldIndex++) {
         int fieldNum = fieldIds_[fieldIndex];
         const std::vector<int> & elmtOffset = globalIndexer_->getGIDFieldOffsets(blockId,fieldNum);

         // loop over basis functions
         for(std::size_t basis=0;basis<elmtOffset.size();basis++) {
            int offset = elmtOffset[basis];
            int lid = LIDs[offset];

            const ScalarT field = (scatterFields_[fieldIndex])(worksetCellIndex,basis);

            // loop over stochastic basis scatter field values to residual vectors
            int stochIndex = 0;
            panzer::SGEpetraLinearObjContainer::iterator itr;
            for(itr=sgEpetraContainer->begin();itr!=sgEpetraContainer->end();++itr,++stochIndex)
               (*(*itr)->get_f())[lid] += field.coeff(stochIndex);
         }
      }
   }
}

// **********************************************************************
// Specialization: SGJacobian
// **********************************************************************

template<typename TRAITS,typename LO,typename GO>
panzer::ScatterResidual_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
ScatterResidual_Epetra(const Teuchos::RCP<const UniqueGlobalIndexer<LO,GO> > & indexer,
                       const Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > & cIndexer,
                       const Teuchos::ParameterList& p,bool useDiscreteAdjoint)
   : globalIndexer_(indexer)
   , globalDataKey_("Residual Scatter Container")
   , useDiscreteAdjoint_(useDiscreteAdjoint)
{
  TEUCHOS_ASSERT(cIndexer==Teuchos::null);

  std::string scatterName = p.get<std::string>("Scatter Name");
  scatterHolder_ =
    Teuchos::rcp(new PHX::Tag<ScalarT>(scatterName,Teuchos::rcp(new PHX::MDALayout<Dummy>(0))));

  // get names to be evaluated
  const std::vector<std::string>& names =
    *(p.get< Teuchos::RCP< std::vector<std::string> > >("Dependent Names"));

  // grab map from evaluated names to field names
  fieldMap_ = p.get< Teuchos::RCP< std::map<std::string,std::string> > >("Dependent Map");

  Teuchos::RCP<PHX::DataLayout> dl =
    p.get< Teuchos::RCP<const panzer::PureBasis> >("Basis")->functional;

  // build the vector of fields that this is dependent on
  scatterFields_.resize(names.size());
  for (std::size_t eq = 0; eq < names.size(); ++eq) {
    scatterFields_[eq] = PHX::MDField<ScalarT,Cell,NODE>(names[eq],dl);

    // tell the field manager that we depend on this field
    this->addDependentField(scatterFields_[eq]);
  }

  // this is what this evaluator provides
  this->addEvaluatedField(*scatterHolder_);

  if (p.isType<std::string>("Global Data Key"))
     globalDataKey_ = p.get<std::string>("Global Data Key");
  if (p.isType<bool>("Use Discrete Adjoint"))
     useDiscreteAdjoint = p.get<bool>("Use Discrete Adjoint");

  TEUCHOS_ASSERT(cIndexer==Teuchos::null); // SG does not work with a cIndexer!

  // discrete adjoint does not work with non-square matrices
  if(useDiscreteAdjoint)
  { TEUCHOS_ASSERT(cIndexer==Teuchos::null); }

  this->setName(scatterName+" Scatter Residual (SGJacobian)");
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::ScatterResidual_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
preEvaluate(typename TRAITS::PreEvalData d)
{
  // extract linear object container
  sgEpetraContainer_ = Teuchos::rcp_dynamic_cast<SGEpetraLinearObjContainer>(d.gedc.getDataObject(globalDataKey_));

  TEUCHOS_ASSERT(sgEpetraContainer_!=Teuchos::null);
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::ScatterResidual_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
postRegistrationSetup(typename TRAITS::SetupData d,
		      PHX::FieldManager<TRAITS>& fm)
{
  fieldIds_.resize(scatterFields_.size());
  // load required field numbers for fast use
  for(std::size_t fd=0;fd<scatterFields_.size();++fd) {
    // get field ID from DOF manager
    std::string fieldName = fieldMap_->find(scatterFields_[fd].fieldTag().name())->second;
    fieldIds_[fd] = globalIndexer_->getFieldNum(fieldName);

    // fill field data object
    this->utils.setFieldData(scatterFields_[fd],fm);
  }
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::ScatterResidual_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
evaluateFields(typename TRAITS::EvalData workset)
{
   std::vector<int> rLIDs, cLIDs;
   std::vector<double> jacRow;

   // for convenience pull out some objects from workset
   std::string blockId = this->wda(workset).block_id;
   const std::vector<std::size_t> & localCellIds = this->wda(workset).cell_local_ids;

   Teuchos::RCP<SGEpetraLinearObjContainer> sgEpetraContainer = sgEpetraContainer_;
   Teuchos::RCP<Epetra_CrsMatrix> Jac_template = (*sgEpetraContainer->begin())->get_A();

   // NOTE: A reordering of these loops will likely improve performance
   //       The "getGIDFieldOffsets" may be expensive.  However the
   //       "getElementGIDs" can be cheaper. However the lookup for LIDs
   //       may be more expensive!

   // scatter operation for each cell in workset
   for(std::size_t worksetCellIndex=0;worksetCellIndex<localCellIds.size();++worksetCellIndex) {
      std::size_t cellLocalId = localCellIds[worksetCellIndex];

      rLIDs = globalIndexer_->getElementLIDs(cellLocalId);
      cLIDs = rLIDs;

      // loop over each field to be scattered
      for(std::size_t fieldIndex = 0; fieldIndex < scatterFields_.size(); fieldIndex++) {
         int fieldNum = fieldIds_[fieldIndex];
         const std::vector<int> & elmtOffset = globalIndexer_->getGIDFieldOffsets(blockId,fieldNum);

         // loop over the basis functions (currently they are nodes)
         for(std::size_t rowBasisNum = 0; rowBasisNum < elmtOffset.size(); rowBasisNum++) {
            const ScalarT scatterField = (scatterFields_[fieldIndex])(worksetCellIndex,rowBasisNum);
            int rowOffset = elmtOffset[rowBasisNum];
            int row = rLIDs[rowOffset];

            // loop over stochastic basis scatter field values to residual vectors
            int stochIndex = 0;
            panzer::SGEpetraLinearObjContainer::iterator itr;
            for(itr=sgEpetraContainer->begin();itr!=sgEpetraContainer->end();++itr,++stochIndex) {
               Teuchos::RCP<Epetra_Vector> r = (*itr)->get_f();
               Teuchos::RCP<Epetra_CrsMatrix> Jac = (*itr)->get_A();

               // Sum residual
               if(r!=Teuchos::null)
       	          r->SumIntoMyValue(row,0,scatterField.val().coeff(stochIndex));

               // loop over the sensitivity indices: all DOFs on a cell
               jacRow.resize(scatterField.size());

               if(useDiscreteAdjoint_) {
                 // loop over the sensitivity indices: all DOFs on a cell
                 jacRow.resize(scatterField.size());

                 for(int sensIndex=0;sensIndex<scatterField.size();++sensIndex)
                    jacRow[sensIndex] = scatterField.fastAccessDx(sensIndex).coeff(stochIndex);

                 int err = 0;
                 for(std::size_t c=0;c<cLIDs.size();c++)
                    err += std::abs(Jac->SumIntoMyValues(cLIDs[c], 1, &jacRow[c],&row));
                 TEUCHOS_ASSERT_EQUALITY(err,0); // do the check only once!
               }
               else {
                 for(int sensIndex=0;sensIndex<scatterField.size();++sensIndex)
                    jacRow[sensIndex] = scatterField.fastAccessDx(sensIndex).coeff(stochIndex);

                 // Sum SGJacobian
                 int err = Jac->SumIntoMyValues(row, scatterField.size(), &jacRow[0],
	  				        panzer::ptrFromStlVector(cLIDs));
                 TEUCHOS_ASSERT_EQUALITY(err,0);
               }
            }
         } // end rowBasisNum
      } // end fieldIndex
   }
}

// **********************************************************************

#endif

#endif
