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

#ifndef PANZER_MODEL_EVALUATOR_DECL_HPP
#define PANZER_MODEL_EVALUATOR_DECL_HPP

#include "Panzer_config.hpp"

#include "Panzer_Traits.hpp"
#include "Panzer_AssemblyEngine_TemplateManager.hpp"
#include "Panzer_ParameterLibrary.hpp"
#include "Panzer_GlobalEvaluationData.hpp"
#include "Panzer_ResponseLibrary.hpp"
#include "Panzer_ResponseMESupportBase.hpp"
#include "Panzer_ResponseMESupportBuilderBase.hpp"

#include "Teuchos_RCP.hpp"
#include "Teuchos_AbstractFactory.hpp"

#include "Thyra_VectorBase.hpp"
#include "Thyra_VectorSpaceBase.hpp"
#include "Thyra_StateFuncModelEvaluatorBase.hpp"
#include "Thyra_LinearOpWithSolveFactoryBase.hpp"

#include <Panzer_NodeType.hpp>

namespace panzer {

class FieldManagerBuilder;
template<typename> class LinearObjFactory;
struct GlobalData;
class ReadOnlyVectorGlobalEvaluationData;

template<typename Scalar>
class ModelEvaluator
  : public Thyra::StateFuncModelEvaluatorBase<Scalar>
{
public:

public:

  /** \name Constructors/Initializers/Accessors */
  //@{

  ModelEvaluator(const Teuchos::RCP<panzer::FieldManagerBuilder>& fmb,
                 const Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits> >& rLibrary,
                 const Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits> >& lof,
                 const std::vector<Teuchos::RCP<Teuchos::Array<std::string> > >& p_names,
                 const std::vector<Teuchos::RCP<Teuchos::Array<double> > >& p_values,
                 const Teuchos::RCP<const Thyra::LinearOpWithSolveFactoryBase<Scalar> > & solverFactory,
                 const Teuchos::RCP<panzer::GlobalData>& global_data,
                 bool build_transient_support,double t_init);

  ModelEvaluator(const Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits> >& lof,
                 const Teuchos::RCP<const Thyra::LinearOpWithSolveFactoryBase<Scalar> > & solverFactory,
                 const Teuchos::RCP<panzer::GlobalData>& global_data,
                 bool build_transient_support,double t_init);

  /** \brief . */
  ModelEvaluator();

  //@}

  /** \name Public functions overridden from ModelEvaulator. */
  //@{

  /** \brief . */
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > get_x_space() const;

  /** \brief . */
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > get_f_space() const;

  /** \brief . */
  Teuchos::RCP<const Teuchos::Array<std::string> > get_p_names(int i) const;

  /** \brief . */
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > get_p_space(int i) const;

  /** \brief . */
  Teuchos::ArrayView<const std::string> get_g_names(int i) const override;

  /** \brief . */
  const std::string & get_g_name(int i) const;

  /** \brief . */
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > get_g_space(int i) const;

  /** \brief . */
  Teuchos::RCP<Thyra::LinearOpBase<Scalar> > create_W_op() const;

  /** \brief . */
  Teuchos::RCP<const Thyra::LinearOpWithSolveFactoryBase<Scalar> > get_W_factory() const;

  /** \brief . */
  Teuchos::RCP<Thyra::LinearOpBase<Scalar> > create_DfDp_op(int i) const;

  /** \brief . */
  Thyra::ModelEvaluatorBase::InArgs<Scalar> createInArgs() const;

  Thyra::ModelEvaluatorBase::InArgs<Scalar> getNominalValues() const;

  //@}

  void setupModel(const Teuchos::RCP<panzer::WorksetContainer> & wc,
                  const std::vector<Teuchos::RCP<panzer::PhysicsBlock> >& physicsBlocks,
                  const std::vector<panzer::BC> & bcs,
                  const panzer::EquationSetFactory & eqset_factory,
                  const panzer::BCStrategyFactory& bc_factory,
                  const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>& volume_cm_factory,
                  const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>& bc_cm_factory,
                  const Teuchos::ParameterList& closure_models,
                  const Teuchos::ParameterList& user_data,
                  bool writeGraph=false,const std::string & graphPrefix="");

  /** Add a simple (i.e. nondistributed) parameter to the model evaluator.

      Note that these parameters will automatically use the parameter library
      passed into the model evaluator object through the GlobalData.

      \param[in]  name Name of the parameter
      \param[in]  initial Initial (default) condition for this parameter

      \return The index associated with this parameter for accessing it through the ModelEvaluator interface.
      \note The implementation for this is a call to the Array version of <code>addParameter</code>
  */
  int addParameter(const std::string & name,const Scalar & initial);

  /** Add a simple (i.e. nondistributed) parameter to the model evaluator.

      Note that these parameters will automatically use the parameter library
      passed into the model evaluator object through the GlobalData.

      \param[in]  names Names of the parameter
      \param[in]  initialValues Initial values for the parameters

      \return The index associated with this parameter for accessing it through the ModelEvaluator interface.
  */
  int addParameter(const Teuchos::Array<std::string> & names,
                   const Teuchos::Array<Scalar> & initialValues);

  /** Add a distributed parameter to the model evaluator

      Distributed parameters are special in that they most likely
      will require a global to ghost call before being used in the
      evaluator.  This function registers the parameter and any
      needed machinery to perform the global to ghost call.

      \param[in] name Name of the distributed parameter
      \param[in] vs   Vector space that this corresponds to
      \param[in] ged  Global evaluation data object that handles ghosting
      \param[in] initial Initial value to use for this parameter (defaults in the equation set)
      \param[in] ugi Unique global indexer used for this parameter. Useful in constructing
                     derivatives.

      \return The index associated with this parameter for accessing it through the ModelEvaluator interface.
  */
  int addDistributedParameter(const std::string & name,
                              const Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > & vs,
                              const Teuchos::RCP<GlobalEvaluationData> & ged,
                              const Teuchos::RCP<const Thyra::VectorBase<Scalar> > & initial,
                              const Teuchos::RCP<const UniqueGlobalIndexerBase> & ugi=Teuchos::null);

  /** Add a global evaluation data object that will be filled as a side
    * effect when evalModel is called. This is useful for building things
    * like auxiliary operators used in block preconditioning. This will not
    * be used as a parameter (or response) to the model evaluator. 
    *
    * \param[in] name Name to associate with global evaluation data object
    * \param[in] ged Pointer to a global evaluation data object
    */
  void addNonParameterGlobalEvaluationData(const std::string & name,
                                           const Teuchos::RCP<GlobalEvaluationData> & ged);

  /** Add a response specified by a list of WorksetDescriptor objects. The specifics of the
    * response are specified by the response factory builder. This version supports computing derivatives with 
    * respect to both the state ('x') and control ('p') variables and is thus ``flexible''.
    *
    * NOTE: Response factories must use a response of type <code>ResponseMESupportBase</code>. This is
    * how the model evaluator parses and puts responses in the right location. If this condition is violated
    * the <code>evalModel</code> call will fail. Furthermore, this method cannot be called after <code>buildRespones</code>
    * has been called.
    *
    * \param[in] responseName Name of the response to be added.
    * \param[in] wkst_desc A vector of descriptors describing the types of elements
    *                                that make up the response.
    * \param[in] builder Builder that builds the correct response object.
    *
    * \return The index associated with this response for accessing it through the ModelEvaluator interface.
    */
  int addFlexibleResponse(const std::string & responseName,
                         const std::vector<WorksetDescriptor> & wkst_desc,
                         const Teuchos::RCP<ResponseMESupportBuilderBase> & builder);

  /** Add a response specified by a list of WorksetDescriptor objects. The specifics of the
    * response are specified by the response factory builder.
    *
    * NOTE: Response factories must use a response of type <code>ResponseMESupportBase</code>. This is
    * how the model evaluator parses and puts responses in the right location. If this condition is violated
    * the <code>evalModel</code> call will fail. Furthermore, this method cannot be called after <code>buildRespones</code>
    * has been called.
    *
    * \param[in] responseName Name of the response to be added.
    * \param[in] wkst_desc A vector of descriptors describing the types of elements
    *                                that make up the response.
    * \param[in] builder Builder that builds the correct response object.
    *
    * \return The index associated with this response for accessing it through the ModelEvaluator interface.
    */
  template <typename ResponseEvaluatorFactory_BuilderT>
  int addResponse(const std::string & responseName,
                  const std::vector<WorksetDescriptor> & wkst_desc,
                  const ResponseEvaluatorFactory_BuilderT & builder);

  /** Build all the responses set on the model evaluator.  Once this method is called
    * no other responses can be added. An exception is thrown if they are.
    */
  void buildResponses(
       const std::vector<Teuchos::RCP<panzer::PhysicsBlock> >& physicsBlocks,
       const panzer::EquationSetFactory & eqset_factory,
       const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>& cm_factory,
       const Teuchos::ParameterList& closure_models,
       const Teuchos::ParameterList& user_data,
       const bool write_graphviz_file=false,
       const std::string& graphviz_file_prefix="")
  { responseLibrary_->buildResponseEvaluators(physicsBlocks,eqset_factory,cm_factory,closure_models,user_data,write_graphviz_file,graphviz_file_prefix);
    require_in_args_refresh_ = true;
    require_out_args_refresh_ = true;

    typedef Thyra::ModelEvaluatorBase MEB;
    MEB::OutArgsSetup<Scalar> outArgs;
    outArgs.setModelEvalDescription(this->description());
    outArgs.set_Np_Ng(parameters_.size(), responses_.size());
    outArgs.setSupports(MEB::OUT_ARG_f);
    outArgs.setSupports(MEB::OUT_ARG_W_op);
    prototypeOutArgs_ = outArgs; }

  /** Build all the responses set on the model evaluator.  Once this method is called
    * no other responses can be added. An exception is thrown if they are.
    */
  void buildResponses(
       const std::vector<Teuchos::RCP<panzer::PhysicsBlock> >& physicsBlocks,
       const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>& cm_factory,
       const Teuchos::ParameterList& closure_models,
       const Teuchos::ParameterList& user_data,
       const bool write_graphviz_file=false,
       const std::string& graphviz_file_prefix="")
  { responseLibrary_->buildResponseEvaluators(physicsBlocks,cm_factory,closure_models,user_data,write_graphviz_file,graphviz_file_prefix);
    require_in_args_refresh_ = true;
    require_out_args_refresh_ = true;

    typedef Thyra::ModelEvaluatorBase MEB;
    MEB::OutArgsSetup<Scalar> outArgs;
    outArgs.setModelEvalDescription(this->description());
    outArgs.set_Np_Ng(parameters_.size(), responses_.size());
    outArgs.setSupports(MEB::OUT_ARG_f);
    outArgs.setSupports(MEB::OUT_ARG_W_op);
    prototypeOutArgs_ = outArgs; }

  /** This method builds the response libraries that build the 
    * dfdp sensitivities for the distributed parameters if requested.
    * Note that in general the user is expected to call this through
    * setupModel and not call it directly.
    */
  void buildDistroParamDfDp_RL(
       const Teuchos::RCP<panzer::WorksetContainer> & wc,
       const std::vector<Teuchos::RCP<panzer::PhysicsBlock> >& physicsBlocks,
       const std::vector<panzer::BC> & bcs,
       const panzer::EquationSetFactory & eqset_factory,
       const panzer::BCStrategyFactory& bc_factory,
       const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>& cm_factory,
       const Teuchos::ParameterList& closure_models,
       const Teuchos::ParameterList& user_data,
       const bool write_graphviz_file=false,
       const std::string& graphviz_file_prefix="");

  /** This method builds the response libraries that build the 
    * dgdp sensitivities for the distributed parameters if requested.
    * This only applies to "flexible" responses.
    * Note that in general the user is expected to call this through
    * setupModel and not call it directly.
    */
  void buildDistroParamDgDp_RL(
       const Teuchos::RCP<panzer::WorksetContainer> & wc,
       const std::vector<Teuchos::RCP<panzer::PhysicsBlock> >& physicsBlocks,
       const std::vector<panzer::BC> & bcs,
       const panzer::EquationSetFactory & eqset_factory,
       const panzer::BCStrategyFactory& bc_factory,
       const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>& cm_factory,
       const Teuchos::ParameterList& closure_models,
       const Teuchos::ParameterList& user_data,
       const bool write_graphviz_file=false,
       const std::string& graphviz_file_prefix="");

  /** This function is intended for experts only, it allows for a beta to be set for the
    * dirichlet conditions only. This allows the dirichlet condition to be propagated to
    * the mass matrix. The reason it is one time only is that it breaks encapsulation,
    * and should be only used if absolutely neccessary.
    *
    * \param[in] beta Value of beta to use.
    */
  void setOneTimeDirichletBeta(const Scalar & beta) const;

  /** Apply the dirichlet boundary conditions to the vector "f" using the 
    * "x" values as the current solution.
    */
  void applyDirichletBCs(const Teuchos::RCP<Thyra::VectorBase<Scalar> > & x,
                         const Teuchos::RCP<Thyra::VectorBase<Scalar> > & f) const;

  /** Setup all the assembly input arguments required by "inArgs".
    *
    * \param[in] inArgs Model evalutor input arguments
    * \param[in/out] ae_inArgs Assembly engine input arguments.
    */
  void setupAssemblyInArgs(const Thyra::ModelEvaluatorBase::InArgs<Scalar> & inArgs,
                           panzer::AssemblyEngineInArgs & ae_inargs) const;

  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits> > getResponseLibrary() const
  { return responseLibrary_; }

private:

  /** \name Private functions overridden from ModelEvaulatorDefaultBase. */
  //@{

  /** \brief . */
  Thyra::ModelEvaluatorBase::OutArgs<Scalar> createOutArgsImpl() const;

  /** \brief . */
  void evalModelImpl(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                     const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //@}

  //! Evaluate a simple model, meaning a residual and a jacobian, no fancy stochastic galerkin or multipoint
  void evalModelImpl_basic(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                           const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Construct a simple response dicatated by this set of out args
  void evalModelImpl_basic_g(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                             const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  /** handles evaluation of responses dgdx
    *
    * \note This method should (basically) be a no-op if <code>required_basic_dgdx(outArgs)==false</code>.
    *       However, for efficiency this is not checked.
    */
  void evalModelImpl_basic_dgdx(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                                const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  /** handles evaluation of responses dgdp (distributed)
    *
    * \note This method should (basically) be a no-op if <code>required_basic_dgdx_distro(outArgs)==false</code>.
    *       However, for efficiency this is not checked.
    */
  void evalModelImpl_basic_dgdp_distro(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                                       const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  /** handles evaluation of dfdp
    *
    * \note This method should (basically) be a no-op if <code>required_basic_dfdp_scalar(outArgs)==false</code>.
    *       However, for efficiency this is not checked.
    */
  void evalModelImpl_basic_dfdp_scalar(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                                       const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  /** handles evaluation of dfdp
    *
    * \note This method should (basically) be a no-op if <code>required_basic_dfdp_distro(outArgs)==false</code>.
    *       However, for efficiency this is not checked.
    */
  void evalModelImpl_basic_dfdp_distro(const Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
                                       const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Does this set of out args require a simple response?
  bool required_basic_g(const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Are their required responses in the out args? DgDx 
  bool required_basic_dgdx(const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Are their required responses in the out args? DgDp 
  bool required_basic_dgdp_distro(const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Are derivatives of the residual with respect to the scalar parameters in the out args? DfDp 
  bool required_basic_dfdp_scalar(const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Are derivatives of the residual with respect to the distributed parameters in the out args? DfDp 
  bool required_basic_dfdp_distro(const Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs) const;

  //! Initialize the nominal values with good starting conditions
  void initializeNominalValues();

private: // data members

  struct ParameterObject {
    bool is_distributed; // or (is scalar?)
    Teuchos::RCP<Teuchos::Array<std::string> > names;
    Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > space;
    Teuchos::RCP<const Thyra::VectorBase<Scalar> > initial_value;

    // for distributed parameters
    Teuchos::RCP<const UniqueGlobalIndexerBase> global_indexer;
    Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits> > dfdp_rl;
        // for residual sensitivities with respect to a distributed parameter
    Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits> > dgdp_rl;
        // for response sensitivities with respect to a distributed parameter

    // for scalar parameters
    panzer::ParamVec scalar_value;
  };

  struct ResponseObject {
    std::string name;
    Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > space;

    // for distributed parameter sensitivities
    Teuchos::RCP<ResponseMESupportBuilderBase> builder;
        // used for delayed construction of dgdp (distributed parameter) responses
    std::vector<WorksetDescriptor> wkst_desc;
        // used for delayed construction of dgdp (distributed parameter) responses
        
    struct SearchName {
      std::string name;
      SearchName(const std::string & n) : name(n) {}
      bool operator()(const Teuchos::RCP<ResponseObject> & ro) { return name==ro->name; }
    };
  };

  Teuchos::RCP<ParameterObject> createScalarParameter(const Teuchos::Array<std::string> & names,
                                                      const Teuchos::Array<Scalar> & in_values) const;
  Teuchos::RCP<ParameterObject> createDistributedParameter(const std::string & key,
                        const Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > & vs,
                        const Teuchos::RCP<const Thyra::VectorBase<Scalar> > & initial,
                        const Teuchos::RCP<const UniqueGlobalIndexerBase> & ugi) const;

  double t_init_;

  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > x_space_;
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > f_space_;

  mutable Thyra::ModelEvaluatorBase::InArgs<Scalar> prototypeInArgs_;
  mutable Thyra::ModelEvaluatorBase::OutArgs<Scalar> prototypeOutArgs_;

  mutable Thyra::ModelEvaluatorBase::InArgs<Scalar> nominalValues_;

  mutable panzer::AssemblyEngine_TemplateManager<panzer::Traits> ae_tm_;     // they control and provide access to evaluate

  std::vector<Teuchos::RCP<ParameterObject> > parameters_;

  mutable bool require_in_args_refresh_;
  mutable bool require_out_args_refresh_;

  // responses
  mutable Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits> > responseLibrary_;
  std::vector<Teuchos::RCP<ResponseObject> > responses_;

  Teuchos::RCP<panzer::GlobalData> global_data_;
  bool build_transient_support_;

  // basic specific linear object objects
  Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits> > lof_;
  mutable Teuchos::RCP<panzer::LinearObjContainer> ghostedContainer_;
  mutable Teuchos::RCP<ReadOnlyVector_GlobalEvaluationData> xContainer_;
  mutable Teuchos::RCP<ReadOnlyVector_GlobalEvaluationData> xdotContainer_;


  Teuchos::RCP<const Thyra::LinearOpWithSolveFactoryBase<Scalar> > solverFactory_;

  GlobalEvaluationDataContainer nonParamGlobalEvaluationData_;
  GlobalEvaluationDataContainer distrParamGlobalEvaluationData_;

  mutable bool oneTimeDirichletBeta_on_;
  mutable Scalar oneTimeDirichletBeta_;
};

// Inline definition of the add response (its template on the builder type)
template<typename Scalar>
template <typename ResponseEvaluatorFactory_BuilderT>
int ModelEvaluator<Scalar>::
addResponse(const std::string & responseName,
            const std::vector<WorksetDescriptor> & wkst_desc,
            const ResponseEvaluatorFactory_BuilderT & builder)
{
   using Teuchos::RCP;
   using Teuchos::rcp;

   // see if the response evaluators have been constucted yet
   TEUCHOS_TEST_FOR_EXCEPTION(responseLibrary_->responseEvaluatorsBuilt(),std::logic_error,
                              "panzer::ModelEvaluator::addResponse: Response with name \"" << responseName << "\" "
                              "cannot be added to the model evaluator because evalModel has already been called!");

   // add the response
   responseLibrary_->addResponse(responseName,wkst_desc,builder);

   // check that the response can be found
   TEUCHOS_TEST_FOR_EXCEPTION(std::find_if(responses_.begin(),responses_.end(),typename ResponseObject::SearchName(responseName))!=responses_.end(),
                              std::logic_error,
                              "panzer::ModelEvaluator::addResponse: Response with name \"" << responseName << "\" "
                              "has already been added to the model evaluator!");

   // allocate response object
   RCP<ResponseObject> respObject = rcp(new ResponseObject);

   // handle panzer::Traits::Residual
   {
      // check that at least there is a response value
      Teuchos::RCP<panzer::ResponseBase> respBase = responseLibrary_->getResponse<panzer::Traits::Residual>(responseName);
      TEUCHOS_TEST_FOR_EXCEPTION(respBase==Teuchos::null,std::logic_error,
                                 "panzer::ModelEvaluator::addResponse: Response with name \"" << responseName << "\" "
                                 "has no residual type! Not sure what is going on!");
   
      // check that the response supports interactions with the model evaluator
      Teuchos::RCP<panzer::ResponseMESupportBase<panzer::Traits::Residual> > resp = 
          Teuchos::rcp_dynamic_cast<panzer::ResponseMESupportBase<panzer::Traits::Residual> >(respBase);
      TEUCHOS_TEST_FOR_EXCEPTION(resp==Teuchos::null,std::logic_error,
                                 "panzer::ModelEvaluator::addResponse: Response with name \"" << responseName << "\" "
                                 "resulted in bad cast to panzer::ResponseMESupportBase, the type of the response is incompatible!");

      // set the response in the model evaluator
      Teuchos::RCP<const Thyra::VectorSpaceBase<double> > vs = resp->getVectorSpace();
      respObject->space = vs;

      // lets be cautious and set a vector on the response
      resp->setVector(Thyra::createMember(vs));
   }

   // handle panzer::Traits::Jacobian (do a quick safety check, response is null or appropriate for jacobian)
   Teuchos::RCP<panzer::ResponseBase> respJacBase = responseLibrary_->getResponse<panzer::Traits::Jacobian>(responseName);
   if(respJacBase!=Teuchos::null) {
      typedef panzer::Traits::Jacobian RespEvalT;

      // check that the response supports interactions with the model evaluator
      Teuchos::RCP<panzer::ResponseMESupportBase<RespEvalT> > resp = 
          Teuchos::rcp_dynamic_cast<panzer::ResponseMESupportBase<RespEvalT> >(respJacBase);
      TEUCHOS_TEST_FOR_EXCEPTION(resp==Teuchos::null,std::logic_error,
                                 "panzer::ModelEvaluator::addResponse: Response with name \"" << responseName << 
                                 "\" resulted in bad cast to panzer::ResponseMESupportBase<Jacobian>, the type "
                                 "of the response is incompatible!");

      // setup the vector (register response as epetra)
      if(resp->supportsDerivative())
         resp->setDerivative(resp->buildEpetraDerivative());
   }

   respObject->name = responseName;
   respObject->wkst_desc = wkst_desc;
 
   responses_.push_back(respObject);

   require_in_args_refresh_ = true;
   require_out_args_refresh_ = true;

   return responses_.size()-1;
}


}

// #include "Panzer_ModelEvaluator_impl.hpp"

#endif 
