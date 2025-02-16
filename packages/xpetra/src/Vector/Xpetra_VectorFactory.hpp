// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
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
#ifndef XPETRA_VECTORFACTORY_HPP
#define XPETRA_VECTORFACTORY_HPP

#include "Xpetra_ConfigDefs.hpp"
#include "Xpetra_Vector.hpp"

#ifdef HAVE_XPETRA_TPETRA
#  include "Xpetra_TpetraVector.hpp"
#endif
#ifdef HAVE_XPETRA_EPETRA
#  include "Xpetra_EpetraVector.hpp"
#  include "Xpetra_EpetraIntVector.hpp"
#endif

#include "Xpetra_Exceptions.hpp"

namespace Xpetra {

  template <class Scalar/* = Vector<>::scalar_type*/,
            class LocalOrdinal/* = typename Vector<Scalar>::local_ordinal_type*/,
            class GlobalOrdinal/* = typename Vector<Scalar, LocalOrdinal>::local_ordinal_type*/,
            class Node/* = typename Vector<Scalar, LocalOrdinal, GlobalOrdinal>::node_type*/>
  class VectorFactory {
#undef XPETRA_VECTORFACTORY_SHORT
#include "Xpetra_UseShortNames.hpp"

  private:
    //! Private constructor. This is a static class.
    VectorFactory() {}

  public:

    //! Constructor specifying the number of non-zeros for all rows.
    static RCP<Vector> Build(const Teuchos::RCP<const Map> &map, bool zeroOut=true) {
      XPETRA_MONITOR("VectorFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (map->lib() == UseTpetra)
        return rcp( new TpetraVector(map, zeroOut) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(map->lib());
      XPETRA_FACTORY_END;
    }

  };
#define XPETRA_VECTORFACTORY_SHORT

  // Specializations on Serial node (mainly used for Epetra)
#ifdef HAVE_XPETRA_SERIAL

  // Specialization for Scalar=double, LO=GO=int and Serial node
  // Used both for Epetra and Tpetra
  // For any other node definition the general default implementation is used which allows Tpetra only
  template <>
  class VectorFactory<double, int, int, Kokkos::Compat::KokkosSerialWrapperNode> {
    typedef double                              Scalar;
    typedef int                                 LocalOrdinal;
    typedef int                                 GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

#undef XPETRA_VECTORFACTORY_SHORT
#include "Xpetra_UseShortNames.hpp"

  private:
    //! Private constructor. This is a static class.
    VectorFactory() {}

  public:

    static RCP<Vector> Build(const Teuchos::RCP<const Map>& map, bool zeroOut=true) {
      XPETRA_MONITOR("VectorFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (map->lib() == UseTpetra)
        return rcp( new TpetraVector(map, zeroOut) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("VectorFactory<int,int>", "TpetraVector<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (map->lib() == UseEpetra)
        return rcp( new EpetraVectorT<int,Node>(map, zeroOut) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

  };

  // Specialization for Scalar=double, LO=int, GO=long long and Serial node
  // Used both for Epetra and Tpetra
  // For any other node definition the general default implementation is used which allows Tpetra only
#ifdef HAVE_XPETRA_INT_LONG_LONG
  template <>
  class VectorFactory<double, int, long long, Kokkos::Compat::KokkosSerialWrapperNode> {

    typedef double                              Scalar;
    typedef int                                 LocalOrdinal;
    typedef long long                           GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

#undef XPETRA_VECTORFACTORY_SHORT
#include "Xpetra_UseShortNames.hpp"

  private:
    //! Private constructor. This is a static class.
    VectorFactory() {}

  public:

    static RCP<Vector> Build(const Teuchos::RCP<const Map>& map, bool zeroOut=true) {
      XPETRA_MONITOR("VectorFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (map->lib() == UseTpetra)
        return rcp( new TpetraVector(map, zeroOut) );
#endif

#if defined(HAVE_XPETRA_EPETRA) && ! defined(XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES)
      if (map->lib() == UseEpetra)
        return rcp( new EpetraVectorT<long long,Node>(map, zeroOut) );
#endif

      XPETRA_FACTORY_END;
    }

  };
#endif // HAVE_XPETRA_INT_LONG_LONG

#define XPETRA_VECTORFACTORY_SHORT

  // Specialization for Scalar=int, LO=GO=int and Serial node
  // Used both for Epetra and Tpetra
  // For any other node definition the general default implementation is used which allows Tpetra only
  template <>
  class VectorFactory<int, int, int, Kokkos::Compat::KokkosSerialWrapperNode> {

    typedef int                                 Scalar;
    typedef int                                 LocalOrdinal;
    typedef int                                 GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

#undef XPETRA_VECTORFACTORY_SHORT
#include "Xpetra_UseShortNames.hpp"

  private:
    //! Private constructor. This is a static class.
    VectorFactory() {}

  public:

    static RCP<Vector> Build(const Teuchos::RCP<const Map>& map, bool zeroOut=true) {
      XPETRA_MONITOR("VectorFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (map->lib() == UseTpetra)
        return rcp( new TpetraVector(map, zeroOut) );
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (map->lib() == UseEpetra)
        return rcp( new EpetraIntVectorT<int,Node>(map, zeroOut) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

  };

  // Specialization for Scalar=int, LO=int, GO=long long and Serial node
  // Used both for Epetra and Tpetra
  // For any other node definition the general default implementation is used which allows Tpetra only
#ifdef HAVE_XPETRA_INT_LONG_LONG
  template <>
  class VectorFactory<int, int, long long, Kokkos::Compat::KokkosSerialWrapperNode> {

    typedef int                                 Scalar;
    typedef int                                 LocalOrdinal;
    typedef long long                           GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

#undef XPETRA_VECTORFACTORY_SHORT
#include "Xpetra_UseShortNames.hpp"

  private:
    //! Private constructor. This is a static class.
    VectorFactory() {}

  public:

    static RCP<Vector> Build(const Teuchos::RCP<const Map>& map, bool zeroOut=true) {
      XPETRA_MONITOR("VectorFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (map->lib() == UseTpetra)
        return rcp( new TpetraVector(map, zeroOut) );
#endif

#if defined(HAVE_XPETRA_EPETRA) && ! defined(XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES)
      if (map->lib() == UseEpetra)
        return rcp( new EpetraIntVectorT<long long,Node>(map, zeroOut) );
#endif

      XPETRA_FACTORY_END;
    }

  };
#endif // HAVE_XPETRA_INT_LONG_LONG
#endif // HAVE_XPETRA_SERIAL
}

#define XPETRA_VECTORFACTORY_SHORT
#endif
