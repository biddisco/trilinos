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
#ifndef MUELU_ZOLTAN2INTERFACE_DECL_HPP
#define MUELU_ZOLTAN2INTERFACE_DECL_HPP

#include "MueLu_ConfigDefs.hpp"

#if defined(HAVE_MUELU_ZOLTAN2) && defined(HAVE_MPI)

#include <Xpetra_Matrix.hpp>
#include <Xpetra_VectorFactory.hpp>

#include "MueLu_SingleLevelFactoryBase.hpp"
#include "MueLu_Zoltan2Interface_fwd.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

namespace MueLu {

  /*!
    @class Zoltan2Interface
    @brief Interface to Zoltan2 library.

    This interface provides access to partitioning methods in Zoltan2.
    Currently, it only supports the MultiJagged and RCB algorithms.
  */

  //FIXME: this class should not be templated
  template <class Scalar,
            class LocalOrdinal = typename Xpetra::Matrix<Scalar>::local_ordinal_type,
            class GlobalOrdinal = typename Xpetra::Matrix<Scalar, LocalOrdinal>::global_ordinal_type,
            class Node = typename Xpetra::Matrix<Scalar, LocalOrdinal, GlobalOrdinal>::node_type>
  class Zoltan2Interface : public SingleLevelFactoryBase {

#undef MUELU_ZOLTAN2INTERFACE_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors
    //@{

    //! Constructor
    Zoltan2Interface();

    //! Destructor
    virtual ~Zoltan2Interface() { }
    //@}

    RCP<const ParameterList> GetValidParameterList() const;

    //! @name Input
    //@{
    void DeclareInput(Level & level) const;
    //@}

    //! @name Build methods.
    //@{
    void Build(Level &level) const;

    //@}

  private:
    RCP<ParameterList> defaultZoltan2Params;

  };  //class Zoltan2Interface

#ifndef HAVE_MUELU_TPETRA_INST_INT_INT
  /*!
    @class Zoltan2Interface
    @brief Interface to Zoltan2 library.
    */

  // TAW: Oct 16 2015: we need the specialization of Zoltan2Interface since it is a object living in the Tpetra stack only.
  //                   If Tpetra is not compiled with GO=int enabled we need dummy implementations here.
  template <class Scalar, class Node>
  class Zoltan2Interface<Scalar,int,int,Node> : public SingleLevelFactoryBase {
  public:
    Zoltan2Interface() {};
    virtual ~Zoltan2Interface() { }
    RCP<const ParameterList> GetValidParameterList() const { return Teuchos::null; };
    void DeclareInput(Level & level) const {};
    void Build(Level &level) const {};
  };  //class Zoltan2Interface (specialization for LO=GO=int)
#endif

} //namespace MueLu

#define MUELU_ZOLTAN2INTERFACE_SHORT
#endif //if defined(HAVE_MUELU_ZOLTAN2) && defined(HAVE_MPI)

#endif // MUELU_ZOLTAN2INTERFACE_DECL_HPP
