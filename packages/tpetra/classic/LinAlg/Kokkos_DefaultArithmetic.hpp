//@HEADER
// ************************************************************************
//
//          Kokkos: Node API and Parallel Node Kernels
//              Copyright (2008) Sandia Corporation
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
//@HEADER

#ifndef KOKKOS_DEFAULTARITHMETIC_H
#define KOKKOS_DEFAULTARITHMETIC_H

/// \file Kokkos_DefaultArithmetic.hpp
/// \brief Traits class for local multivector operations.

#include "Kokkos_DefaultNode.hpp"
#include "Kokkos_NodeHelpers.hpp" // ReadyBufferHelper
#include "Kokkos_MultiVector.hpp"
#include "Kokkos_MultiVectorKernelOps.hpp"

#include "Teuchos_ArrayView.hpp"
#include "Teuchos_Assert.hpp"
#include "Teuchos_BLAS.hpp"
#include "Teuchos_BLAS_types.hpp"
#include "Teuchos_Tuple.hpp"
#include "Teuchos_TypeNameTraits.hpp"

#include <stdexcept>


namespace KokkosClassic {

  // Class for providing GEMM for a particular Node
  template <typename Scalar, typename Node>
  struct NodeGEMM {
    public:
      static void GEMM(Teuchos::ETransp transA, Teuchos::ETransp transB, Scalar alpha, const MultiVector<Scalar,Node> &A, const MultiVector<Scalar,Node> &B, Scalar beta, MultiVector<Scalar,Node> &C) {
        Teuchos::BLAS<int,Scalar> blas;
        const int m = Teuchos::as<int>(C.getNumRows()),
                  n = Teuchos::as<int>(C.getNumCols()),
                  k = (transA == Teuchos::NO_TRANS ? A.getNumCols() : A.getNumRows()),
                  lda = Teuchos::as<int>(A.getStride()),
                  ldb = Teuchos::as<int>(B.getStride()),
                  ldc = Teuchos::as<int>(C.getStride());
        // For some BLAS implementations (i.e. MKL), GEMM when B has one column
        // is signficantly less efficient
        if (n == 1 && transB == Teuchos::NO_TRANS)
          blas.GEMV(transA, A.getNumRows(), A.getNumCols(), alpha, A.getValues().getRawPtr(), lda, B.getValues().getRawPtr(), Teuchos::as<int>(1), beta, C.getValuesNonConst().getRawPtr(), Teuchos::as<int>(1));
        else
          blas.GEMM(transA, transB, m, n, k, alpha, A.getValues().getRawPtr(), lda, B.getValues().getRawPtr(), ldb, beta, C.getValuesNonConst().getRawPtr(), ldc);
      }
  };

  /// \class DefaultArithmeticBase
  /// \brief Base class for DefaultArithmetic; not for users of the latter.
  ///
  /// \tparam MV The local multivector type.  We provide a
  ///   specialization for MultiVector.
  template <class MV>
  class DefaultArithmeticBase {
  public:
    /// \brief Compute the matrix-matrix product <tt>C = alpha*Op(A)*Op(B) + beta*C</tt>.
    ///
    /// \c Op(A) may be either \c A, its transpose, or its conjugate
    /// transpose, depending on \c transA.  Likewise, \c Op(B) may be
    /// either \c B, its transpose, or its conjugate transpose,
    /// depending on \c transB.
    static void
    GEMM (MV &C, Teuchos::ETransp transA, Teuchos::ETransp transB,
          typename MV::ScalarType alpha, const MV &A,
          const MV &B, typename MV::ScalarType beta);

    //! Fill \c A with uniform random numbers.
    static void Random (MV& A);

    /// \brief Tell \c A about its dimensions, and give it a pointer to its data.
    ///
    /// \param A [out] The multivector to tell about its dimensions and data.
    /// \param numRows [in] Number of rows in A.
    /// \param numCols [in] Number of columns in A.
    /// \param values [in] Pointer to A's data.  This is a matrix
    ///   stored in column-major format.
    /// \param stride [in] Stride between columns of the matrix.
    ///
    /// \pre <tt>stride >= numRows</tt>
    static void
    initializeValues (MV &A, size_t numRows, size_t numCols,
                      const ArrayRCP<typename MV::ScalarType> &values,
                      size_t stride);

    /// Tell \c A about its dimensions and original dimensions, and
    /// give it a pointer to its data.
    ///
    /// Use this version of initializeValues when the MV to initialize
    /// is actually a view of another MV.  Keeping the original
    /// dimensions lets you do error checking correctly, especially
    /// when going from a subset view (a view of a subset of rows) to
    /// a superset of the subset.  This is an important case for
    /// Tpetra, e.g., when making domain Map vectors that are actually
    /// views of column Map vectors, then getting the original column
    /// Map vector back.  (This makes things like Import and local
    /// Gauss-Seidel more efficient.)
    ///
    /// \param A [out] The multivector to tell about its dimensions and data.
    /// \param numRows [in] Number of rows in A.
    /// \param numCols [in] Number of columns in A.
    /// \param values [in] Pointer to A's data.  This is a matrix
    ///   stored in column-major format.
    /// \param stride [in] Stride between columns of the matrix.
    /// \param origNumRows [in] Original number of rows in the
    ///   multivector (of which A is to be a view).
    /// \param origNumCols [in] Original number of rows in the
    ///   multivector (of which A is to be a view).
    ///
    /// \pre <tt>stride >= numRows</tt>
    /// \pre <tt>stride >= origNumRows</tt>
    static void
    initializeValues (MV &A, size_t numRows, size_t numCols,
                      const ArrayRCP<typename MV::ScalarType> &values,
                      size_t stride,
                      size_t origNumRows,
                      size_t origNumCols);

    //! Get a const pointer to A's data; the same pointer set by initializeValues().
    static ArrayRCP<const typename MV::ScalarType> getValues (const MV &A);

    //! Get a const pointer to the data of column \c j of \c A.
    static ArrayRCP<const typename MV::ScalarType>
    getValues (const MV &A, size_t j);

    //! Get a nonconst pointer to A's data; the same pointer set by initializeValues().
    static ArrayRCP<typename MV::ScalarType> getValuesNonConst (MV &A);

    //! Get a nonconst pointer to the data of column \c j of \c A.
    static ArrayRCP<typename MV::ScalarType>
    getValuesNonConst (const MV &A, size_t j);

    //! The number of rows in \c A.
    static size_t getNumRows (const MV &A);

    //! The number of columns in \c A.
    static size_t getNumCols (const MV &A);

    //! The (column) stride of \c A.
    static size_t getStride (const MV &A);

    /// \brief "Original" number of rows (of the multivector of
    ///   which A is a view).
    ///
    /// If A is <i>not</i> a view of another multivector, then this
    /// method just returns the number of rows.
    static size_t getOrigNumRows (const MV &A);

    /// \brief "Original" number of columns (of the multivector of
    ///   which A is a view).
    ///
    /// If A is <i>not</i> a view of another multivector, then this
    /// method just returns the number of columns.
    static size_t getOrigNumCols (const MV &A);

    //! The Kokkos Node instance with which \c A was created.
    static RCP<typename MV::NodeType> getNode (const MV &A);
  };


  /// \class DefaultArithmetic
  /// \brief Traits class providing a generic arithmetic interface for local multivectors.
  ///
  /// \tparam MV The local multivector type.  We provide a
  ///   specialization for MultiVector.
  template <class MV>
  class DefaultArithmetic : public DefaultArithmeticBase<MV> {
  public:
    //! Initialize all entries of \c A to the given constant value \c alpha.
    static void Init (MV& A, typename MV::ScalarType alpha);

    //! Set A to the reciprocal of B: <tt>B(i,j) = 1/A(i,j)</tt>.
    static void Recip (MV& A, const MV& B);

    /// \brief A threshold, in-place variant of Recip().
    ///
    /// For each element A(i,j) of A, set A(i,j) = 1/A(i,j) if the
    /// magnitude of A(i,j) is greater than or equal to the magnitude
    /// of minDiagVal.  Otherwise, set A(i,j) to minDiagVal.
    static void
    ReciprocalThreshold (MV& A, typename MV::ScalarType& minDiagVal);

    /// \brief Set C to the scaled element-wise multiple of A and B.
    ///
    /// <tt>C(i,j) = scalarC * C(i,j) + scalarAB * B(i,j) * A(i,1)</tt>,
    /// where the input multivector A has only 1 column.  If scalarC
    /// is zero, this method ignores the initial contents of C, even
    /// if there are NaN entries.
    static void
    ElemMult (MV& C,
              typename MV::ScalarType scalarC,
              typename MV::ScalarType scalarAB,
              const MV& A,
              const MV& B);

    /// \brief Assign B to A: <tt>A(i,j) = B(i,j)</tt>.
    ///
    /// If A and B point to the same data, then this function skips
    /// the assignment entirely.
    static void Assign (MV& A, const MV& B);

    /// \brief Assign the given columns of B to A.
    ///
    /// This assigns <tt>A(i, j) = B(i, whichVectors[j])</tt>
    /// for i in <tt>[0, getNumRows(A)]</tt> and
    /// j in <tt>[0, getNumCols(A)]</tt>.
    ///
    /// If for any j, <tt>A(0,j)</tt> and
    /// <tt>B(0, whichVectors[j])</tt> point to the same data,
    /// then this function skips the assignment for that column.
    static void Assign (MV& A, const MV& B, const ArrayView<const size_t>& whichVectors);

    //! Compute the inner products of corresponding columns of A and B.
    static void Dot (const MV& A, const MV& B, const ArrayView<typename MV::ScalarType> &dots);

    //! Compute the inner product of A and B (assuming each has only one column).
    static typename MV::ScalarType Dot (const MV& A, const MV& B);

    /// \brief Compute <tt>B = alpha * A + beta * B</tt>.
    ///
    /// If beta is zero, overwrite B, even if it contains NaN entries.
    static void
    GESUM (MV& B, typename MV::ScalarType alpha, const MV& A, typename MV::ScalarType beta);

    /// \brief Compute <tt>C = alpha * A + beta * B + gamma * C</tt>.
    ///
    /// If gamma is zero, overwrite C, even if it contains NaN entries.
    static void
    GESUM (MV &C, typename MV::ScalarType alpha, const MV &A,
           typename MV::ScalarType beta, const MV &B, typename MV::ScalarType gamma);

    //! Compute the one-norm of each column of \c A.
    static void Norm1 (const MV &A, const ArrayView<typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType> &norms);

    //! Compute the one-norm of (the first column of) \c A.
    static typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType Norm1 (const MV &A);

    //! Compute the sum of each column of \c A.
    static void Sum (const MV &A, const ArrayView<typename MV::ScalarType> &sums);

    //! Compute the sum of (the first column of) \c A.
    static typename MV::ScalarType Sum (const MV& A);

    //! Compute the infinity norm (element of maximum magnitude) of each column of \c A.
    static void NormInf (const MV& A, const ArrayView<typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType> &norms);

    //! Compute the infinity norm (element of maximum magnitude) of (the first column of) \c A.
    static typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType
    NormInf (const MV& A);

    //! Compute the square of the 2-norm of each column of \c A.
    static void Norm2Squared (const MV &A, const ArrayView<typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType> &norms);

    //! Compute the square of the 2-norm of (the first column of) \c A.
    static typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType
    Norm2Squared (const MV& A);

    //! Compute the norm of (the first column of) \c A, weighted by the given vector of weights.
    static typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType
    WeightedNorm (const MV &A, const MV &weightVector);

    //! Compute the norm of each column of \c A, weighted by the given vector of weights.
    static void WeightedNorm (const MV &A, const MV &weightVector, const ArrayView<typename Teuchos::ScalarTraits<typename MV::ScalarType>::magnitudeType> &norms);

    //! Compute <tt>A = abs(B)</tt>, elementwise.
    static void Abs (MV &A, const MV &B);

    //! Compute <tt>B = alpha * A</tt>.
    static void Scale (MV &B, typename MV::ScalarType alpha, const MV &A);

    //! Scale \c A in place by \c alpha: <tt>A = alpha * A</tt>.
    static void Scale (MV &A, typename MV::ScalarType alpha);
  };

  /// \brief Partial specialization of DefaultArithmeticBase for MultiVector<Scalar,Node>.
  ///
  /// \tparam Scalar The type of entries of the multivector.
  /// \tparam The Kokkos Node type.
  template <class Scalar, class Node>
  class DefaultArithmeticBase<MultiVector<Scalar,Node> > {
  public:
    static void
    GEMM (MultiVector<Scalar,Node> &C,
          Teuchos::ETransp transA,
          Teuchos::ETransp transB,
          Scalar alpha,
          const MultiVector<Scalar,Node> &A,
          const MultiVector<Scalar,Node> &B,
          Scalar beta)
    {
      NodeGEMM<Scalar,Node>::GEMM(transA, transB, alpha, A, B, beta, C);
    }

    static void Random(MultiVector<Scalar,Node> &A) {
      // TODO: consider adding rand() functionality to node
      // in the meantime, just generate random numbers via Teuchos and then copy to node
      typedef Teuchos::ScalarTraits<Scalar> SCT;
      const size_t stride = A.getStride();
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      if (nR*nC == 0) return;
      RCP<Node> node = A.getNode();
      ArrayRCP<Scalar> Adata = A.getValuesNonConst();
      // we'll overwrite all data covered by the multivector, but not off-stride data
      // therefore, we are write-only only in the case that stride=nR
      ReadWriteOption rw = (stride == nR ? WriteOnly : ReadWrite);
      ArrayRCP<Scalar> mvdata = node->template viewBufferNonConst<Scalar>(rw,stride*(nC-1)+nR,Adata);
      for (size_t j=0; j<nC; ++j) {
        for (size_t i=0; i<nR; ++i) {
          mvdata[j*stride + i] = SCT::random();
        }
      }
      mvdata = null;
    }

    inline static void
    initializeValues (MultiVector<Scalar,Node> &A,
                      size_t numRows, size_t numCols,
                      const ArrayRCP<Scalar> &values,
                      size_t stride)
    {
      A.initializeValues(numRows,numCols,values,stride);
    }

    inline static void
    initializeValues (MultiVector<Scalar,Node> &A,
                      size_t numRows,
                      size_t numCols,
                      const ArrayRCP<Scalar> &values,
                      size_t stride,
                      size_t origNumRows,
                      size_t origNumCols)
    {
      A.initializeValues(numRows,numCols,values,stride,origNumRows,origNumCols);
    }

    inline static ArrayRCP<const Scalar> getValues(const MultiVector<Scalar,Node> &A) {
      return A.getValues();
    }

    inline static ArrayRCP<const Scalar> getValues(const MultiVector<Scalar,Node> &A, size_t j) {
      return A.getValues(j);
    }

    inline static ArrayRCP<Scalar> getValuesNonConst(MultiVector<Scalar,Node> &A) {
      return A.getValuesNonConst();
    }

    inline static ArrayRCP<Scalar> getValuesNonConst(MultiVector<Scalar,Node> &A, size_t j) {
      return A.getValuesNonConst(j);
    }

    inline static size_t getNumRows(const MultiVector<Scalar,Node> &A) {
      return A.getNumRows();
    }

    inline static size_t getNumCols(const MultiVector<Scalar,Node> &A) {
      return A.getNumCols();
    }

    inline static size_t getStride(const MultiVector<Scalar,Node> &A) {
      return A.getStride();
    }

    inline static size_t getOrigNumRows (const MultiVector<Scalar,Node> &A) {
      return A.getOrigNumRows ();
    }

    inline static size_t getOrigNumCols (const MultiVector<Scalar,Node> &A) {
      return A.getOrigNumCols ();
    }

    inline static RCP<Node> getNode(const MultiVector<Scalar,Node> &A) {
      return A.getNode();
    }
  };

  /// \brief Partial specialization of DefaultArithmetic for MultiVector<Scalar,Node>.
  ///
  /// Tpetra::MultiVector uses this as a traits class for
  /// KokkosClassic::MultiVector, to implement all of its computational
  /// kernels.
  ///
  /// \tparam Scalar The type of entries of the multivector.
  /// \tparam The Kokkos Node type.
  template <class Scalar, class Node>
  class DefaultArithmetic<MultiVector<Scalar, Node> > :
    public DefaultArithmeticBase<MultiVector<Scalar, Node> > {
  public:
    static void Init (MultiVector<Scalar,Node> &A, Scalar alpha) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      if (nR*nC == 0) return;
      const size_t stride = A.getStride();
      RCP<Node> node = A.getNode();
      ArrayRCP<Scalar> data = A.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addNonConstBuffer<Scalar>(data);
      rbh.end();
      // prepare op
      InitOp<Scalar> wdp;
      wdp.alpha = alpha;
      if (stride == nR) {
        // one kernel invocation for whole multivector
        wdp.x = data(0,nR*nC).getRawPtr();
        node->template parallel_for<InitOp<Scalar> >(0,nR*nC,wdp);
      }
      else {
        // one kernel invocation for each column
        for (size_t j=0; j<nC; ++j) {
          wdp.x = data(0,nR).getRawPtr();
          node->template parallel_for<InitOp<Scalar> >(0,nR,wdp);
          data += stride;
        }
      }
    }

    static void
    Recip (MultiVector<Scalar,Node> &A,
           const MultiVector<Scalar,Node> &B)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC != B.getNumCols() || nR != B.getNumRows(),
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Recip(A,B): "
        "A and B must have the same dimensions.");

      RCP<Node> node = B.getNode();
      ArrayRCP<const Scalar> Bdata = B.getValues();
      ArrayRCP<Scalar>       Adata = A.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addNonConstBuffer<Scalar>(Adata);
      rbh.end();
      RecipOp<Scalar> wdp;
      if (A.getStride() == nR && B.getStride() == nR) {
        // one kernel invocation for whole multivector
        wdp.scale = Bdata(0,nR*nC).getRawPtr();
        wdp.x = Adata(0,nR*nC).getRawPtr();
        node->template parallel_for<RecipOp<Scalar> >(0,nR*nC,wdp);
      }
      else {
        // one kernel invocation for each column
        for (size_t j=0; j<nC; ++j) {
          wdp.x = Adata(0,nR).getRawPtr();
          wdp.scale = Bdata(0,nR).getRawPtr();
          node->template parallel_for<RecipOp<Scalar> >(0,nR,wdp);
          Adata += Astride;
          Bdata += Bstride;
        }
      }
    }

    static void
    ReciprocalThreshold (MultiVector<Scalar,Node>& A,
                         const Scalar& minDiagVal)
    {
      const size_t numRows = A.getNumRows ();
      const size_t numCols = A.getNumCols ();
      const size_t stride = A.getStride ();
      ArrayRCP<Scalar> A_data = A.getValuesNonConst ();
      Scalar* const A_ptr = A_data.getRawPtr ();

      RCP<Node> node = A.getNode ();
      ReadyBufferHelper<Node> rbh (node);
      rbh.begin();
      rbh.template addNonConstBuffer<Scalar> (A_data);
      rbh.end();

      if (stride == numRows) {
        // One kernel invocation for all columns of the multivector.
        typedef ReciprocalThresholdOp<Scalar> op_type;
        op_type wdp (A_ptr, minDiagVal);
        node->template parallel_for<op_type> (0, numRows*numCols, wdp);
      }
      else {
        // One kernel invocation for each column of the multivector.
        for (size_t j = 0; j < numCols; ++j) {
          typedef ReciprocalThresholdOp<Scalar> op_type;
          Scalar* const A_j = A_ptr + j * stride;
          op_type wdp (A_j, minDiagVal);
          node->template parallel_for<op_type> (0, numRows, wdp);
        }
      }
    }

    static void
    ElemMult (MultiVector<Scalar,Node> &C,
              Scalar scalarC,
              Scalar scalarAB,
              const MultiVector<Scalar,Node> &A,
              const MultiVector<Scalar,Node> &B)
    {
      const size_t nR_A = A.getNumRows();
      const size_t nC_A = A.getNumCols();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC_A != 1, std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A)
        << ">::ElemMult(C,sC,sAB,A,B): A must have just 1 column.");

      const size_t Cstride = C.getStride();
      const size_t Bstride = B.getStride();
      const size_t nC_C = C.getNumCols();
      const size_t nR_C = C.getNumRows();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC_C != B.getNumCols() || nR_A != B.getNumRows() || nR_C != B.getNumRows(),
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::ElemMult"
        "(C,sC,sAB,A,B): A, B and C must have the same number of rows, "
        "and B and C must have the same number of columns.");

      RCP<Node> node = B.getNode();
      ArrayRCP<Scalar> Cdata = C.getValuesNonConst();
      ArrayRCP<const Scalar> Bdata = B.getValues();
      ArrayRCP<const Scalar> Adata = A.getValues();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addNonConstBuffer<Scalar>(Cdata);
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();

      if (scalarC == Teuchos::ScalarTraits<Scalar>::zero ()) {
        MVElemMultOverwriteOp<Scalar> wdp;
        wdp.scalarYZ = scalarAB;
        // one kernel invocation for each column
        for (size_t j=0; j<nC_C; ++j) {
          wdp.x = Cdata(0,nR_C).getRawPtr();
          wdp.y = Adata(0,nR_C).getRawPtr();
          wdp.z = Bdata(0,nR_C).getRawPtr();
          node->template parallel_for<MVElemMultOverwriteOp<Scalar> >(0,nR_C,wdp);
          Cdata += Cstride;
          Bdata += Bstride;
        }
      }
      else {
        MVElemMultOp<Scalar> wdp;
        wdp.scalarX = scalarC;
        wdp.scalarYZ = scalarAB;
        // one kernel invocation for each column
        for (size_t j=0; j<nC_C; ++j) {
          wdp.x = Cdata(0,nR_C).getRawPtr();
          wdp.y = Adata(0,nR_C).getRawPtr();
          wdp.z = Bdata(0,nR_C).getRawPtr();
          node->template parallel_for<MVElemMultOp<Scalar> >(0,nR_C,wdp);
          Cdata += Cstride;
          Bdata += Bstride;
        }
      }
    }

    static void
    Assign (MultiVector<Scalar,Node> &A,
            const MultiVector<Scalar,Node> &B)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC != B.getNumCols() || nR != B.getNumRows(),
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Assign(A,B): "
        "The MultiVectors A and B do not have the same dimensions.  "
        "A is " << nR << " x " << nC << ", but B is "
        << B.getNumRows() << " x " << B.getNumCols() << ".");
      if (nC*nR == 0) {
        return; // Nothing to do!
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Bdata = B.getValues();
      ArrayRCP<Scalar>       Adata = A.getValuesNonConst();

      // If A and B are the same pointer, just return without doing
      // anything.  This can make the implementation of
      // Tpetra::MultiVector::copyAndPermute more concise.
      if (Adata.getRawPtr () == Bdata.getRawPtr ()) {
        return;
      }

      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addNonConstBuffer<Scalar>(Adata);
      rbh.end();
      // prepare op
      AssignOp<Scalar> wdp;
      if (Astride == nR && Bstride == nR) {
        // one kernel invocation for whole multivector assignment
        wdp.x = Adata(0,nR*nC).getRawPtr();
        wdp.y = Bdata(0,nR*nC).getRawPtr();
        node->template parallel_for<AssignOp<Scalar> >(0,nR*nC,wdp);
      }
      else {
        // one kernel invocation for each column
        for (size_t j=0; j<nC; ++j) {
          wdp.x = Adata(0,nR).getRawPtr();
          wdp.y = Bdata(0,nR).getRawPtr();
          node->template parallel_for<AssignOp<Scalar> >(0,nR,wdp);
          Adata += Astride;
          Bdata += Bstride;
        }
      }
    }

    static void
    Assign (MultiVector<Scalar,Node>& A,
            const MultiVector<Scalar,Node>& B,
            const ArrayView<const size_t>& whichVectors)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t numColsToCopy = static_cast<size_t> (whichVectors.size ());
      TEUCHOS_TEST_FOR_EXCEPTION(
        nR != B.getNumRows() || numColsToCopy > nC,
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Assign(A,B,"
        "whichVectors): The KokkosClassic::MultiVector inputs A and B(whichVectors) "
        "do not have compatible dimensions.  "
        "A is " << nR << " x " << nC << ", but B has "
        << B.getNumRows() << ", and there are " << numColsToCopy
        << " columns of B to copy into A.");
      if (nR == 0 || numColsToCopy == 0) {
        return; // Nothing to do!
      }

      RCP<Node> node = A.getNode();

      // Make sure that the buffers don't go out of scope until the
      // kernels are done.
      ReadyBufferHelper<Node> rbh (node);
      rbh.begin();
      rbh.template addNonConstBuffer<Scalar> (A.getValuesNonConst ());
      rbh.template addConstBuffer<Scalar> (B.getValues ());
      rbh.end();

      AssignOp<Scalar> wdp; // Reuse the struct for each loop iteration.
      // One kernel invocation for each column of B to copy.
      for (size_t j = 0; j < numColsToCopy; ++j) {
        wdp.x = A.getValuesNonConst (j).getRawPtr ();
        wdp.y = B.getValues (whichVectors[j]).getRawPtr ();

        // Skip columns that alias one another.
        if (wdp.x != wdp.y) {
          node->template parallel_for<AssignOp<Scalar> > (0, nR, wdp);
        }
      }
    }

    static void
    Dot (const MultiVector<Scalar,Node> &A,
         const MultiVector<Scalar,Node> &B,
         const ArrayView<Scalar> &dots)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC != B.getNumCols () || nR != B.getNumRows (), std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName (A) << ">::Dot(A,B,dots): "
        "A and B must have the same dimensions.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC > Teuchos::as<size_t> (dots.size ()), std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName (A) << ">::Dot(A,B,dots): "
        "dots must have length as large as number of columns of A and B.");
      if (nR*nC == 0) {
        std::fill (dots.begin(), dots.begin() + nC,
                   Teuchos::ScalarTraits<Scalar>::zero ());
        return;
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Bdata = B.getValues();
      ArrayRCP<const Scalar> Adata = A.getValues();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      DotOp2<Scalar> op;
      for (size_t j=0; j<nC; ++j) {
        op.x = Adata(0,nR).getRawPtr();
        op.y = Bdata(0,nR).getRawPtr();
        dots[j] = node->parallel_reduce(0,nR,op);
        Adata += Astride;
        Bdata += Bstride;
      }
    }

    static Scalar
    Dot (const MultiVector<Scalar,Node> &A,
         const MultiVector<Scalar,Node> &B)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nR != B.getNumRows(), std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Dot(A,B): "
        "A and B must have the same number of rows.");
      if (nR*nC == 0) {
        return Teuchos::ScalarTraits<Scalar>::zero ();
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Bdata = B.getValues(0);
      ArrayRCP<const Scalar> Adata = A.getValues(0);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      DotOp2<Scalar> op;
      op.x = Adata(0,nR).getRawPtr();
      op.y = Bdata(0,nR).getRawPtr();
      return node->parallel_reduce(0,nR,op);
    }

    static void
    GESUM (MultiVector<Scalar,Node> &B,
           Scalar alpha,
           const MultiVector<Scalar,Node> &A,
           Scalar beta)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC != B.getNumCols() || nR != B.getNumRows(), std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::GESUM(B,alpha,A,beta): "
        "A and B must have the same dimensions.");
      RCP<Node> node = B.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      ArrayRCP<Scalar>       Bdata = B.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.template addNonConstBuffer<Scalar>(Bdata);
      rbh.end();

      // mfh 07 Mar 2013: Special case for beta == 0, to overwrite B
      // unconditionally, regardless of NaN entries.
      if (beta == Teuchos::ScalarTraits<Scalar>::zero ()) {
        GESUMZeroBetaOp<Scalar> wdp;
        wdp.alpha = alpha;
        if (Astride == nR && Bstride == nR) {
          // one kernel invocation for whole multivector
          wdp.y = Bdata(0,nR*nC).getRawPtr();
          wdp.x = Adata(0,nR*nC).getRawPtr();
          node->template parallel_for<GESUMZeroBetaOp<Scalar> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          for (size_t j=0; j<nC; ++j) {
            wdp.y = Bdata(0,nR).getRawPtr();
            wdp.x = Adata(0,nR).getRawPtr();
            node->template parallel_for<GESUMZeroBetaOp<Scalar> >(0,nR,wdp);
            Adata += Astride;
            Bdata += Bstride;
          }
        }
      }
      else {
        GESUMOp<Scalar> wdp;
        wdp.alpha = alpha;
        wdp.beta  = beta;
        if (Astride == nR && Bstride == nR) {
          // one kernel invocation for whole multivector
          wdp.y = Bdata(0,nR*nC).getRawPtr();
          wdp.x = Adata(0,nR*nC).getRawPtr();
          node->template parallel_for<GESUMOp<Scalar> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          for (size_t j=0; j<nC; ++j) {
            wdp.y = Bdata(0,nR).getRawPtr();
            wdp.x = Adata(0,nR).getRawPtr();
            node->template parallel_for<GESUMOp<Scalar> >(0,nR,wdp);
            Adata += Astride;
            Bdata += Bstride;
          }
        }
      }
    }

    static void
    GESUM (MultiVector<Scalar,Node> &C,
           Scalar alpha,
           const MultiVector<Scalar,Node> &A,
           Scalar beta,
           const MultiVector<Scalar,Node> &B,
           Scalar gamma)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      const size_t Cstride = C.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC != B.getNumCols() || nR != B.getNumRows(),
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::GESUM"
        "(C,alpha,A,beta,B,gamma): A and B must have the same dimensions.");

      RCP<Node> node = B.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      ArrayRCP<const Scalar> Bdata = B.getValues();
      ArrayRCP<Scalar>       Cdata = C.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addNonConstBuffer<Scalar>(Cdata);
      rbh.end();

      // mfh 07 Mar 2013: Special case for gamma == 0, to overwrite C
      // unconditionally, regardless of NaN entries.
      if (gamma == Teuchos::ScalarTraits<Scalar>::zero ()) {
        GESUMZeroGammaOp3<Scalar> wdp;
        wdp.alpha = alpha;
        wdp.beta  = beta;
        if (Astride == nR && Bstride == nR && Cstride == nR) {
          // one kernel invocation for whole multivector
          wdp.z = Cdata(0,nR*nC).getRawPtr();
          wdp.y = Bdata(0,nR*nC).getRawPtr();
          wdp.x = Adata(0,nR*nC).getRawPtr();
          node->template parallel_for<GESUMZeroGammaOp3<Scalar> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          for (size_t j=0; j<nC; ++j) {
            wdp.z = Cdata(0,nR).getRawPtr();
            wdp.y = Bdata(0,nR).getRawPtr();
            wdp.x = Adata(0,nR).getRawPtr();
            node->template parallel_for<GESUMZeroGammaOp3<Scalar> >(0,nR,wdp);
            Adata += Astride;
            Bdata += Bstride;
            Cdata += Cstride;
          }
        }
      }
      else {
        GESUMOp3<Scalar> wdp;
        wdp.alpha = alpha;
        wdp.beta  = beta;
        wdp.gamma = gamma;
        if (Astride == nR && Bstride == nR && Cstride == nR) {
          // one kernel invocation for whole multivector
          wdp.z = Cdata(0,nR*nC).getRawPtr();
          wdp.y = Bdata(0,nR*nC).getRawPtr();
          wdp.x = Adata(0,nR*nC).getRawPtr();
          node->template parallel_for<GESUMOp3<Scalar> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          for (size_t j=0; j<nC; ++j) {
            wdp.z = Cdata(0,nR).getRawPtr();
            wdp.y = Bdata(0,nR).getRawPtr();
            wdp.x = Adata(0,nR).getRawPtr();
            node->template parallel_for<GESUMOp3<Scalar> >(0,nR,wdp);
            Adata += Astride;
            Bdata += Bstride;
            Cdata += Cstride;
          }
        }
      }
    }

    static void
    Norm1 (const MultiVector<Scalar,Node> &A,
           const ArrayView<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> &norms)
    {
      typedef Teuchos::ScalarTraits<Scalar> STS;
      typedef typename STS::magnitudeType magnitude_type;
      typedef Teuchos::ScalarTraits<magnitude_type> STM;

      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC > Teuchos::as<size_t>(norms.size()),
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Norm1(A,norms): "
        "norms must have length as large as number of columns of A.");

      if (nR*nC == 0) {
        std::fill (norms.begin(), norms.begin() + nC, STM::zero ());
        return;
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      SumAbsOp<Scalar> op;
      for (size_t j=0; j<nC; ++j) {
        op.x = Adata(0,nR).getRawPtr();
        norms[j] = node->parallel_reduce(0,nR,op);
        Adata += Astride;
      }
    }

    static typename Teuchos::ScalarTraits<Scalar>::magnitudeType
    Norm1 (const MultiVector<Scalar,Node> &A)
    {
      typedef Teuchos::ScalarTraits<Scalar> STS;
      typedef typename STS::magnitudeType magnitude_type;
      typedef Teuchos::ScalarTraits<magnitude_type> STM;

      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      if (nR*nC == 0) {
        return STM::zero ();
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues(0);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      SumAbsOp<Scalar> op;
      op.x = Adata(0,nR).getRawPtr();
      return node->parallel_reduce(0,nR,op);
    }

    static void
    Sum (const MultiVector<Scalar,Node> &A,
         const ArrayView<Scalar> &sums)
    {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(
        nC > (size_t)sums.size(),
        std::runtime_error,
        "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Sum(A,sums): "
        "sums must have length as large as number of columns of A.");

      if (nR*nC == 0) {
        std::fill( sums.begin(), sums.begin() + nC, Teuchos::ScalarTraits<Scalar>::zero() );
        return;
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      SumOp<Scalar> op;
      for (size_t j=0; j<nC; ++j) {
        op.x = Adata(0,nR).getRawPtr();
        sums[j] = node->parallel_reduce(0,nR,op);
        Adata += Astride;
      }
    }

    static Scalar Sum(const MultiVector<Scalar,Node> &A) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      if (nR*nC == 0) {
        return Teuchos::ScalarTraits<Scalar>::zero();
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues(0);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      SumOp<Scalar> op;
      op.x = Adata(0,nR).getRawPtr();
      return node->parallel_reduce(0,nR,op);
    }

    static typename Teuchos::ScalarTraits<Scalar>::magnitudeType NormInf(const MultiVector<Scalar,Node> &A) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      if (nR*nC == 0) {
        return Teuchos::ScalarTraits<typename Teuchos::ScalarTraits<Scalar>::magnitudeType>::zero();
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues(0);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      MaxAbsOp<Scalar> op;
      op.x = Adata(0,nR).getRawPtr();
      return node->parallel_reduce(0,nR,op);
    }

    static void NormInf(const MultiVector<Scalar,Node> &A, const ArrayView<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> &norms) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(nC > Teuchos::as<size_t>(norms.size()), std::runtime_error,
                                 "DefaultArithmetic<" << Teuchos::typeName(A) << ">::NormInf(A,norms): norms must have length as large as number of columns of A.");
      if (nR*nC == 0) {
        std::fill( norms.begin(), norms.begin() + nC, Teuchos::ScalarTraits<typename Teuchos::ScalarTraits<Scalar>::magnitudeType>::zero() );
        return;
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      MaxAbsOp<Scalar> op;
      for (size_t j=0; j<nC; ++j) {
        op.x = Adata(0,nR).getRawPtr();
        norms[j] = node->parallel_reduce(0,nR,op);
        Adata += Astride;
      }
    }

    static void Norm2Squared(const MultiVector<Scalar,Node> &A, const ArrayView<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> &norms) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(nC > Teuchos::as<size_t>(norms.size()), std::runtime_error,
                                 "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Norm2Squared(A,norms): norms must have length as large as number of columns of A.");
      if (nR*nC == 0) {
        std::fill( norms.begin(), norms.begin() + nC, Teuchos::ScalarTraits<typename Teuchos::ScalarTraits<Scalar>::magnitudeType>::zero() );
        return;
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      DotOp1<Scalar> op;
      for (size_t j=0; j<nC; ++j) {
        op.x = Adata(0,nR).getRawPtr();
        norms[j] = node->parallel_reduce(0,nR,op);
        Adata += Astride;
      }
    }

    static typename Teuchos::ScalarTraits<Scalar>::magnitudeType
    Norm2Squared(const MultiVector<Scalar,Node> &A) {
      const size_t nR = A.getNumRows();
      if (nR == 0) {
        return Teuchos::ScalarTraits<typename Teuchos::ScalarTraits<Scalar>::magnitudeType>::zero();
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues(0);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.end();
      DotOp1<Scalar> op;
      op.x = Adata(0,nR).getRawPtr();
      return node->parallel_reduce(0,nR,op);
    }

    static typename Teuchos::ScalarTraits<Scalar>::magnitudeType
    WeightedNorm(const MultiVector<Scalar,Node> &A, const MultiVector<Scalar,Node> &weightVector) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      if (nR*nC == 0) {
        return Teuchos::ScalarTraits<typename Teuchos::ScalarTraits<Scalar>::magnitudeType>::zero();
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues(0),
        Wdata = weightVector.getValues(0);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.template addConstBuffer<Scalar>(Wdata);
      rbh.end();
      WeightNormOp<Scalar> op;
      op.x = Adata(0,nR).getRawPtr();
      op.w = Wdata(0,nR).getRawPtr();
      return node->parallel_reduce(0,nR,op);
    }

    static void WeightedNorm(const MultiVector<Scalar,Node> &A, const MultiVector<Scalar,Node> &weightVector, const ArrayView<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> &norms) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride(),
        Wstride = weightVector.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(nC > Teuchos::as<size_t>(norms.size()), std::runtime_error,
                                 "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Norm1(A,norms): norms must have length as large as number of columns of A.");
      if (nR*nC == 0) {
        std::fill( norms.begin(), norms.begin() + nC, Teuchos::ScalarTraits<typename Teuchos::ScalarTraits<Scalar>::magnitudeType>::zero() );
        return;
      }
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues(),
        Wdata = weightVector.getValues();
      const bool OneW = (weightVector.getNumCols() == 1);
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.template addConstBuffer<Scalar>(Wdata);
      rbh.end();
      WeightNormOp<Scalar> op;
      if (OneW) {
        op.w = Wdata(0,nR).getRawPtr();
        for (size_t j=0; j<nC; ++j) {
          op.x = Adata(0,nR).getRawPtr();
          norms[j] = node->parallel_reduce(0,nR,op);
          Adata += Astride;
        }
      }
      else {
        for (size_t j=0; j<nC; ++j) {
          op.x = Adata(0,nR).getRawPtr();
          op.w = Wdata(0,nR).getRawPtr();
          norms[j] = node->parallel_reduce(0,nR,op);
          Adata += Astride;
          Wdata += Wstride;
        }
      }
    }

    static void Abs(MultiVector<Scalar,Node> &A, const MultiVector<Scalar,Node> &B) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(nC != B.getNumCols() || nR != B.getNumRows(), std::runtime_error,
                                 "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Abs(A,B): A and B must have the same dimensions.");
      if (nC*nR == 0) return;
      RCP<Node> node = A.getNode();
      ArrayRCP<const Scalar> Bdata = B.getValues();
      ArrayRCP<Scalar>       Adata = A.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Bdata);
      rbh.template addNonConstBuffer<Scalar>(Adata);
      rbh.end();
      // prepare op
      AbsOp<Scalar> wdp;
      if (Astride == nR && Bstride == nR) {
        // one kernel invocation for whole multivector assignment
        wdp.x = Adata(0,nR*nC).getRawPtr();
        wdp.y = Bdata(0,nR*nC).getRawPtr();
        node->template parallel_for<AbsOp<Scalar> >(0,nR*nC,wdp);
      }
      else {
        // one kernel invocation for each column
        for (size_t j=0; j<nC; ++j) {
          wdp.x = Adata(0,nR).getRawPtr();
          wdp.y = Bdata(0,nR).getRawPtr();
          node->template parallel_for<AbsOp<Scalar> >(0,nR,wdp);
          Adata += Astride;
          Bdata += Bstride;
        }
      }
    }

    static void Scale(MultiVector<Scalar,Node> &B, Scalar alpha, const MultiVector<Scalar,Node> &A) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t Astride = A.getStride();
      const size_t Bstride = B.getStride();
      TEUCHOS_TEST_FOR_EXCEPTION(nC != B.getNumCols() || nR != B.getNumRows(), std::runtime_error,
                                 "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Scale(B,alpha,A): A and B must have the same dimensions.");
      RCP<Node> node = B.getNode();
      ArrayRCP<const Scalar> Adata = A.getValues();
      ArrayRCP<Scalar>       Bdata = B.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addConstBuffer<Scalar>(Adata);
      rbh.template addNonConstBuffer<Scalar>(Bdata);
      rbh.end();
      MVScaleOp<Scalar> wdp;
      wdp.alpha = alpha;
      if (Astride == nR && Bstride == nR) {
        // one kernel invocation for whole multivector
        wdp.x = Bdata(0,nR*nC).getRawPtr();
        wdp.y = Adata(0,nR*nC).getRawPtr();
        node->template parallel_for<MVScaleOp<Scalar> >(0,nR*nC,wdp);
      }
      else {
        // one kernel invocation for each column
        for (size_t j=0; j<nC; ++j) {
          wdp.x = Bdata(0,nR).getRawPtr();
          wdp.y = Adata(0,nR).getRawPtr();
          node->template parallel_for<MVScaleOp<Scalar> >(0,nR,wdp);
          Adata += Astride;
          Bdata += Bstride;
        }
      }
    }

    static void Scale(MultiVector<Scalar,Node> &A, Scalar alpha) {
      const size_t nR = A.getNumRows();
      const size_t nC = A.getNumCols();
      const size_t stride = A.getStride();
      RCP<Node> node = A.getNode();
      ArrayRCP<Scalar> data = A.getValuesNonConst();
      // prepare buffers
      ReadyBufferHelper<Node> rbh(node);
      rbh.begin();
      rbh.template addNonConstBuffer<Scalar>(data);
      rbh.end();
      // prepare op
      SingleScaleOp<Scalar> wdp;
      wdp.alpha = alpha;
      if (stride == nR) {
        // one kernel invocation for whole multivector
        wdp.x = data(0,nR*nC).getRawPtr();
        node->template parallel_for<SingleScaleOp<Scalar> >(0,nR*nC,wdp);
      }
      else {
        // one kernel invocation for each column
        for (size_t j=0; j<nC; ++j) {
          wdp.x = data(0,nR).getRawPtr();
          node->template parallel_for<SingleScaleOp<Scalar> >(0,nR,wdp);
          data += stride;
        }
      }
    }
  };

} // namespace KokkosClassic

#endif
