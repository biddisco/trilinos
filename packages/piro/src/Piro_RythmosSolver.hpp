// @HEADER
// ************************************************************************
//
//        Piro: Strategy package for embedded analysis capabilitites
//                  Copyright (2010) Sandia Corporation
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
// Questions? Contact Andy Salinger (agsalin@sandia.gov), Sandia
// National Laboratories.
//
// ************************************************************************
// @HEADER

#ifndef PIRO_RYTHMOSSOLVER_H
#define PIRO_RYTHMOSSOLVER_H

#include "Thyra_ResponseOnlyModelEvaluatorBase.hpp"

#include "Rythmos_DefaultIntegrator.hpp"
#include "Rythmos_IntegrationObserverBase.hpp"
#include "Rythmos_TimeStepNonlinearSolver.hpp"

#include "Piro_config.hpp"
#include "Piro_ObserverBase.hpp"

#include "Piro_RythmosStepperFactory.hpp"
#include "Piro_RythmosStepControlFactory.hpp"

#include <map>
#include <string>

namespace Piro {

/** \brief Thyra-based Model Evaluator for Rythmos solves
 *  \ingroup Piro_Thyra_solver_grp
 * */
template <typename Scalar>
class RythmosSolver
    : public Thyra::ResponseOnlyModelEvaluatorBase<Scalar>
{
public:
  /** \name Constructors/initializers */
  //@{

  /** \brief Initializes the internals, though the object is a blank slate. To initialize it call <code>initialize</code> */
  RythmosSolver();

  /** \brief Initialize with internally built objects according to the given parameter list. */
  RythmosSolver(
      const Teuchos::RCP<Teuchos::ParameterList> &appParams,
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &model,
      const Teuchos::RCP<Rythmos::IntegrationObserverBase<Scalar> > &observer = Teuchos::null);

  /** \brief Initialize using prebuilt objects. */
  RythmosSolver(
      const Teuchos::RCP<Rythmos::DefaultIntegrator<Scalar> > &stateIntegrator,
      const Teuchos::RCP<Rythmos::StepperBase<Scalar> > &stateStepper,
      const Teuchos::RCP<Thyra::NonlinearSolverBase<Scalar> > &timeStepSolver,
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &model,
      Scalar finalTime,
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &initialConditionModel = Teuchos::null,
      Teuchos::EVerbosityLevel verbosityLevel = Teuchos::VERB_DEFAULT);
  //@}

  /** \brief Initialize using prebuilt objects - supplying initial time value. */
  RythmosSolver(
      const Teuchos::RCP<Rythmos::DefaultIntegrator<Scalar> > &stateIntegrator,
      const Teuchos::RCP<Rythmos::StepperBase<Scalar> > &stateStepper,
      const Teuchos::RCP<Thyra::NonlinearSolverBase<Scalar> > &timeStepSolver,
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &model,
      Scalar initialTime,
      Scalar finalTime,
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &initialConditionModel = Teuchos::null,
      Teuchos::EVerbosityLevel verbosityLevel = Teuchos::VERB_DEFAULT);
  //@}

  void initialize(
      const Teuchos::RCP<Teuchos::ParameterList> &appParams,
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &model,
      const Teuchos::RCP<Rythmos::IntegrationObserverBase<Scalar> > &observer = Teuchos::null);

  Teuchos::RCP<const Rythmos::IntegratorBase<Scalar> > getRythmosIntegrator() const;

  /** \name Overridden from Thyra::ModelEvaluatorBase. */
  //@{
  /** \brief . */
  Thyra::ModelEvaluatorBase::InArgs<Scalar> getNominalValues() const;
  /** \brief . */
  Thyra::ModelEvaluatorBase::InArgs<Scalar> createInArgs() const;
  /** \brief . */
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > get_p_space(int l) const;
  /** \brief . */
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > get_g_space(int j) const;
  //@}

  void addStepperFactory(const std::string & stepperName,
                         const Teuchos::RCP<RythmosStepperFactory<Scalar> > & stepperFactories);

  void addStepControlFactory(const std::string & stepControlName,
                             const Teuchos::RCP<RythmosStepControlFactory<Scalar> > & step_control_strategy);

private:
  /** \name Overridden from Thyra::ModelEvaluatorDefaultBase. */
  //@{
  /** \brief . */
  Thyra::ModelEvaluatorBase::OutArgs<Scalar> createOutArgsImpl() const;

  /** \brief . */
  void evalModelImpl(
      const Thyra::ModelEvaluatorBase::InArgs<Scalar>& inArgs,
      const Thyra::ModelEvaluatorBase::OutArgs<Scalar>& outArgs) const;
  //@}

  Teuchos::RCP<Thyra::LinearOpBase<Scalar> > create_DgDp_op_impl(int j, int l) const;

  /** \brief . */
  Teuchos::RCP<const Teuchos::ParameterList> getValidRythmosParameters() const;
  Teuchos::RCP<const Teuchos::ParameterList> getValidRythmosSolverParameters() const;

  Teuchos::RCP<Rythmos::DefaultIntegrator<Scalar> > fwdStateIntegrator;
  Teuchos::RCP<Rythmos::StepperBase<Scalar> > fwdStateStepper;
  Teuchos::RCP<Thyra::NonlinearSolverBase<Scalar> > fwdTimeStepSolver;

  Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > model;
  Teuchos::RCP<Thyra::ModelEvaluatorDefaultBase<double> > thyraModel;
  Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > initialConditionModel;

  Scalar t_initial;
  Scalar t_final;

  int num_p;
  int num_g;

  Teuchos::RCP<Teuchos::FancyOStream> out;
  Teuchos::EVerbosityLevel solnVerbLevel;

  // used for adding user defined steppers externally, this gives us "the open-close principal"
  std::map<std::string,Teuchos::RCP<RythmosStepperFactory<Scalar> > > stepperFactories;

  std::map<std::string,Teuchos::RCP<RythmosStepControlFactory<Scalar> > > stepControlFactories;

  bool isInitialized;
};

/** \brief Non-member constructor function */
template <typename Scalar>
Teuchos::RCP<RythmosSolver<Scalar> >
rythmosSolver(
    const Teuchos::RCP<Teuchos::ParameterList> &appParams,
    const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > &model,
    const Teuchos::RCP<ObserverBase<Scalar> > &piroObserver);

}

/** \class Piro::RythmosSolver
 *  \ingroup Piro_Thyra_solver_grp
 * */

#include "Piro_RythmosSolver_Def.hpp"

#endif
