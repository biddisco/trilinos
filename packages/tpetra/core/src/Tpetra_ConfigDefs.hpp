// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef TPETRA_CONFIGDEFS_HPP
#define TPETRA_CONFIGDEFS_HPP

#ifndef __cplusplus
#define __cplusplus
#endif // ifndef __cplusplus

/* this section undefines all the things autotools defines for us that we wish it didn't. */

#ifdef PACKAGE
#undef PACKAGE
#endif // ifdef PACKAGE

#ifdef PACKAGE_NAME
#undef PACKAGE_NAME
#endif // ifdef PACKAGE_NAME

#ifdef PACKAGE_BUGREPORT
#undef PACKAGE_BUGREPORT
#endif // ifdef PACKAGE_BUGREPORT

#ifdef PACKAGE_STRING
#undef PACKAGE_STRING
#endif // ifdef PACKAGE_STRING

#ifdef PACKAGE_TARNAME
#undef PACKAGE_TARNAME
#endif // ifdef PACKAGE_TARNAME

#ifdef PACKAGE_VERSION
#undef PACKAGE_VERSION
#endif // ifdef PACKAGE_VERSION

#ifdef VERSION
#undef VERSION
#endif // ifdef VERSION

// end of undoing autoconf's work section

#include <TpetraCore_config.h>
#include <Teuchos_ConfigDefs.hpp>
#include <Kokkos_ConfigDefs.hpp>
#include <Kokkos_DefaultNode.hpp>

//! %Tpetra namespace
namespace Tpetra {
  // Used in all Tpetra code that explicitly must a type (like a loop index)
  // that is used with the Teuchos::Array[View,RCP] classes.

  //! Size type for Teuchos Array objects.
  typedef Teuchos_Ordinal Array_size_type;
}

// these make some of the macros in Tpetra_Util.hpp much easier to describe
#ifdef HAVE_TPETRA_THROW_EFFICIENCY_WARNINGS
  #define TPETRA_THROWS_EFFICIENCY_WARNINGS 1
#else
  #define TPETRA_THROWS_EFFICIENCY_WARNINGS 0
#endif

#ifdef HAVE_TPETRA_PRINT_EFFICIENCY_WARNINGS
  #define TPETRA_PRINTS_EFFICIENCY_WARNINGS 1
#else
  #define TPETRA_PRINTS_EFFICIENCY_WARNINGS 0
#endif

#ifdef HAVE_TPETRA_THROW_ABUSE_WARNINGS
  #define TPETRA_THROWS_ABUSE_WARNINGS 1
#else
  #define TPETRA_THROWS_ABUSE_WARNINGS 0
#endif

#ifdef HAVE_TPETRA_PRINT_ABUSE_WARNINGS
  #define TPETRA_PRINTS_ABUSE_WARNINGS 1
#else
  #define TPETRA_PRINTS_ABUSE_WARNINGS 0
#endif


#include <functional>

//#ifndef __CUDACC__
// mem management
#include <Teuchos_ArrayView.hpp>
#include <Teuchos_ArrayRCP.hpp>
#include <Teuchos_Array.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_Tuple.hpp>
// traits classes
#include <Teuchos_OrdinalTraits.hpp>
#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_TypeNameTraits.hpp>
#include <Teuchos_NullIteratorTraits.hpp>
#include <Teuchos_SerializationTraits.hpp>
// comm
#include <Teuchos_Comm.hpp>
#include <Teuchos_CommHelpers.hpp>
// misc
#include <Teuchos_ParameterList.hpp>
//#endif

//! Namespace for Tpetra classes and methods
namespace Tpetra {

  /// \brief Global size_t object.
  ///
  /// This type is intended to support scenarios where the global
  /// memory allocation is larger than that of a single node.
  ///
  /// Currently, it is typedefed to \c size_t.
  typedef size_t global_size_t;

  /// \brief Enum for local versus global allocation of Map entries.
  ///
  /// \c LocallyReplicated means that the Map's entries are locally
  /// replicated across all processes.
  ///
  /// \c GloballyDistributed means that the Map's entries are globally
  /// distributed across all processes.
  enum LocalGlobal {
    LocallyReplicated,
    GloballyDistributed
  };

  /// \brief Return status of Map remote index lookup (getRemoteIndexList()).
  enum LookupStatus {
    AllIDsPresent, /*!< All queried indices were present in the Map */
    IDNotPresent   /*!< At least one of the specified indices was not present in the Map */
  };

  /*! Allocation profile for matrix/graph entries */
  enum ProfileType {
    StaticProfile,  /*!< Single, static allocation (strict and more efficient) */
    DynamicProfile  /*!< Multiple, dynamic allocations (flexibile, but less efficient) */
  };

  /*! Optimize storage option */
  enum OptimizeOption {
    DoOptimizeStorage,   /*!< Indicates that storage should be optimized */
    DoNotOptimizeStorage /*!< Indicates that storage should not be optimized */
  };


  /// \brief Namespace for Tpetra implementation details.
  /// \warning Do NOT rely on the contents of this namespace.
  namespace Details {

    //! Declarations of values of Tpetra classes' default template parameters.
    namespace DefaultTypes {
      //! Default value of Scalar template parameter.
      typedef double scalar_type;
      //! Default value of LocalOrdinal template parameter.
      typedef int local_ordinal_type;

      /// \typedef global_ordinal_type
      /// \brief Default value of GlobalOrdinal template parameter.
#if defined(HAVE_TPETRA_INST_INT_INT)
      typedef int global_ordinal_type;
#elif defined(HAVE_TPETRA_INST_INT_LONG_LONG)
      typedef long long global_ordinal_type;
#elif defined(HAVE_TPETRA_INST_INT_LONG)
      typedef long global_ordinal_type;
#elif defined(HAVE_TPETRA_INST_INT_UNSIGNED_LONG)
      typedef unsigned long global_ordinal_type;
#elif defined(HAVE_TPETRA_INST_INT_UNSIGNED)
      typedef unsigned global_ordinal_type;
#else
#  error "Tpetra: No global ordinal types in the set {int, long long, long, unsigned long, unsigned} have been enabled."
#endif
      //! Default value of Node template parameter.
      typedef KokkosClassic::DefaultNode::DefaultNodeType node_type;
    } // namespace DefaultTypes

  } // namespace Details

  enum EPrivateComputeViewConstructor {
    COMPUTE_VIEW_CONSTRUCTOR
  };

  enum EPrivateHostViewConstructor {
    HOST_VIEW_CONSTRUCTOR
  };

  // import Teuchos memory management classes into Tpetra
//#ifndef __CUDACC__
  using Teuchos::ArrayRCP;
  using Teuchos::ArrayView;
  using Teuchos::Array;
  using Teuchos::OrdinalTraits;
  using Teuchos::ScalarTraits;
  using Teuchos::RCP;
  using Teuchos::Tuple;
  using Teuchos::Comm;
  using Teuchos::null;

  using Teuchos::outArg;
  using Teuchos::tuple;
  using Teuchos::arcp;
  using Teuchos::rcp;
  using Teuchos::rcpFromRef;
  using Teuchos::rcp_const_cast;
  using Teuchos::av_reinterpret_cast;
  using Teuchos::arcp_reinterpret_cast;

  using Teuchos::typeName;

  using Teuchos::ParameterList;
  using Teuchos::parameterList;
  using Teuchos::sublist;
//#endif

  /// \class project1st
  /// \brief Binary function that returns its first argument.
  /// \tparam Arg1 Type of the first argument, and type of the
  ///   return value.
  /// \tparam Arg2 Type of the second argument.  It may differ from
  ///   the type of the first argument.
  ///
  /// This function object might be defined in the std namespace; it
  /// is an SGI extension to the STL.  We can't count on it living in
  /// the std namespace, but it's useful, so we define it here.
  ///
  /// If you apply <tt>using namespace std;</tt> to the global
  /// namespace, and if your STL implementation includes project1st,
  /// it will cause collisions with this definition.  We recommend for
  /// this and other reasons not to state <tt>using namespace
  /// std;</tt> in the global namespace.
  template<class Arg1, class Arg2>
  class project1st : public std::binary_function<Arg1, Arg2, Arg1> {
  public:
    typedef Arg1 first_argument_type;
    typedef Arg2 second_argument_type;
    typedef Arg1 result_type;
    Arg1 operator () (const Arg1& x, const Arg2& ) const {
      return x;
    }
  };

  /// \class project2nd
  /// \brief Binary function that returns its second argument.
  /// \tparam Arg1 Type of the first argument.
  /// \tparam Arg2 Type of the second argument, and type of the return
  ///   value.  It may differ from the type of the first argument.
  ///
  /// This function object might be defined in the std namespace; it
  /// is an SGI extension to the STL.  We can't count on it living in
  /// the std namespace, but it's useful, so we define it here.
  ///
  /// If you apply <tt>using namespace std;</tt> to the global
  /// namespace, and if your STL implementation includes project1st,
  /// it will cause collisions with this definition.  We recommend for
  /// this and other reasons not to state <tt>using namespace
  /// std;</tt> in the global namespace.
  template<class Arg1, class Arg2>
  class project2nd : public std::binary_function<Arg1, Arg2, Arg2> {
  public:
    typedef Arg1 first_argument_type;
    typedef Arg2 second_argument_type;
    typedef Arg2 result_type;
    Arg2 operator () (const Arg1& , const Arg2& y) const {
      return y;
    }
  };

} // end of Tpetra namespace


// We include this after the above Tpetra namespace declaration,
// so that we don't interfere with Doxygen's ability to find the
// Tpetra namespace declaration.
#include <Tpetra_CombineMode.hpp>


//! Namespace for %Tpetra example classes and methods
namespace TpetraExamples {
}

namespace Tpetra {
  //! Namespace for %Tpetra Reduction/Tranformation Interface
  namespace RTI {
  }
}

namespace Tpetra {
  //! Namespace for external %Tpetra functionality
  namespace Ext {
  }

  /// \brief Distributed sparse matrix-matrix multiply and add.
  ///
  /// This namespace includes functions for computing the sum or product
  /// of two distributed sparse matrices, each of which is represented
  /// as a Tpetra::CrsMatrix.
  namespace MatrixMatrix {
  }
}

namespace Tpetra {
  //! Sweep direction for Gauss-Seidel or Successive Over-Relaxation (SOR).
  enum ESweepDirection {
    Forward = 0,
    Backward,
    Symmetric
  };
}

#if defined(HAVE_TPETRACORE_KOKKOSCORE) && defined(HAVE_TPETRACORE_TEUCHOSKOKKOSCOMPAT) && defined(TPETRA_ENABLE_KOKKOS_DISTOBJECT)
#define TPETRA_USE_KOKKOS_DISTOBJECT 1
#else
#define TPETRA_USE_KOKKOS_DISTOBJECT 0
#endif

#include <Kokkos_Complex.hpp>

// Specializations of Teuchos::SerializationTraits for
// Kokkos::complex<{float,double}>.

namespace Teuchos {
  template<typename Ordinal>
  class SerializationTraits<Ordinal, ::Kokkos::complex<float> >
    : public DirectSerializationTraits<Ordinal, ::Kokkos::complex<float> >
  {};

  template<typename Ordinal>
  class SerializationTraits<Ordinal, ::Kokkos::complex<double> >
    : public DirectSerializationTraits<Ordinal, ::Kokkos::complex<double> >
  {};
} // namespace Teuchos

#endif // TPETRA_CONFIGDEFS_HPP
