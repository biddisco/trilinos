// @HEADER
// ************************************************************************
//
//        Phalanx: A Partial Differential Equation Field Evaluation 
//       Kernel for Flexible Management of Complex Dependency Chains
//                    Copyright 2008 Sandia Corporation
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
// Questions? Contact Roger Pawlowski (rppawlo@sandia.gov), Sandia
// National Laboratories.
//
// ************************************************************************
// @HEADER

#ifndef PHX_FIELD_EVALUATOR_MANAGER_DEF_HPP
#define PHX_FIELD_EVALUATOR_MANAGER_DEF_HPP

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <utility>
#include <typeinfo>
#include "Teuchos_Assert.hpp"
#include "Teuchos_TypeNameTraits.hpp"
#include "Phalanx_config.hpp"
#include "Phalanx_Evaluator.hpp"
#include "Phalanx_Exceptions.hpp"
#include "Phalanx_FieldTag_STL_Functors.hpp"

//=======================================================================
template<typename Traits>
PHX::EvaluatorManager<Traits>::
EvaluatorManager(const std::string& evaluation_type_name) :
  graphviz_filename_for_errors_("error.dot"),
  write_graphviz_file_on_error_(true),
  evaluation_type_name_(evaluation_type_name),
  sorting_called_(false),
#ifdef PHX_ALLOW_MULTIPLE_EVALUATORS_FOR_SAME_FIELD
  allow_multiple_evaluators_for_same_field_(true)
#else
  allow_multiple_evaluators_for_same_field_(false)
#endif
{ }

//=======================================================================
template<typename Traits>
PHX::EvaluatorManager<Traits>::~EvaluatorManager()
{ }

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
requireField(const PHX::FieldTag& t)
{
  FTPredRef pred(t);
  std::vector< Teuchos::RCP<PHX::FieldTag> >::iterator i = 
    std::find_if(required_fields_.begin(), required_fields_.end(), pred);
  
  if (i == required_fields_.end())
    required_fields_.push_back(t.clone());
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
registerEvaluator(const Teuchos::RCP<PHX::Evaluator<Traits> >& p)
{
#ifdef PHX_TEUCHOS_TIME_MONITOR
  // Add counter to name so that all timers have unique names
  static int count=0;
  std::stringstream uniqueName;
  uniqueName << "Phalanx: Evaluator " << count++ <<": ";
  evalTimers.push_back(
     Teuchos::TimeMonitor::getNewTimer(uniqueName.str() + p->getName()));
#endif

  // insert evaluated fields into map, check for multiple evaluators
  // that provide the same field.
  nodes_.push_back(PHX::DagNode<Traits>(static_cast<const int>(nodes_.size()),p));
  const std::vector<Teuchos::RCP<PHX::FieldTag>>& evaluatedFields = 
    p->evaluatedFields();
  for (auto i=evaluatedFields.cbegin(); i != evaluatedFields.cend(); ++i) {
    auto check = field_to_node_index_.insert(std::make_pair((*i)->identifier(), static_cast<int>(nodes_.size()-1)));
    if (!allow_multiple_evaluators_for_same_field_) {
      TEUCHOS_TEST_FOR_EXCEPTION(check.second == false,
				 PHX::multiple_evaluator_for_field_exception,
				 *this
				 << "\n\nError: PHX::EvaluatorManager::registerEvaluator() - The field \"" 
				 << (*i)->identifier() 
				 << "\" that is evaluated by the evaluator named \"" 
				 << p->getName() 
				 << "\" is already evaluated by another registered evaluator named \"" 
				 << (nodes_[field_to_node_index_[(*i)->identifier()]]).get()->getName()
				 << "\"."
				 // << " Printing evaluators:\n" << *p << "\n" 
				 // << (evaluators_[field_to_index_[(*i)->identifier()]]) 
				 << std::endl);
    }
  }
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
setDefaultGraphvizFilenameForErrors(const std::string& file_name)
{
  graphviz_filename_for_errors_ = file_name;
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
setWriteGraphvizFileOnError(bool write_file)
{
  write_graphviz_file_on_error_ = write_file;
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
sortAndOrderEvaluators()
{
#ifdef PHX_TEUCHOS_TIME_MONITOR
  Teuchos::TimeMonitor(*Teuchos::TimeMonitor::getNewTimer("Phalanx::SortAndOrderEvaluatorsNew"));
#endif
  
  // Color all nodes white, reset the discovery and final times
  for (auto& n : nodes_)
    n.resetDfsParams(PHX::Color::WHITE);

  topoSortEvalIndex.clear();

  // Loop over required fields
  int time = 0;
  for (const auto& req_field : required_fields_) {

    auto node_index_it = field_to_node_index_.find(req_field->identifier());

    if (node_index_it == field_to_node_index_.end()) {
      
      if (write_graphviz_file_on_error_)
	this->writeGraphvizFileNew(graphviz_filename_for_errors_, true, true);
      
      TEUCHOS_TEST_FOR_EXCEPTION(node_index_it == field_to_node_index_.end(),
				 PHX::missing_evaluator_exception,
				 *this
				 << "\n\nERROR: The required field \""
				 << req_field->identifier() 
				 << "\" does not have an evaluator. Current "
				 << "list of Evaluators are printed above this "
				 << "error message.\n");
    }
    
    auto& node = nodes_[node_index_it->second];
    if (node.color() == PHX::Color::WHITE)
      dfsVisit(node,time);
  }

  // Create a list of fields to allocate
  fields_.clear();
  for (std::size_t i = 0; i < topoSortEvalIndex.size(); i++) {
    const auto& fields = (nodes_[topoSortEvalIndex[i]]).get()->evaluatedFields();
    fields_.insert(fields_.end(),fields.cbegin(),fields.cend());
  }

  sorting_called_ = true;
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
dfsVisit(PHX::DagNode<Traits>& node, int& time)
{
  node.setColor(PHX::Color::GREY);
  time += 1;
  node.setDiscoveryTime(time);

  // Add the adjacencies.
  // NOTE: we could do this for all nodes before entering the DFS
  // algorithm, but then the safety check forces users to satisfy
  // dependencies for nodes that may potentially NOT be in the final
  // graph.  So we have to do it here when we know we actually will
  // use the node.
  {
    const auto& req_fields = node.get()->dependentFields(); 
    for (const auto& field : req_fields) {
      auto node_index_it = field_to_node_index_.find(field->identifier());

      if (node_index_it == field_to_node_index_.end()) {

	if (write_graphviz_file_on_error_)
	  this->writeGraphvizFileNew(graphviz_filename_for_errors_, true, true);
	
	TEUCHOS_TEST_FOR_EXCEPTION(node_index_it == field_to_node_index_.end(),
				   PHX::missing_evaluator_exception,
				   *this
				   << "\n\nERROR: The required field \""
				   << field->identifier() 
				   << "\" does not have an evaluator. Current "
				   << "list of Evaluators are printed above this "
				   << "error message.\n\n"
				   << "\nPlease inspect the EvaluatorManager output above, or \n"
				   << "visually inspect the error graph that was dumped by \n"
				   << "running the graphviz dot program on the file\n" 
				   << graphviz_filename_for_errors_ << ": \n\n"
				   << "dot -Tjpg -o error.jpg " 
				   << graphviz_filename_for_errors_ << "\n\n"
				   << "The above command generates a jpg file, \"error.jpg\"\n"
				   << "that you can view in any web browser/graphics program.\n");
      }

      node.addAdjacency(node_index_it->second);
    }
  }

  for (auto& adj_node_index : node.adjacencies()) {

    auto& adj_node = nodes_[adj_node_index];

    if (adj_node.color() == PHX::Color::WHITE) {
      dfsVisit(adj_node,time);
    }
    else if (adj_node.color() == PHX::Color::GREY) {

      std::ostringstream os_adj_node;
      this->printEvaluator(*(adj_node.get()),os_adj_node);
      std::ostringstream os_node;
      this->printEvaluator(*(node.get()),os_node);
      
      if (write_graphviz_file_on_error_)
	this->writeGraphvizFileNew(graphviz_filename_for_errors_, true, true);

      TEUCHOS_TEST_FOR_EXCEPTION(adj_node.color() == PHX::Color::GREY,
				 PHX::circular_dag_exception,
				 *this
				 << "\n\nERROR: In constructing the directed acyclic graph from \n"
				 << "the node dependencies, a circular dependency has been \n"
				 << "identified. The dependence is injected in going from node:\n\n" 
				 << os_adj_node.str() 
				 << "\n back to node\n\n" 
				 << os_node.str() 
				 << "\nPlease inspect the EvaluatorManager output above, or \n"
				 << "visually inspect the error graph that was dumped by \n"
				 << "running the graphviz dot program on the file\n" 
				 << graphviz_filename_for_errors_ << ": \n\n"
				 << "dot -Tjpg -o error.jpg " 
				 << graphviz_filename_for_errors_<< "\n\n"
				 << "The above command generates a jpg file, \"error.jpg\"\n"
				 << "that you can view in any web browser/graphics program.\n");
    }
  }
  node.setColor(PHX::Color::BLACK);
  time += 1;
  node.setFinalTime(time);
  topoSortEvalIndex.push_back(node.index()); // for topo sort
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
printEvaluator(const PHX::Evaluator<Traits>& e, std::ostream& os) const
{
  os << "Name=" << e.getName() << "\n";
  os << "  *Evaluated Fields:\n";
  for (const auto& f : e.evaluatedFields()) 
    os << "    " << f->identifier() << "\n";
  os << "  *Dependent Fields:\n";
  if (e.dependentFields().size() > 0) {
    for (const auto& f : e.dependentFields()) 
      os << "    " << f->identifier() << "\n";
  }
  else {
    os << "    None!\n";
  }
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
postRegistrationSetup(typename Traits::SetupData d,
		      PHX::FieldManager<Traits>& vm)
{
  // Call each providers' post registration setup
  for (std::size_t n = 0; n < topoSortEvalIndex.size(); ++n)
    nodes_[topoSortEvalIndex[n]].getNonConst()->postRegistrationSetup(d,vm);
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
evaluateFields(typename Traits::EvalData d)
{
  for (std::size_t n = 0; n < topoSortEvalIndex.size(); ++n) {
#ifdef PHX_TEUCHOS_TIME_MONITOR
    Teuchos::TimeMonitor Time(*evalTimers[topoSortEvalIndex[n]]);
#endif
    nodes_[topoSortEvalIndex[n]].getNonConst()->evaluateFields(d);
  }
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
preEvaluate(typename Traits::PreEvalData d)
{
  for (std::size_t n = 0; n < topoSortEvalIndex.size(); ++n)
    nodes_[topoSortEvalIndex[n]].getNonConst()->preEvaluate(d);
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
postEvaluate(typename Traits::PostEvalData d)
{
  for (std::size_t n = 0; n < topoSortEvalIndex.size(); ++n)
    nodes_[topoSortEvalIndex[n]].getNonConst()->postEvaluate(d);
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
setEvaluationTypeName(const std::string& evaluation_type_name)
{
  evaluation_type_name_ = evaluation_type_name;
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
writeGraphvizFile(const std::string filename,
		  bool writeEvaluatedFields,
		  bool writeDependentFields,
		  bool ) const
{
  writeGraphvizFileNew(filename,writeEvaluatedFields,writeDependentFields);    
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
writeGraphvizFileNew(const std::string filename,
		     bool writeEvaluatedFields,
		     bool writeDependentFields) const
{
  std::ofstream ofs;
  ofs.open(filename.c_str());
  
  ofs << "digraph G {\n";

  // This can be called from inside a DFS during an error, so we can't
  // change the DFS node objects when starting a new search. Need to
  // copy the node vector.
  std::vector<PHX::DagNode<Traits>> nodes_copy;
  nodes_copy.insert(nodes_copy.end(),nodes_.cbegin(),nodes_.cend());

  for (auto& n : nodes_copy)
    n.resetDfsParams(PHX::Color::WHITE);

  // Loop over required fields
  int missing_node_index = nodes_.size();
  for (const auto& req_field : required_fields_) {
    auto node_index_it = field_to_node_index_.find(req_field->identifier());

    if (node_index_it == field_to_node_index_.end()) {
      ofs << missing_node_index 
	  << " ["  << "fontcolor=\"red\"" << ", label=\"  ** MISSING EVALUATOR **\\n    " 
	  << req_field->identifier() << "    **** MISSING ****\"]\n";     
      missing_node_index += 1;
    }
    else {
      auto& node = nodes_copy[node_index_it->second];
      if (node.color() == PHX::Color::WHITE)
	writeGraphvizDfsVisit(node,
			      nodes_copy,
			      ofs,
			      writeEvaluatedFields,
			      writeDependentFields);
    }
  }

  ofs << "}";
  ofs.close();
}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::
writeGraphvizDfsVisit(PHX::DagNode<Traits>& node,
		      std::vector<PHX::DagNode<Traits>>& nodes_copy,
		      std::ostream& ofs,
		      const bool writeEvaluatedFields,
		      const bool writeDependentFields) const
{
  node.setColor(PHX::Color::GREY);

  // Add valid adjacencies, write node
  {
    std::string font_color = "";
    std::vector<std::string> dependent_field_labels;

    const auto& req_fields = node.get()->dependentFields(); 
    for (const auto& field : req_fields) {
      auto node_index_it = field_to_node_index_.find(field->identifier());
      
      // failed to find node
      if (node_index_it == field_to_node_index_.end()) {
	font_color = "red";
	std::string dependent_field_label = field->identifier() + "    **** MISSING ****";
	dependent_field_labels.emplace(dependent_field_labels.end(),dependent_field_label);
      }
      else {
	dependent_field_labels.push_back(field->identifier());	
	node.addAdjacency(node_index_it->second);
      }
    }

    // Write the node
    ofs << node.index() 
	<< " [fontcolor=\"" << font_color 
	<< "\", label=\"" << node.get()->getName();
    if (writeEvaluatedFields) {
      ofs << "\\n   Evaluates:";
      const auto& eval_fields = node.get()->evaluatedFields(); 
      for (const auto& field : eval_fields)
	ofs << "\\n      " << field->identifier();
    }
    if (writeDependentFields) {
      ofs << "\\n   Dependencies:";
      for(const auto& field : dependent_field_labels)
	ofs << "\\n      " << field;
    }
    ofs << "\"]\n";
  }

  // Write edges and trace adjacencies
  for (auto& adj_node_index : node.adjacencies()) {

    auto& adj_node = nodes_copy[adj_node_index];

    if (adj_node.color() == PHX::Color::WHITE) {
      ofs << node.index() << "->" << adj_node.index() << "\n";
      writeGraphvizDfsVisit(adj_node,
			    nodes_copy,
			    ofs,
			    writeEvaluatedFields,
			    writeDependentFields);
    }
    else if (adj_node.color() == PHX::Color::GREY) {
      ofs << node.index() << "->" << adj_node.index() << " [color=red]\n";
    }
    else { // BLACK node
      ofs << node.index() << "->" << adj_node.index() << "\n";
    }
  }

  node.setColor(PHX::Color::BLACK);
}


//=======================================================================
template<typename Traits>
const std::vector< Teuchos::RCP<PHX::FieldTag> >& 
PHX::EvaluatorManager<Traits>::getFieldTags()
{
  return fields_;
}

//=======================================================================
template<typename Traits>
bool PHX::EvaluatorManager<Traits>::sortingCalled() const
{
  return sorting_called_;
}

//=======================================================================
template<typename Traits>
const std::vector<int>& 
PHX::EvaluatorManager<Traits>::getEvaluatorInternalOrdering() const
{return topoSortEvalIndex;}

//=======================================================================
template<typename Traits>
void PHX::EvaluatorManager<Traits>::print(std::ostream& os) const
{
  os << "******************************************************" << std::endl;
  os << "PHX::EvaluatorManager" << std::endl;
  os << "Evaluation Type = " << evaluation_type_name_ << std::endl;
  os << "******************************************************" << std::endl;

  os << "\n** Starting Required Field List" << std::endl;
  for (std::size_t i = 0; i < required_fields_.size(); i++) {
    os << *(this->required_fields_[i]) << std::endl;
  }
  os << "** Finished Required Field List" << std::endl;

  os << "\n** Starting Registered Field Evaluators" << std::endl;
  for (std::size_t n=0; n < nodes_.size(); ++n) {
    os << "Evaluator[" << n << "]: ";
    this->printEvaluator(*(nodes_[n].get()),os);
  }
  os << "** Finished Registered Field Evaluators" << std::endl;


  os << "\n** Starting Evaluator Order" << std::endl;
  for (std::size_t k = 0; k < topoSortEvalIndex.size(); ++k) {
    os << k << "    " << topoSortEvalIndex[k] << std::endl;
  }
  os << "\nDetails:\n";
  for (std::size_t n = 0; n < topoSortEvalIndex.size(); ++n) {
    os << "Evaluator[" << topoSortEvalIndex[n] << "]: ";
    this->printEvaluator(*(nodes_[topoSortEvalIndex[n]].get()),os);    
  }
  os << "** Finished Provider Evaluation Order" << std::endl;

  os << "******************************************************" << std::endl;
  os << "Finished PHX::EvaluatorManager" << std::endl;
  os << "Evaluation Type = " << evaluation_type_name_ << std::endl;
  os << "******************************************************" << std::endl;

}

//=======================================================================
template<typename Traits>
std::ostream&
PHX::operator<<(std::ostream& os, const PHX::EvaluatorManager<Traits>& m)
{
  m.print(os);
  return os;
}

//=======================================================================

#endif
