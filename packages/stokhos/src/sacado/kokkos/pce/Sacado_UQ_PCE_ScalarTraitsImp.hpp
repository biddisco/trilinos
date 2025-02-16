// @HEADER
// ***********************************************************************
//
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#ifndef SACADO_UQ_PCE_SCALARTRAITSIMP_HPP
#define SACADO_UQ_PCE_SCALARTRAITSIMP_HPP

#ifdef HAVE_SACADO_TEUCHOS

#include "Teuchos_ScalarTraits.hpp"
#include "Teuchos_SerializationTraits.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_Assert.hpp"
#include "Sacado_mpl_apply.hpp"
#include "Teuchos_as.hpp"

#include <iterator>

namespace Sacado {

  namespace UQ {

    //! Implementation for Teuchos::ScalarTraits for all PCE types
    template <typename PCEType>
    struct PCEScalarTraitsImp {
      typedef typename PCEType::storage_type storage_type;
      typedef typename storage_type::value_type value_type;
      typedef typename storage_type::ordinal_type ordinal_type;

      typedef typename Teuchos::ScalarTraits<value_type>::magnitudeType value_mag_type;
      typedef typename Teuchos::ScalarTraits<value_type>::halfPrecision value_half_type;
      typedef typename Teuchos::ScalarTraits<value_type>::doublePrecision value_double_type;

      typedef typename Sacado::mpl::apply<storage_type,ordinal_type,value_mag_type>::type storage_mag_type;
      typedef typename Sacado::mpl::apply<storage_type,ordinal_type,value_half_type>::type storage_half_type;
      typedef typename Sacado::mpl::apply<storage_type,ordinal_type,value_double_type>::type storage_double_type;

      typedef typename Sacado::mpl::apply<PCEType, storage_mag_type>::type magnitudeType;
      //typedef value_mag_type magnitudeType;
      typedef typename Sacado::mpl::apply<PCEType, storage_half_type>::type halfPrecision;
      typedef typename Sacado::mpl::apply<PCEType, storage_double_type>::type doublePrecision;

      typedef value_type innerProductType;

      static const bool isComplex = Teuchos::ScalarTraits<value_type>::isComplex;
      static const bool isOrdinal = Teuchos::ScalarTraits<value_type>::isOrdinal;
      static const bool isComparable =
        Teuchos::ScalarTraits<value_type>::isComparable;
      static const bool hasMachineParameters =
        Teuchos::ScalarTraits<value_type>::hasMachineParameters;
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType eps() {
        return Teuchos::ScalarTraits<value_type>::eps();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType sfmin() {
        return Teuchos::ScalarTraits<value_type>::sfmin();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType base()  {
        return Teuchos::ScalarTraits<value_type>::base();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType prec()  {
        return Teuchos::ScalarTraits<value_type>::prec();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType t()     {
        return Teuchos::ScalarTraits<value_type>::t();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType rnd()   {
        return Teuchos::ScalarTraits<value_type>::rnd();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType emin()  {
        return Teuchos::ScalarTraits<value_type>::emin();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType rmin()  {
        return Teuchos::ScalarTraits<value_type>::rmin();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType emax()  {
        return Teuchos::ScalarTraits<value_type>::emax();
      }
      KOKKOS_INLINE_FUNCTION
      static typename Teuchos::ScalarTraits<value_type>::magnitudeType rmax()  {
        return Teuchos::ScalarTraits<value_type>::rmax();
      }
      KOKKOS_INLINE_FUNCTION
      static magnitudeType magnitude(const PCEType& a) {
        return a.two_norm();
      }
      KOKKOS_INLINE_FUNCTION
      static innerProductType innerProduct(const PCEType& a, const PCEType& b) {
        return a.inner_product(b);
      }
      KOKKOS_INLINE_FUNCTION
      static PCEType zero()  {
        return PCEType(0.0);
      }
      KOKKOS_INLINE_FUNCTION
      static PCEType one()   {
        return PCEType(1.0);
      }

      // Conjugate is only defined for real derivative components
      KOKKOS_INLINE_FUNCTION
      static PCEType conjugate(const PCEType& x) {
        PCEType y = x;
        y.copyForWrite();
        y.val() = Teuchos::ScalarTraits<value_type>::conjugate(x.val());
        return y;
      }

      // Real part is only defined for real derivative components
      KOKKOS_INLINE_FUNCTION
      static PCEType real(const PCEType& x) {
        PCEType y = x;
        y.copyForWrite();
        y.val() = Teuchos::ScalarTraits<value_type>::real(x.val());
        return y;
      }

      // Imaginary part is only defined for real derivative components
      KOKKOS_INLINE_FUNCTION
      static PCEType imag(const PCEType& x) {
        return PCEType(Teuchos::ScalarTraits<value_type>::imag(x.val()));
      }

      KOKKOS_INLINE_FUNCTION
      static value_type nan() {
        return Teuchos::ScalarTraits<value_type>::nan();
      }
      KOKKOS_INLINE_FUNCTION
      static bool isnaninf(const PCEType& x) {
        for (int i=0; i<x.size(); i++)
          if (Teuchos::ScalarTraits<value_type>::isnaninf(x.fastAccessCoeff(i)))
            return true;
        return false;
      }
      KOKKOS_INLINE_FUNCTION
      static void seedrandom(unsigned int s) {
        Teuchos::ScalarTraits<value_type>::seedrandom(s);
      }
      KOKKOS_INLINE_FUNCTION
      static value_type random() {
        return Teuchos::ScalarTraits<value_type>::random();
      }
      KOKKOS_INLINE_FUNCTION
      static const char * name() {
        return "Sacado::UQ::PCE<>";
      }
      KOKKOS_INLINE_FUNCTION
      static PCEType squareroot(const PCEType& x) {
        return std::sqrt(x);
      }
      KOKKOS_INLINE_FUNCTION
      static PCEType pow(const PCEType& x, const PCEType& y) {
        return std::pow(x,y);
      }
      KOKKOS_INLINE_FUNCTION
      static PCEType log(const PCEType& x) {
        return std::log(x);
      }
      KOKKOS_INLINE_FUNCTION
      static PCEType log10(const PCEType& x) {
        return std::log10(x);
      }

      // Helper function to determine whether a complex value is real
      KOKKOS_INLINE_FUNCTION
      static bool is_complex_real(const value_type& x) {
        return
          Teuchos::ScalarTraits<value_type>::magnitude(x-Teuchos::ScalarTraits<value_type>::real(x)) == 0;
      }

      // Helper function to determine whether a Fad type is real
      KOKKOS_INLINE_FUNCTION
      static bool is_pce_real(const PCEType& x) {
        if (x.size() == 0)
          return true;
        if (Teuchos::ScalarTraits<value_type>::isComplex) {
          for (int i=0; i<x.size(); i++)
            if (!is_complex_real(x.fastAccessCoeff(i)))
              return false;
        }
        return true;
      }

    }; // class PCEScalarTraitsImp

    //! Implementation for Teuchos::ValueTypeConversionTraits for all PCE types
    template <typename TypeTo, typename PCEType>
    struct PCEValueTypeConversionTraitsImp {
      typedef typename Sacado::ValueType<PCEType>::type ValueT;
      typedef Teuchos::ValueTypeConversionTraits<TypeTo,ValueT> VTCT;
      static TypeTo convert( const PCEType t ) {
        return VTCT::convert(t.val());
      }
      static TypeTo safeConvert( const PCEType t ) {
        return VTCT::safeConvert(t.val());
      }
    };

    //! Implementation of Teuchos::SerializationTraits for all PCE types
    template <typename Ordinal, typename PCEType>
    class PCESerializationTraitsImp {
      typedef typename Sacado::ValueType<PCEType>::type ValueT;
      typedef Teuchos::SerializationTraits<Ordinal,ValueT> vSerT;
      typedef Teuchos::SerializationTraits<Ordinal,int> iSerT;
      typedef Teuchos::SerializationTraits<Ordinal,Ordinal> oSerT;

    public:

      /// \brief Whether the type T supports direct serialization.
      static const bool supportsDirectSerialization = false;

      //! @name Indirect serialization functions (always defined and supported)
      //@{

      /** \brief Return the number of bytes for <tt>count</tt> objects. */
      static Ordinal fromCountToIndirectBytes(const Ordinal count,
                                              const PCEType buffer[]) {
        Ordinal bytes = 0;
        for (Ordinal i=0; i<count; i++) {
          int sz = buffer[i].size();
          Ordinal b1 = iSerT::fromCountToIndirectBytes(1, &sz);
          Ordinal b2 = vSerT::fromCountToIndirectBytes(sz, buffer[i].coeff());
          Ordinal b3 = oSerT::fromCountToIndirectBytes(1, &b2);
          bytes += b1+b2+b3;
        }
        return bytes;
      }

      /** \brief Serialize to an indirect <tt>char[]</tt> buffer. */
      static void serialize (const Ordinal count,
                             const PCEType buffer[],
                             const Ordinal bytes,
                             char charBuffer[]) {
        for (Ordinal i=0; i<count; i++) {
          // First serialize size
          int sz = buffer[i].size();
          Ordinal b1 = iSerT::fromCountToIndirectBytes(1, &sz);
          iSerT::serialize(1, &sz, b1, charBuffer);
          charBuffer += b1;

          // Next serialize PCE coefficients
          Ordinal b2 = vSerT::fromCountToIndirectBytes(sz, buffer[i].coeff());
          Ordinal b3 = oSerT::fromCountToIndirectBytes(1, &b2);
          oSerT::serialize(1, &b2, b3, charBuffer);
          charBuffer += b3;
          vSerT::serialize(sz, buffer[i].coeff(), b2, charBuffer);
          charBuffer += b2;
        }
      }

      /** \brief Return the number of objects for <tt>bytes</tt> of storage. */
      static Ordinal fromIndirectBytesToCount(const Ordinal bytes,
                                              const char charBuffer[]) {
        Ordinal count = 0;
        Ordinal bytes_used = 0;
        while (bytes_used < bytes) {

          // Bytes for size
          Ordinal b1 = iSerT::fromCountToDirectBytes(1);
          bytes_used += b1;
          charBuffer += b1;

          // Bytes for PCE coefficients
          Ordinal b3 = oSerT::fromCountToDirectBytes(1);
          const Ordinal *b2 = oSerT::convertFromCharPtr(charBuffer);
          bytes_used += b3;
          charBuffer += b3;
          bytes_used += *b2;
          charBuffer += *b2;

          ++count;
        }
        return count;
      }

      /** \brief Deserialize from an indirect <tt>char[]</tt> buffer. */
      static void deserialize (const Ordinal bytes,
                               const char charBuffer[],
                               const Ordinal count,
                               PCEType buffer[]) {
        for (Ordinal i=0; i<count; i++) {

          // Deserialize size
          Ordinal b1 = iSerT::fromCountToDirectBytes(1);
          const int *sz = iSerT::convertFromCharPtr(charBuffer);
          charBuffer += b1;

          // Make sure PCE object is ready to receive values
          // We assume it has already been initialized with the proper
          // cijk object
          if (buffer[i].size() != *sz)
            buffer[i].reset(buffer[i].cijk(), *sz);
          buffer[i].copyForWrite();

          // Deserialize PCE coefficients
          Ordinal b3 = oSerT::fromCountToDirectBytes(1);
          const Ordinal *b2 = oSerT::convertFromCharPtr(charBuffer);
          charBuffer += b3;
          vSerT::deserialize(*b2, charBuffer, *sz, buffer[i].coeff());
          charBuffer += *b2;
        }

      }

      //@}

    };


    //! Serializer object for all PCE types
    template <typename Ordinal, typename PCEType, typename ValueSerializer>
    class PCESerializerImp {

    public:

      //! Typename of value serializer
      typedef ValueSerializer value_serializer_type;

      //! Typename of cijk
      typedef typename PCEType::cijk_type cijk_type;


    protected:
      typedef typename Sacado::ValueType<PCEType>::type ValueT;
      typedef Teuchos::SerializationTraits<Ordinal,int> iSerT;
      typedef Teuchos::SerializationTraits<Ordinal,Ordinal> oSerT;

      cijk_type cijk;
      Teuchos::RCP<const ValueSerializer> vs;
      int sz;

    public:

      /// \brief Whether the type T supports direct serialization.
      static const bool supportsDirectSerialization = false;

      PCESerializerImp(const cijk_type& cijk_,
                       const Teuchos::RCP<const ValueSerializer>& vs_) :
        cijk(cijk_), vs(vs_), sz(cijk.dimension()) {}

      //! Return specified serializer size
      cijk_type getSerializerCijk() const { return cijk; }

      //! Get nested value serializer
      Teuchos::RCP<const value_serializer_type> getValueSerializer() const {
        return vs; }

      //! @name Indirect serialization functions (always defined and supported)
      //@{

      /** \brief Return the number of bytes for <tt>count</tt> objects. */
      Ordinal fromCountToIndirectBytes(const Ordinal count,
                                       const PCEType buffer[]) const {
        Ordinal bytes = 0;
        PCEType *x = NULL;
        const PCEType *cx;
        for (Ordinal i=0; i<count; i++) {
          int my_sz = buffer[i].size();
          if (sz != my_sz) {
            if (x == NULL)
              x = new PCEType;
            *x = buffer[i];
            x->reset(cijk);
            cx = x;
          }
          else
            cx = &(buffer[i]);
          Ordinal b1 = iSerT::fromCountToIndirectBytes(1, &sz);
          Ordinal b2 = vs->fromCountToIndirectBytes(sz, cx->coeff());
          Ordinal b3 = oSerT::fromCountToIndirectBytes(1, &b2);
          bytes += b1+b2+b3;
        }
        if (x != NULL)
          delete x;
        return bytes;
      }

      /** \brief Serialize to an indirect <tt>char[]</tt> buffer. */
      void serialize (const Ordinal count,
                      const PCEType buffer[],
                      const Ordinal bytes,
                      char charBuffer[]) const {
        PCEType *x = NULL;
        const PCEType *cx;
        for (Ordinal i=0; i<count; i++) {
          // First serialize size
          int my_sz = buffer[i].size();
          if (sz != my_sz) {
            if (x == NULL)
              x = new PCEType(cijk);
            *x = buffer[i];
            x->reset(cijk);
            cx = x;
          }
          else
            cx = &(buffer[i]);
          Ordinal b1 = iSerT::fromCountToIndirectBytes(1, &sz);
          iSerT::serialize(1, &sz, b1, charBuffer);
          charBuffer += b1;

          // Next serialize PCE coefficients
          Ordinal b2 = vs->fromCountToIndirectBytes(sz, cx->coeff());
          Ordinal b3 = oSerT::fromCountToIndirectBytes(1, &b2);
          oSerT::serialize(1, &b2, b3, charBuffer);
          charBuffer += b3;
          vs->serialize(sz, cx->coeff(), b2, charBuffer);
          charBuffer += b2;
        }
        if (x != NULL)
          delete x;
      }

      /** \brief Return the number of objects for <tt>bytes</tt> of storage. */
      Ordinal fromIndirectBytesToCount(const Ordinal bytes,
                                       const char charBuffer[]) const {
        Ordinal count = 0;
        Ordinal bytes_used = 0;
        while (bytes_used < bytes) {

          // Bytes for size
          Ordinal b1 = iSerT::fromCountToDirectBytes(1);
          bytes_used += b1;
          charBuffer += b1;

          // Bytes for PCE coefficients
          Ordinal b3 = oSerT::fromCountToDirectBytes(1);
          const Ordinal *b2 = oSerT::convertFromCharPtr(charBuffer);
          bytes_used += b3;
          charBuffer += b3;
          bytes_used += *b2;
          charBuffer += *b2;

          ++count;
        }
        return count;
      }

      /** \brief Deserialize from an indirect <tt>char[]</tt> buffer. */
      void deserialize (const Ordinal bytes,
                        const char charBuffer[],
                        const Ordinal count,
                        PCEType buffer[]) const {
        for (Ordinal i=0; i<count; i++) {

          // Deserialize size
          Ordinal b1 = iSerT::fromCountToDirectBytes(1);
          const int *my_sz = iSerT::convertFromCharPtr(charBuffer);
          charBuffer += b1;

          // Create empty PCE object of given size
          buffer[i].reset(cijk);

          // Deserialize PCE coefficients
          Ordinal b3 = oSerT::fromCountToDirectBytes(1);
          const Ordinal *b2 = oSerT::convertFromCharPtr(charBuffer);
          charBuffer += b3;
          vs->deserialize(*b2, charBuffer, *my_sz, buffer[i].coeff());
          charBuffer += *b2;
        }

      }

      //@}

    };

  } // namespace UQ

} // namespace Sacado

#endif // HAVE_SACADO_TEUCHOS

#endif // SACADO_FAD_SCALARTRAITSIMP_HPP
