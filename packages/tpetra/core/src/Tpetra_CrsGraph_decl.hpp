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

#ifndef TPETRA_CRSGRAPH_DECL_HPP
#define TPETRA_CRSGRAPH_DECL_HPP

/// \file Tpetra_CrsGraph_decl.hpp
/// \brief Declaration of the Tpetra::CrsGraph class
///
/// If you want to use Tpetra::CrsGraph, include "Tpetra_CrsGraph.hpp"
/// (a file which CMake generates and installs for you).  If you only
/// want the declaration of Tpetra::CrsGraph, include this file
/// (Tpetra_CrsGraph_decl.hpp).

#include "Tpetra_ConfigDefs.hpp"
#include "Tpetra_RowGraph.hpp"
#include "Tpetra_DistObject.hpp"
#include "Tpetra_Exceptions.hpp"

#include "KokkosCompat_ClassicNodeAPI_Wrapper.hpp"
#include "Kokkos_DualView.hpp"
#include "Kokkos_StaticCrsGraph.hpp"

#include "Teuchos_Describable.hpp"
#include "Teuchos_ParameterListAcceptorDefaultBase.hpp"


namespace Tpetra {

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  //
  // Dear users: These are just forward declarations.  Please skip
  // over them and go down to the CrsMatrix class declaration.  Thank
  // you.
  //
  template <class LO, class GO, class N, const bool isClassic>
  class CrsGraph;

  // forward declaration (needed for "friend" inside CrsGraph)
  template <class S, class LO, class GO, class N, const bool isClassic>
  class CrsMatrix;

  namespace Experimental {
    // forward declaration (needed for "friend" inside CrsGraph)
    template<class S, class LO, class GO, class N>
    class BlockCrsMatrix;
  } // namespace Experimental

  namespace Details {
    // Forward declaration of an implementation detail of CrsGraph::clone.
    template<class OutputCrsGraphType, class InputCrsGraphType>
    class CrsGraphCopier {
    public:
      static Teuchos::RCP<OutputCrsGraphType>
      clone (const InputCrsGraphType& graphIn,
             const Teuchos::RCP<typename OutputCrsGraphType::node_type> nodeOut,
             const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);
    };
  } // namespace Details
#endif // DOXYGEN_SHOULD_SKIP_THIS

  /// \struct RowInfo
  /// \brief Allocation information for a locally owned row in a
  ///   CrsGraph or CrsMatrix
  ///
  /// A RowInfo instance identifies a locally owned row uniquely by
  /// its local index, and contains other information useful for
  /// inserting entries into the row.  It is the return value of
  /// CrsGraph's getRowInfo() or updateAllocAndValues() methods.
  struct RowInfo {
    size_t localRow;
    size_t allocSize;
    size_t numEntries;
    size_t offset1D;
  };

  enum ELocalGlobal {
    LocalIndices,
    GlobalIndices
  };

  namespace Details {
    /// \brief Status of the graph's or matrix's storage, when not in
    ///   a fill-complete state.
    ///
    /// When a CrsGraph or CrsMatrix is <i>not</i> fill complete, its
    /// data live in one of three storage formats:
    ///
    /// <ol>
    /// <li> "2-D storage": The graph stores column indices as "array
    ///   of arrays," and the matrix stores values as "array of
    ///   arrays."  The graph <i>must</i> have k_numRowEntries_
    ///   allocated.  This only ever exists if the graph was created
    ///   with DynamicProfile.  A matrix with 2-D storage must own its
    ///   graph, and the graph must have 2-D storage. </li>
    ///
    /// <li> "Unpacked 1-D storage": The graph uses a row offsets
    ///   array, and stores column indices in a single array.  The
    ///   matrix also stores values in a single array.  "Unpacked"
    ///   means that there may be extra space in each row: that is,
    ///   the row offsets array only says how much space there is in
    ///   each row.  The graph must use k_numRowEntries_ to find out
    ///   how many entries there actually are in the row.  A matrix
    ///   with unpacked 1-D storage must own its graph, and the graph
    ///   must have unpacked 1-D storage. </li>
    ///
    /// <li> "Packed 1-D storage": The matrix may or may not own the
    ///   graph.  "Packed" means that there is no extra space in each
    ///   row.  Thus, the k_numRowEntries_ array is not necessary and
    ///   may have been deallocated.  If the matrix was created with a
    ///   constant ("static") graph, this must be true. </li>
    /// </ol>
    ///
    /// With respect to the Kokkos refactor version of Tpetra, "2-D
    /// storage" should be considered a legacy option.
    ///
    /// The phrase "When not in a fill-complete state" is important.
    /// When the graph is fill complete, it <i>always</i> uses 1-D
    /// "packed" storage.  However, if storage is "not optimized," we
    /// retain the 1-D unpacked or 2-D format, and thus retain this
    /// enum value.
    enum EStorageStatus {
      STORAGE_2D, //<! 2-D storage
      STORAGE_1D_UNPACKED, //<! 1-D "unpacked" storage
      STORAGE_1D_PACKED, //<! 1-D "packed" storage
      STORAGE_UB //<! Invalid value; upper bound on enum values
    };
  } // namespace Details

  /// \class CrsGraph
  /// \brief A distributed graph accessed by rows (adjacency lists)
  ///   and stored sparsely.
  ///
  /// \tparam LocalOrdinal The type of local indices.  See the
  ///   documentation of Map for requirements.
  /// \tparam GlobalOrdinal The type of global indices.  See the
  ///   documentation of Map for requirements.
  /// \tparam Node The Kokkos Node type.  See the documentation of Map
  ///   for requirements.
  ///
  /// This class implements a distributed-memory parallel sparse
  /// graph.  It provides access by rows to the elements of the graph,
  /// as if the local data were stored in compressed sparse row format
  /// (adjacency lists, in graph terms).  (Implementations are
  /// <i>not</i> required to store the data in this way internally.)
  /// This class has an interface like that of Epetra_CrsGraph, but
  /// also allows insertion of data into nonowned rows, much like
  /// Epetra_FECrsGraph.
  ///
  /// \section Tpetra_CrsGraph_prereq Prerequisites
  ///
  /// Before reading the rest of this documentation, it helps to know
  /// something about the Teuchos memory management classes, in
  /// particular Teuchos::RCP, Teuchos::ArrayRCP, and
  /// Teuchos::ArrayView.  You should also know a little bit about MPI
  /// (the Message Passing Interface for distributed-memory
  /// programming).  You won't have to use MPI directly to use
  /// CrsGraph, but it helps to be familiar with the general idea of
  /// distributed storage of data over a communicator.  Finally, you
  /// should read the documentation of Map.
  ///
  /// \section Tpetra_CrsGraph_local_vs_global Local vs. global indices and nonlocal insertion
  ///
  /// Graph entries can be added using either local or global coordinates
  /// for the indices. The accessors isGloballyIndexed() and
  /// isLocallyIndexed() indicate whether the indices are currently
  /// stored as global or local indices. Many of the class methods are
  /// divided into global and local versions, which differ only in
  /// whether they accept/return indices in the global or local
  /// coordinate space. Some of these methods may only be used if the
  /// graph coordinates are in the appropriate coordinates.  For example,
  /// getGlobalRowView() returns a View to the indices in global
  /// coordinates; if the indices are not in global coordinates, then no
  /// such View can be created.
  ///
  /// The global/local distinction does distinguish between operation
  /// on the global/local graph. Almost all methods operate on the
  /// local graph, i.e., the rows of the graph associated with the
  /// local node, per the distribution specified by the row
  /// map. Access to non-local rows requires performing an explicit
  /// communication via the import/export capabilities of the CrsGraph
  /// object; see DistObject. However, the method
  /// insertGlobalIndices() is an exception to this rule, as non-local
  /// rows are allowed to be added via the local graph. These rows are
  /// stored in the local graph and communicated to the appropriate
  /// node on the next call to globalAssemble() or fillComplete() (the
  /// latter calls the former).
  template <class LocalOrdinal = Details::DefaultTypes::local_ordinal_type,
            class GlobalOrdinal = Details::DefaultTypes::global_ordinal_type,
            class Node = Details::DefaultTypes::node_type,
            const bool classic = Node::classic>
  class CrsGraph :
    public RowGraph<LocalOrdinal, GlobalOrdinal, Node>,
    public DistObject<GlobalOrdinal,
                      LocalOrdinal,
                      GlobalOrdinal,
                      Node>,
    public Teuchos::ParameterListAcceptorDefaultBase
  {
    static_assert (! classic, "The 'classic' version of Tpetra was deprecated long ago, and has been removed.");

    template <class S, class LO, class GO, class N, const bool isClassic>
    friend class CrsMatrix;
    template <class LO2, class GO2, class N2, const bool isClassic>
    friend class CrsGraph;
    template <class S, class LO, class GO, class N>
    friend class ::Tpetra::Experimental::BlockCrsMatrix;

    //! The specialization of DistObject that is this class' parent class.
    typedef DistObject<GlobalOrdinal, LocalOrdinal, GlobalOrdinal, Node> dist_object_type;

  public:
    //! This class' first template parameter; the type of local indices.
    typedef LocalOrdinal local_ordinal_type;
    //! This class' second template parameter; the type of global indices.
    typedef GlobalOrdinal global_ordinal_type;
    //! This class' Kokkos Node type.
    typedef Node node_type;

    //! This class' Kokkos device type.
    typedef typename Node::device_type device_type;
    //! This class' Kokkos execution space.
    typedef typename device_type::execution_space execution_space;

    //! The type of the part of the sparse graph on each MPI process.
    typedef Kokkos::StaticCrsGraph<LocalOrdinal,
                                   Kokkos::LayoutLeft,
                                   execution_space> local_graph_type;
    //! DEPRECATED; use local_graph_type (above) instead.
    typedef local_graph_type LocalStaticCrsGraphType TPETRA_DEPRECATED;

    //! DEPRECATED; use <tt>local_graph_type::row_map_type</tt> instead.
    typedef typename local_graph_type::row_map_type t_RowPtrs TPETRA_DEPRECATED;
    //! DEPRECATED; use <tt>local_graph_type::row_map_type::non_const_type</tt> instead.
    typedef typename local_graph_type::row_map_type::non_const_type t_RowPtrsNC TPETRA_DEPRECATED;
    //! DEPRECATED; use <tt>local_graph_type::entries_type::non_const_type</tt> instead.
    typedef typename local_graph_type::entries_type::non_const_type t_LocalOrdinal_1D TPETRA_DEPRECATED;

    //! The Map specialization used by this class.
    typedef Tpetra::Map<LocalOrdinal, GlobalOrdinal, Node> map_type;
    //! The Import specialization used by this class.
    typedef Tpetra::Import<LocalOrdinal, GlobalOrdinal, Node> import_type;
    //! The Export specialization used by this class.
    typedef Tpetra::Export<LocalOrdinal, GlobalOrdinal, Node> export_type;

    //! @name Constructor/Destructor Methods
    //@{

    /// \brief Constructor specifying a single upper bound for the
    ///   number of entries in all rows on the calling process.
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param maxNumEntriesPerRow [in] Maximum number of graph
    ///   entries per row.  If pftype==DynamicProfile, this is only a
    ///   hint, and you can set this to zero without affecting
    ///   correctness.  If pftype==StaticProfile, this sets the amount
    ///   of storage allocated, and you cannot exceed this number of
    ///   entries in any row.
    ///
    /// \param pftype [in] Whether to allocate storage dynamically
    ///   (DynamicProfile) or statically (StaticProfile).
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              size_t maxNumEntriesPerRow,
              ProfileType pftype = DynamicProfile,
              const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /// \brief Constructor specifying a (possibly different) upper
    ///   bound for the number of entries in each row.
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param numEntPerRow [in] Maximum number of graph entries to
    ///   allocate for each row.  If pftype==DynamicProfile, this is
    ///   only a hint.  If pftype==StaticProfile, this sets the amount
    ///   of storage allocated, and you cannot exceed the allocated
    ///   number of entries for any row.
    ///
    /// \param pftype [in] Whether to allocate storage dynamically
    ///   (DynamicProfile) or statically (StaticProfile).
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Kokkos::DualView<const size_t*, execution_space>& numEntPerRow,
              const ProfileType pftype = DynamicProfile,
              const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /// \brief Constructor specifying a (possibly different) upper
    ///   bound for the number of entries in each row (legacy
    ///   KokkosClassic version).
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param numEntPerRow [in] Maximum number of graph entries to
    ///   allocate for each row.  If pftype==DynamicProfile, this is
    ///   only a hint.  If pftype==StaticProfile, this sets the amount
    ///   of storage allocated, and you cannot exceed the allocated
    ///   number of entries for any row.
    ///
    /// \param pftype [in] Whether to allocate storage dynamically
    ///   (DynamicProfile) or statically (StaticProfile).
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::ArrayRCP<const size_t>& numEntPerRow,
              const ProfileType pftype = DynamicProfile,
              const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /// \brief Constructor specifying column Map and a single upper
    ///   bound for the number of entries in all rows on the calling
    ///   process.
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param colMap [in] Distribution of columns of the graph.
    ///
    /// \param maxNumEntriesPerRow [in] Maximum number of graph
    ///   entries per row.  If pftype==DynamicProfile, this is only a
    ///   hint, and you can set this to zero without affecting
    ///   correctness.  If pftype==StaticProfile, this sets the amount
    ///   of storage allocated, and you cannot exceed this number of
    ///   entries in any row.
    ///
    /// \param pftype [in] Whether to allocate storage dynamically
    ///   (DynamicProfile) or statically (StaticProfile).
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::RCP<const map_type>& colMap,
              const size_t maxNumEntriesPerRow,
              const ProfileType pftype = DynamicProfile,
              const Teuchos::RCP<Teuchos::ParameterList>& params = null);

    /// \brief Constructor specifying column Map and number of entries in each row.
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param colMap [in] Distribution of columns of the graph.
    ///
    /// \param numEntPerRow [in] Maximum number of graph entries to
    ///   allocate for each row.  If pftype==DynamicProfile, this is
    ///   only a hint.  If pftype==StaticProfile, this sets the amount
    ///   of storage allocated, and you cannot exceed the allocated
    ///   number of entries for any row.
    ///
    /// \param pftype [in] Whether to allocate storage dynamically
    ///   (DynamicProfile) or statically (StaticProfile).
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::RCP<const map_type>& colMap,
              const Kokkos::DualView<const size_t*, execution_space>& numEntPerRow,
              ProfileType pftype = DynamicProfile,
              const Teuchos::RCP<Teuchos::ParameterList>& params = null);

    /// \brief Constructor specifying column Map and number of entries
    ///   in each row (legacy KokkosClassic version).
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param colMap [in] Distribution of columns of the graph.
    ///
    /// \param numEntPerRow [in] Maximum number of graph entries to
    ///   allocate for each row.  If pftype==DynamicProfile, this is
    ///   only a hint.  If pftype==StaticProfile, this sets the amount
    ///   of storage allocated, and you cannot exceed the allocated
    ///   number of entries for any row.
    ///
    /// \param pftype [in] Whether to allocate storage dynamically
    ///   (DynamicProfile) or statically (StaticProfile).
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::RCP<const map_type>& colMap,
              const Teuchos::ArrayRCP<const size_t>& numEntPerRow,
              ProfileType pftype = DynamicProfile,
              const Teuchos::RCP<Teuchos::ParameterList>& params = null);

    /// \brief Constructor specifying column Map and arrays containing the graph in sorted, local ids.
    ///
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param colMap [in] Distribution of columns of the graph.
    ///
    /// \param rowPointers [in] The beginning of each row in the graph,
    ///   as in a CSR "rowptr" array.  The length of this vector should be
    ///   equal to the number of rows in the graph, plus one.  This last
    ///   entry should store the nunber of nonzeros in the graph.
    ///
    /// \param columnIndices [in] The local indices of the columns,
    ///   as in a CSR "colind" array.  The length of this vector
    ///   should be equal to the number of unknowns in the graph.
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::RCP<const map_type>& colMap,
              const typename local_graph_type::row_map_type& rowPointers,
              const typename local_graph_type::entries_type::non_const_type& columnIndices,
              const Teuchos::RCP<Teuchos::ParameterList>& params = null);

    /// \brief Constructor specifying column Map and arrays containing the graph in sorted, local ids.
    ///
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param colMap [in] Distribution of columns of the graph.
    ///
    /// \param rowPointers [in] The beginning of each row in the graph,
    ///   as in a CSR "rowptr" array.  The length of this vector should be
    ///   equal to the number of rows in the graph, plus one.  This last
    ///   entry should store the nunber of nonzeros in the graph.
    ///
    /// \param columnIndices [in] The local indices of the columns,
    ///   as in a CSR "colind" array.  The length of this vector
    ///   should be equal to the number of unknowns in the graph.
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::RCP<const map_type>& colMap,
              const Teuchos::ArrayRCP<size_t> & rowPointers,
              const Teuchos::ArrayRCP<LocalOrdinal> & columnIndices,
              const Teuchos::RCP<Teuchos::ParameterList>& params = null);

    /// \brief Constructor specifying column Map and a local (sorted)
    ///   graph, which the resulting CrsGraph views.
    ///
    /// Unlike most other CrsGraph constructors, successful completion
    /// of this constructor will result in a fill-complete graph.
    ///
    /// \param rowMap [in] Distribution of rows of the graph.
    ///
    /// \param colMap [in] Distribution of columns of the graph.
    ///
    /// \param lclGraph [in] A locally indexed Kokkos::StaticCrsGraph
    ///   whose local row indices come from the specified row Map, and
    ///   whose local column indices come from the specified column
    ///   Map.
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    CrsGraph (const Teuchos::RCP<const map_type>& rowMap,
              const Teuchos::RCP<const map_type>& colMap,
              const local_graph_type& lclGraph,
              const Teuchos::RCP<Teuchos::ParameterList>& params);

    /// \brief Create a cloned CrsGraph for a different Node type.
    ///
    /// This method creates a new CrsGraph on a specified Kokkos Node
    /// type, with all of the entries of this CrsGraph object.
    ///
    /// \param node2 [in] Kokkos Node instance for constructing the
    ///   clone CrsGraph and its constituent objects.
    ///
    /// \param params [in/out] Optional list of parameters. If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.  See the list below for valid options.
    ///
    /// Parameters accepted by this method:
    /// - "Static profile clone" [bool, default: true] If \c true,
    ///   creates the clone with a static allocation profile. If
    ///   false, a dynamic allocation profile is used.
    /// - "Locally indexed clone" [bool] If \c true, fills clone
    ///   using this graph's column map and local indices (requires
    ///   that this graph have a column map.) If false, fills clone
    ///   using global indices and does not provide a column map. By
    ///   default, will use local indices only if this graph is using
    ///   local indices.
    /// - "fillComplete clone" [boolean, default: true] If \c true,
    ///   calls fillComplete() on the cloned CrsGraph object, with
    ///   parameters from \c params sublist "CrsGraph". The domain map
    ///   and range maps passed to fillComplete() are those of the map
    ///   being cloned, if they exist. Otherwise, the row map is used.
    template<class Node2>
    Teuchos::RCP<CrsGraph<LocalOrdinal, GlobalOrdinal, Node2, Node2::classic> >
    clone (const Teuchos::RCP<Node2>& node2,
           const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) const
    {
      typedef CrsGraph<LocalOrdinal, GlobalOrdinal, Node2, Node2::classic> output_crs_graph_type;
      typedef CrsGraph<LocalOrdinal, GlobalOrdinal, Node, classic> input_crs_graph_type;
      typedef Details::CrsGraphCopier<output_crs_graph_type, input_crs_graph_type> copier_type;
      return copier_type::clone (*this, node2, params);
    }

    //! Destructor.
    virtual ~CrsGraph();

    //@}
    //! @name Implementation of Teuchos::ParameterListAcceptor
    //@{

    //! Set the given list of parameters (must be nonnull).
    void setParameterList (const Teuchos::RCP<Teuchos::ParameterList>& params);

    //! Default parameter list suitable for validation.
    Teuchos::RCP<const ParameterList> getValidParameters () const;

    //@}
    //! @name Insertion/Removal Methods
    //@{

    /// \brief Insert global indices into the graph.
    ///
    /// \pre \c globalRow is a valid index in the row Map.  It need
    ///   not be owned by the calling process.
    /// \pre <tt>isLocallyIndexed() == false</tt>
    /// \pre <tt>isStorageOptimized() == false</tt>
    ///
    /// \post <tt>indicesAreAllocated() == true</tt>
    /// \post <tt>isGloballyIndexed() == true</tt>
    ///
    /// If \c globalRow does not belong to the graph on this process,
    /// then it will be communicated to the appropriate process when
    /// globalAssemble() is called.  (That method will be called
    /// automatically during the next call to fillComplete().)
    /// Otherwise, the entries will be inserted into the part of the
    /// graph owned by the calling process.
    ///
    /// If the graph row already contains entries at the indices
    /// corresponding to values in \c indices, then the redundant
    /// indices will be eliminated.  This may happen either at
    /// insertion or during the next call to fillComplete().
    void
    insertGlobalIndices (GlobalOrdinal globalRow,
                         const Teuchos::ArrayView<const GlobalOrdinal>& indices);

    //! Insert local indices into the graph.
    /**
       \pre \c localRow is a local row belonging to the graph on this process.
       \pre <tt>isGloballyIndexed() == false</tt>
       \pre <tt>isStorageOptimized() == false</tt>
       \pre <tt>hasColMap() == true</tt>

       \post <tt>indicesAreAllocated() == true</tt>
       \post <tt>isLocallyIndexed() == true</tt>

       \note If the graph row already contains entries at the indices
         corresponding to values in \c indices, then the redundant
         indices will be eliminated; this may happen at insertion or
         during the next call to fillComplete().
    */
    void
    insertLocalIndices (const LocalOrdinal localRow,
                        const Teuchos::ArrayView<const LocalOrdinal> &indices);

    //! Remove all graph indices from the specified local row.
    /**
       \pre \c localRow is a local row of this graph.
       \pre <tt>isGloballyIndexed() == false</tt>
       \pre <tt>isStorageOptimized() == false</tt>

       \post <tt>getNumEntriesInLocalRow(localRow) == 0</tt>
       \post <tt>indicesAreAllocated() == true</tt>
       \post <tt>isLocallyIndexed() == true</tt>
    */
    void removeLocalIndices (LocalOrdinal localRow);

    //@}
    //! @name Transformational Methods
    /**
       Each of the methods in this group is a global collective. It is
       necessary to call these mehtods on all nodes participating in the
       communicator associated with this graph.
    */
    //@{

    /// \brief Communicate non-local contributions to other processes.
    ///
    /// This method is called automatically by fillComplete().
    /// Most users do not need to call this themselves,
    /// though we do permit this.
    void globalAssemble ();

    /*! Resume fill operations.
      After calling fillComplete(), resumeFill() must be called before initiating any changes to the graph.

      resumeFill() may be called repeatedly.

      \post  <tt>isFillActive() == true<tt>
      \post  <tt>isFillComplete() == false<tt>
    */
    void resumeFill (const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /*! \brief Signal that data entry is complete, specifying domain and range maps.

      Off-process indices are distributed (via globalAssemble()),
      indices are sorted, redundant indices are eliminated, and
      global indices are transformed to local indices.

      \pre <tt>isFillActive() == true<tt>
      \pre <tt>isFillComplete()() == false<tt>

      \post <tt>isFillActive() == false<tt>
      \post <tt>isFillComplete() == true<tt>

      Parameters:
      - "Optimize Storage" (\c bool): Default is false.  If true,
        then isStorageOptimized() returns true after fillComplete
        finishes.  See isStorageOptimized() for consequences.
    */
    void
    fillComplete (const Teuchos::RCP<const map_type> &domainMap,
                  const Teuchos::RCP<const map_type> &rangeMap,
                  const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /*! \brief Signal that data entry is complete.

      Off-node entries are distributed (via globalAssemble()), repeated entries are summed, and global indices are transformed to local indices.

      \note This method calls fillComplete( getRowMap(), getRowMap(), os ). See parameter options there.
    */
    void fillComplete (const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /// \brief Perform a fillComplete on a graph that already has
    ///   data, via setAllIndices().
    ///
    /// The graph must already have filled local 1-D storage.  If the
    /// graph has been constructed in any other way, this method will
    /// throw an exception.  This routine is needed to support other
    /// Trilinos packages and should not be called by ordinary users.
    ///
    /// \warning This method is intended for expert developer use
    ///   only, and should never be called by user code.
    void
    expertStaticFillComplete (const Teuchos::RCP<const map_type> & domainMap,
                              const Teuchos::RCP<const map_type> & rangeMap,
                              const Teuchos::RCP<const import_type> &importer=Teuchos::null,
                              const Teuchos::RCP<const export_type> &exporter=Teuchos::null,
                              const Teuchos::RCP<Teuchos::ParameterList> &params=Teuchos::null);
    //@}
    //! @name Methods implementing RowGraph.
    //@{

    //! Returns the communicator.
    Teuchos::RCP<const Comm<int> > getComm() const;

    //! Returns the underlying node.
    Teuchos::RCP<node_type> getNode() const;

    //! Returns the Map that describes the row distribution in this graph.
    Teuchos::RCP<const map_type> getRowMap () const;

    //! \brief Returns the Map that describes the column distribution in this graph.
    Teuchos::RCP<const map_type> getColMap () const;

    //! Returns the Map associated with the domain of this graph.
    Teuchos::RCP<const map_type> getDomainMap () const;

    //! Returns the Map associated with the domain of this graph.
    Teuchos::RCP<const map_type> getRangeMap () const;

    //! Returns the importer associated with this graph.
    Teuchos::RCP<const import_type> getImporter () const;

    //! Returns the exporter associated with this graph.
    Teuchos::RCP<const export_type> getExporter () const;

    //! Returns the number of global rows in the graph.
    /** Undefined if isFillActive().
     */
    global_size_t getGlobalNumRows() const;

    //! \brief Returns the number of global columns in the graph.
    /** Returns the number of entries in the domain map of the matrix.
        Undefined if isFillActive().
    */
    global_size_t getGlobalNumCols() const;

    //! Returns the number of graph rows owned on the calling node.
    size_t getNodeNumRows() const;

    //! Returns the number of columns connected to the locally owned rows of this graph.
    /** Throws std::runtime_error if <tt>hasColMap() == false</tt>
     */
    size_t getNodeNumCols() const;

    //! Returns the index base for global indices for this graph.
    GlobalOrdinal getIndexBase() const;

    //! Returns the global number of entries in the graph.
    /** Undefined if isFillActive().
     */
    global_size_t getGlobalNumEntries() const;

    //! Returns the local number of entries in the graph.
    size_t getNodeNumEntries() const;

    //! \brief Returns the current number of entries on this node in the specified global row.
    /*! Returns OrdinalTraits<size_t>::invalid() if the specified global row does not belong to this graph. */
    size_t getNumEntriesInGlobalRow(GlobalOrdinal globalRow) const;

    //! Returns the current number of entries on this node in the specified local row.
    /*! Returns OrdinalTraits<size_t>::invalid() if the specified local row is not valid for this graph. */
    size_t getNumEntriesInLocalRow(LocalOrdinal localRow) const;

    //! \brief Returns the total number of indices allocated for the graph, across all rows on this node.
    /*! This is the allocation available to the user. Actual allocation may be larger, for example, after
      calling fillComplete(), and thus this does not necessarily reflect the memory consumption of the
      this graph.

      This quantity is computed during the actual allocation. Therefore, if <tt>indicesAreAllocated() == false</tt>,
      this method returns <tt>OrdinalTraits<size_t>::invalid()</tt>.
    */
    size_t getNodeAllocationSize() const;

    //! \brief Returns the current number of allocated entries for this node in the specified global row .
    /** Throws exception std::runtime_error if the specified global row does not belong to this node. */
    size_t getNumAllocatedEntriesInGlobalRow(GlobalOrdinal globalRow) const;

    //! Returns the current number of allocated entries on this node in the specified local row.
    /** Throws exception std::runtime_error if the specified local row is not valid for this node. */
    size_t getNumAllocatedEntriesInLocalRow(LocalOrdinal localRow) const;

    //! \brief Returns the number of global diagonal entries, based on global row/column index comparisons.
    /** Undefined if isFillActive().
     */
    global_size_t getGlobalNumDiags() const;

    //! \brief Returns the number of local diagonal entries, based on global row/column index comparisons.
    /** Undefined if isFillActive().
     */
    size_t getNodeNumDiags() const;

    /// \brief Maximum number of entries in all rows over all processes.
    ///
    /// \note Undefined if isFillActive().
    ///
    /// \note This is the same as the result of a global maximum of
    ///   getNodeMaxNumRowEntries() over all processes.  That may not
    ///   necessarily mean what you think it does if some rows of the
    ///   matrix are owned by multiple processes.  In particular, some
    ///   processes might only own some of the entries in a particular
    ///   row.  This method only counts the number of entries in each
    ///   row that a process owns, not the total number of entries in
    ///   the row over all processes.
    size_t getGlobalMaxNumRowEntries() const;

    //! \brief Maximum number of entries in all rows owned by the calling process.
    /** Undefined if isFillActive().
     */
    size_t getNodeMaxNumRowEntries() const;

    /// \brief Whether the graph has a column Map.
    ///
    /// A CrsGraph has a column Map either because it was given to its
    /// constructor, or because it was constructed in fillComplete().
    /// Calling fillComplete() always makes a column Map if the graph
    /// does not already have one.
    ///
    /// A column Map lets the graph
    ///
    ///   - use local indices for storing entries in each row, and
    ///   - compute an Import from the domain Map to the column Map.
    ///
    /// The latter is mainly useful for a graph associated with a
    /// CrsMatrix.
    bool hasColMap() const;

    /// \brief Whether the graph is locally lower triangular.
    ///
    /// \pre <tt>! isFillActive()</tt>.
    ///   If fill is active, this method's behavior is undefined.
    ///
    /// \note This is entirely a local property.  That means this
    ///   method may return different results on different processes.
    bool isLowerTriangular() const;

    /// \brief Whether the graph is locally upper triangular.
    ///
    /// \pre <tt>! isFillActive()</tt>.
    ///   If fill is active, this method's behavior is undefined.
    ///
    /// \note This is entirely a local property.  That means this
    ///   method may return different results on different processes.
    bool isUpperTriangular() const;

    //! \brief If graph indices are in the local range, this function returns true. Otherwise, this function returns false. */
    bool isLocallyIndexed() const;

    //! \brief If graph indices are in the global range, this function returns true. Otherwise, this function returns false. */
    bool isGloballyIndexed() const;

    //! Returns \c true if fillComplete() has been called and the graph is in compute mode.
    bool isFillComplete() const;

    //! Returns \c true if resumeFill() has been called and the graph is in edit mode.
    bool isFillActive() const;

    /// \brief Whether graph indices in all rows are known to be sorted.
    ///
    /// A fill-complete graph is always sorted, as is a newly
    /// constructed graph. A graph is sorted immediately after calling
    /// resumeFill(), but any changes to the graph may result in the
    /// sorting status becoming unknown (and therefore, presumed
    /// unsorted).
    bool isSorted() const;

    //! \brief Returns \c true if storage has been optimized.
    /**
       Optimized storage means that the allocation of each row is equal to the
       number of entries. The effect is that a pass through the matrix, i.e.,
       during a mat-vec, requires minimal memory traffic. One limitation of
       optimized storage is that no new indices can be added to the graph.
    */
    bool isStorageOptimized() const;

    //! Returns \c true if the graph was allocated with static data structures.
    ProfileType getProfileType() const;

    /// \brief Get a copy of the given row, using global indices.
    ///
    /// \param GlobalRow [in] Global index of the row.
    /// \param Indices [out] On output: Global column indices.
    /// \param NumIndices [out] Number of indices returned.
    void
    getGlobalRowCopy (GlobalOrdinal GlobalRow,
                      const Teuchos::ArrayView<GlobalOrdinal>& Indices,
                      size_t& NumIndices) const;

    /// \brief Get a copy of the given row, using local indices.
    ///
    /// \param LocalRow [in] Local index of the row.
    /// \param Indices [out] On output: Local column indices.
    /// \param NumIndices [out] Number of indices returned.
    ///
    /// \pre <tt>hasColMap()</tt>
    void
    getLocalRowCopy (LocalOrdinal LocalRow,
                     const Teuchos::ArrayView<LocalOrdinal>& indices,
                     size_t& NumIndices) const;

    //! Extract a const, non-persisting view of global indices in a specified row of the graph.
    /*!
      \param GlobalRow - (In) Global row number for which indices are desired.
      \param Indices   - (Out) Global column indices corresponding to values.
      \pre <tt>isLocallyIndexed() == false</tt>
      \post <tt>indices.size() == getNumEntriesInGlobalRow(GlobalRow)</tt>

      Note: If \c GlobalRow does not belong to this node, then \c indices is set to null.
    */
    void
    getGlobalRowView (GlobalOrdinal GlobalRow,
                      Teuchos::ArrayView<const GlobalOrdinal>& Indices) const;

    //! Extract a const, non-persisting view of local indices in a specified row of the graph.
    /*!
      \param LocalRow - (In) Local row number for which indices are desired.
      \param Indices  - (Out) Global column indices corresponding to values.
      \pre <tt>isGloballyIndexed() == false</tt>
      \post <tt>indices.size() == getNumEntriesInLocalRow(LocalRow)</tt>

      Note: If \c LocalRow does not belong to this node, then \c indices is set to null.
    */
    void
    getLocalRowView (LocalOrdinal LocalRow,
                     Teuchos::ArrayView<const LocalOrdinal>& indices) const;

    //@}
    //! @name Overridden from Teuchos::Describable
    //@{

    /** \brief Return a simple one-line description of this object. */
    std::string description() const;

    //! Print the object to the given output stream with given verbosity level.
    void
    describe (Teuchos::FancyOStream& out,
              const Teuchos::EVerbosityLevel verbLevel =
              Teuchos::Describable::verbLevel_default) const;

    //@}
    //! \name Implementation of DistObject
    //@{

    virtual bool
    checkSizes (const SrcDistObject& source);

    virtual void
    copyAndPermute (const SrcDistObject& source,
                    size_t numSameIDs,
                    const Teuchos::ArrayView<const LocalOrdinal> &permuteToLIDs,
                    const Teuchos::ArrayView<const LocalOrdinal> &permuteFromLIDs);

    virtual void
    packAndPrepare (const SrcDistObject& source,
                    const Teuchos::ArrayView<const LocalOrdinal> &exportLIDs,
                    Teuchos::Array<GlobalOrdinal> &exports,
                    const Teuchos::ArrayView<size_t> & numPacketsPerLID,
                    size_t& constantNumPackets,
                    Distributor &distor);

    virtual void
    pack (const Teuchos::ArrayView<const LocalOrdinal>& exportLIDs,
          Teuchos::Array<GlobalOrdinal>& exports,
          const Teuchos::ArrayView<size_t>& numPacketsPerLID,
          size_t& constantNumPackets,
          Distributor& distor) const;

    virtual void
    unpackAndCombine (const Teuchos::ArrayView<const LocalOrdinal> &importLIDs,
                      const Teuchos::ArrayView<const GlobalOrdinal> &imports,
                      const Teuchos::ArrayView<size_t> &numPacketsPerLID,
                      size_t constantNumPackets,
                      Distributor &distor,
                      CombineMode CM);
    //@}
    //! \name Advanced methods, at increased risk of deprecation.
    //@{

    /// \brief Get an upper bound on the number of entries that can be
    ///   stored in each row.
    ///
    /// When a CrsGraph is constructed, callers must give an upper
    /// bound on the number of entries in each local row.  They may
    /// either supply a single integer which is the upper bound for
    /// all local rows, or they may give an array with a possibly
    /// different upper bound for each local row.
    ///
    /// This method returns the upper bound for each row.  If
    /// numEntriesPerLocalRowBound is Teuchos::null on output and
    /// boundSameForAllLocalRows is true on output, then
    /// numEntriesAllLocalRowsBound is the upper bound for all local
    /// rows.  If boundSameForAllLocalRows is false on output, then
    /// numEntriesPerLocalRowBound has zero or more entries on output,
    /// and numEntriesPerLocalRowBound[i_local] is the upper bound for
    /// local row i_local.
    ///
    /// The output argument boundSameForAllLocalRows is conservative;
    /// it only tells us whether boundForAllLocalRows has a meaningful
    /// value on output.  We don't necessarily check whether all
    /// entries of boundPerLocalRow are the same.
    void
    getNumEntriesPerLocalRowUpperBound (Teuchos::ArrayRCP<const size_t>& boundPerLocalRow,
                                        size_t& boundForAllLocalRows,
                                        bool& boundSameForAllLocalRows) const;

    /// \brief Set the graph's data directly, using 1-D storage.
    ///
    /// \pre <tt>hasColMap() == true</tt>
    /// \pre <tt>rowPointers.size() != getNodeNumRows()+1</tt>
    /// \pre No insert routines have been called.
    ///
    /// \warning This method is intended for expert developer use
    ///   only, and should never be called by user code.
    void
    setAllIndices (const typename local_graph_type::row_map_type& rowPointers,
                   const typename local_graph_type::entries_type::non_const_type& columnIndices);

    /// \brief Set the graph's data directly, using 1-D storage.
    ///
    /// \pre <tt>hasColMap() == true</tt>
    /// \pre <tt>rowPointers.size() != getNodeNumRows()+1</tt>
    /// \pre No insert routines have been called.
    ///
    /// \warning This method is intended for expert developer use
    ///   only, and should never be called by user code.
    void
    setAllIndices (const Teuchos::ArrayRCP<size_t> & rowPointers,
                   const Teuchos::ArrayRCP<LocalOrdinal> & columnIndices);

    /// \brief Get a host view of the row offsets.
    ///
    /// \note Please prefer getLocalGraph() to get the row offsets.
    ///
    /// This may return either a copy or a view of the row offsets.
    /// In either case, it will <i>always</i> live in host memory,
    /// never in (CUDA) device memory.
    Teuchos::ArrayRCP<const size_t> getNodeRowPtrs () const;

    //! Get an Teuchos::ArrayRCP of the packed column-indices.
    /*!  The returned buffer exists in host-memory.
     */
    Teuchos::ArrayRCP<const LocalOrdinal> getNodePackedIndices() const;

    /// \brief Replace the graph's current column Map with the given Map.
    ///
    /// This <i>only</i> replaces the column Map.  It does <i>not</i>
    /// change the graph's current column indices, or otherwise apply
    /// a permutation.  For example, suppose that before calling this
    /// method, the calling process owns a row containing local column
    /// indices [0, 2, 4].  These indices do <i>not</i> change, nor
    /// does their order change, as a result of calling this method.
    ///
    /// \param newColMap [in] New column Map.  Must be nonnull.
    void replaceColMap (const Teuchos::RCP<const map_type>& newColMap);

    /// \brief Reindex the column indices in place, and replace the
    ///   column Map.  Optionally, replace the Import object as well.
    ///
    /// \pre On every calling process, every index owned by the
    ///   current column Map must also be owned by the new column Map.
    ///
    /// \pre If the new Import object is provided, the new Import
    ///   object's source Map must be the same as the current domain
    ///   Map, and the new Import's target Map must be the same as the
    ///   new column Map.
    ///
    /// \param newColMap [in] New column Map.  Must be nonnull.
    ///
    /// \param newImport [in] New Import object.  Optional; computed
    ///   if not provided or if null.  Computing an Import is
    ///   expensive, so it is worth providing this if you can.
    ///
    /// \param sortIndicesInEachRow [in] If true, sort the indices in
    ///   each row after reindexing.
    void
    reindexColumns (const Teuchos::RCP<const map_type>& newColMap,
                    const Teuchos::RCP<const import_type>& newImport = Teuchos::null,
                    const bool sortIndicesInEachRow = true);

    /// \brief Replace the current domain Map and Import with the given parameters.
    ///
    /// \warning This method is ONLY for use by experts.
    /// \warning We make NO promises of backwards compatibility.
    ///   This method may change or disappear at any time.
    ///
    /// \pre <tt>isFillComplete() == true<tt>
    /// \pre <tt>isFillActive() == false<tt>
    /// \pre Either the given Import object is null, or the target Map
    ///   of the given Import is the same as this graph's column Map.
    /// \pre Either the given Import object is null, or the source Map
    ///    of the given Import is the same as this graph's domain Map.
    void
    replaceDomainMapAndImporter (const Teuchos::RCP<const map_type>& newDomainMap,
                                 const Teuchos::RCP<const import_type>& newImporter);

    /// \brief Remove processes owning zero rows from the Maps and their communicator.
    ///
    /// \warning This method is ONLY for use by experts.  We highly
    ///   recommend using the nonmember function of the same name
    ///   defined in Tpetra_DistObject_decl.hpp.
    ///
    /// \warning We make NO promises of backwards compatibility.
    ///   This method may change or disappear at any time.
    ///
    /// \param newMap [in] This <i>must</i> be the result of calling
    ///   the removeEmptyProcesses() method on the row Map.  If it
    ///   is not, this method's behavior is undefined.  This pointer
    ///   will be null on excluded processes.
    ///
    /// This method satisfies the strong exception guarantee, as
    /// long the destructors of Export, Import, and Map do not throw
    /// exceptions.  This means that either the method returns
    /// normally (without throwing an exception), or there are no
    /// externally visible side effects.  However, this does not
    /// guarantee no deadlock when the graph's original communicator
    /// contains more than one process.  In order to prevent
    /// deadlock, you must still wrap this call in a try/catch block
    /// and do an all-reduce over all processes in the original
    /// communicator to test whether the call succeeded.  This
    /// safety measure should usually be unnecessary, since the
    /// method call should only fail on user error or failure to
    /// allocate memory.
    virtual void
    removeEmptyProcessesInPlace (const Teuchos::RCP<const map_type>& newMap);
    //@}

    template<class ViewType, class OffsetViewType >
    struct pack_functor {
      typedef typename ViewType::execution_space execution_space;
      ViewType src;
      ViewType dest;
      OffsetViewType src_offset;
      OffsetViewType dest_offset;
      typedef typename OffsetViewType::non_const_value_type ScalarIndx;

      pack_functor(ViewType dest_, ViewType src_, OffsetViewType dest_offset_, OffsetViewType src_offset_):
        src(src_),dest(dest_),src_offset(src_offset_),dest_offset(dest_offset_) {};

      KOKKOS_INLINE_FUNCTION
      void operator() (size_t row) const {
        ScalarIndx i = src_offset(row);
        ScalarIndx j = dest_offset(row);
        const ScalarIndx k = dest_offset(row+1);
        for(;j<k;j++,i++) {
          dest(j) = src(i);
        }
      }
    };

  protected:
    // these structs are conveniences, to cut down on the number of
    // arguments to some of the methods below.
    struct SLocalGlobalViews {
      Teuchos::ArrayView<const GlobalOrdinal> ginds;
      Teuchos::ArrayView<const LocalOrdinal>  linds;
    };
    struct SLocalGlobalNCViews {
      Teuchos::ArrayView<GlobalOrdinal>       ginds;
      Teuchos::ArrayView<LocalOrdinal>        linds;
    };

    bool indicesAreAllocated () const;
    void allocateIndices (const ELocalGlobal lg);

    template <class T>
    Teuchos::ArrayRCP<Teuchos::Array<T> > allocateValues2D () const;

    template <class T>
    RowInfo updateLocalAllocAndValues (const RowInfo rowInfo,
                                       const size_t newAllocSize,
                                       Teuchos::Array<T>& rowVals)
    {
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPT( ! isLocallyIndexed () );
      TEUCHOS_TEST_FOR_EXCEPT( ! indicesAreAllocated() );
      TEUCHOS_TEST_FOR_EXCEPT( newAllocSize == 0 );
      TEUCHOS_TEST_FOR_EXCEPT( newAllocSize < rowInfo.allocSize );
      TEUCHOS_TEST_FOR_EXCEPT( ! rowMap_->isNodeLocalElement (rowInfo.localRow) );
#endif // HAVE_TPETRA_DEBUG

      // Teuchos::ArrayRCP::resize automatically copies over values on reallocation.
      lclInds2D_[rowInfo.localRow].resize (newAllocSize);
      rowVals.resize (newAllocSize);
      nodeNumAllocated_ += (newAllocSize - rowInfo.allocSize);

      RowInfo rowInfoOut = rowInfo;
      rowInfoOut.allocSize = newAllocSize;
      return rowInfoOut;
    }

    template <class T>
    RowInfo
    updateGlobalAllocAndValues (const RowInfo rowInfo,
                                const size_t newAllocSize,
                                Teuchos::Array<T>& rowVals)
    {
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPT( ! isGloballyIndexed () );
      TEUCHOS_TEST_FOR_EXCEPT( ! indicesAreAllocated () );
      TEUCHOS_TEST_FOR_EXCEPT( newAllocSize == 0 );
      TEUCHOS_TEST_FOR_EXCEPT( newAllocSize < rowInfo.allocSize );
      TEUCHOS_TEST_FOR_EXCEPT( ! rowMap_->isNodeLocalElement (rowInfo.localRow) );
#endif // HAVE_TPETRA_DEBUG

      // Teuchos::ArrayRCP::resize automatically copies over values on reallocation.
      gblInds2D_[rowInfo.localRow].resize (newAllocSize);
      rowVals.resize (newAllocSize);
      nodeNumAllocated_ += (newAllocSize - rowInfo.allocSize);

      RowInfo rowInfoOut = rowInfo;
      rowInfoOut.allocSize = newAllocSize;
      return rowInfoOut;
    }

    //! \name Methods governing changes between global and local indices
    //@{

    //! Make the graph's column Map, if it does not already have one.
    void makeColMap ();
    void makeIndicesLocal ();
    void makeImportExport ();

    //@}
    //! \name Methods for inserting indices or transforming values
    //@{

    template<ELocalGlobal lg>
    size_t filterIndices (const SLocalGlobalNCViews& inds) const
    {
      using Teuchos::ArrayView;
      static_assert (lg == GlobalIndices || lg == LocalIndices,
                     "Tpetra::CrsGraph::filterIndices: The template parameter "
                     "lg must be either GlobalIndices or LocalIndicies.");

      const map_type& cmap = *colMap_;
      size_t numFiltered = 0;
#ifdef HAVE_TPETRA_DEBUG
      size_t numFiltered_debug = 0;
#endif
      if (lg == GlobalIndices) {
        ArrayView<GlobalOrdinal> ginds = inds.ginds;
        typename ArrayView<GlobalOrdinal>::iterator fend = ginds.begin();
        typename ArrayView<GlobalOrdinal>::iterator cptr = ginds.begin();
        while (cptr != ginds.end()) {
          if (cmap.isNodeGlobalElement(*cptr)) {
            *fend++ = *cptr;
#ifdef HAVE_TPETRA_DEBUG
            ++numFiltered_debug;
#endif
          }
          ++cptr;
        }
        numFiltered = fend - ginds.begin();
      }
      else if (lg == LocalIndices) {
        ArrayView<LocalOrdinal> linds = inds.linds;
        typename ArrayView<LocalOrdinal>::iterator fend = linds.begin();
        typename ArrayView<LocalOrdinal>::iterator cptr = linds.begin();
        while (cptr != linds.end()) {
          if (cmap.isNodeLocalElement(*cptr)) {
            *fend++ = *cptr;
#ifdef HAVE_TPETRA_DEBUG
            ++numFiltered_debug;
#endif
          }
          ++cptr;
        }
        numFiltered = fend - linds.begin();
      }
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPT( numFiltered != numFiltered_debug );
#endif
      return numFiltered;
    }


    template<class T>
    size_t
    filterGlobalIndicesAndValues (const Teuchos::ArrayView<GlobalOrdinal>& ginds,
                                  const Teuchos::ArrayView<T>& vals) const
    {
      using Teuchos::ArrayView;
      const map_type& cmap = *colMap_;
      size_t numFiltered = 0;
      typename ArrayView<T>::iterator fvalsend = vals.begin();
      typename ArrayView<T>::iterator valscptr = vals.begin();
#ifdef HAVE_TPETRA_DEBUG
      size_t numFiltered_debug = 0;
#endif
      typename ArrayView<GlobalOrdinal>::iterator fend = ginds.begin();
      typename ArrayView<GlobalOrdinal>::iterator cptr = ginds.begin();
      while (cptr != ginds.end()) {
        if (cmap.isNodeGlobalElement (*cptr)) {
          *fend++ = *cptr;
          *fvalsend++ = *valscptr;
#ifdef HAVE_TPETRA_DEBUG
          ++numFiltered_debug;
#endif
        }
        ++cptr;
        ++valscptr;
      }
      numFiltered = fend - ginds.begin();
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPT( numFiltered != numFiltered_debug );
      TEUCHOS_TEST_FOR_EXCEPT( valscptr != vals.end() );
      const size_t numFilteredActual =
        static_cast<size_t> (fvalsend - vals.begin ());
      TEUCHOS_TEST_FOR_EXCEPT( numFiltered != numFilteredActual );
#endif // HAVE_TPETRA_DEBUG
      return numFiltered;
    }

    template<class T>
    size_t
    filterLocalIndicesAndValues (const Teuchos::ArrayView<LocalOrdinal>& linds,
                                 const Teuchos::ArrayView<T>& vals) const
    {
      using Teuchos::ArrayView;
      const map_type& cmap = *colMap_;
      size_t numFiltered = 0;
      typename ArrayView<T>::iterator fvalsend = vals.begin();
      typename ArrayView<T>::iterator valscptr = vals.begin();
#ifdef HAVE_TPETRA_DEBUG
      size_t numFiltered_debug = 0;
#endif
      typename ArrayView<LocalOrdinal>::iterator fend = linds.begin();
      typename ArrayView<LocalOrdinal>::iterator cptr = linds.begin();
      while (cptr != linds.end()) {
        if (cmap.isNodeLocalElement (*cptr)) {
          *fend++ = *cptr;
          *fvalsend++ = *valscptr;
#ifdef HAVE_TPETRA_DEBUG
          ++numFiltered_debug;
#endif
        }
        ++cptr;
        ++valscptr;
      }
      numFiltered = fend - linds.begin();
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPT( numFiltered != numFiltered_debug );
      TEUCHOS_TEST_FOR_EXCEPT( valscptr != vals.end() );
      const size_t numFilteredActual =
        Teuchos::as<size_t> (fvalsend - vals.begin ());
      TEUCHOS_TEST_FOR_EXCEPT( numFiltered != numFilteredActual );
#endif
      return numFiltered;
    }

    /// \brief Insert indices into the given row.
    ///
    /// \pre <tt>! (lg == LocalIndices && I == GlobalIndices)</tt>.
    ///   It does not make sense to give this method local column
    ///   indices (meaning that the graph has a column Map), yet to
    ///   ask it to store global indices.
    ///
    /// \param rowInfo [in] Result of CrsGraph's getRowInfo() or
    ///   updateAllocAndValues() methods, for the locally owned row
    ///   (whose local index is <tt>rowInfo.localRow</tt>) for which
    ///   you want to insert indices.
    ///
    /// \param newInds [in] View of the column indices to insert.  If
    ///   <tt>lg == GlobalIndices</tt>, then newInds.ginds, a
    ///   <tt>Teuchos::ArrayView<const GlobalOrdinal></tt>, contains
    ///   the (global) column indices to insert.  Otherwise, if <tt>lg
    ///   == LocalIndices</tt>, then newInds.linds, a
    ///   <tt>Teuchos::ArrayView<const LocalOrdinal></tt>, contains
    ///   the (local) column indices to insert.
    ///
    /// \param lg If <tt>lg == GlobalIndices</tt>, then the input
    ///   indices (in \c newInds) are global indices.  Otherwise, if
    ///   <tt>lg == LocalIndices</tt>, the input indices are local
    ///   indices.
    ///
    /// \param I If <tt>lg == GlobalIndices</tt>, then this method
    ///   will store the input indices as global indices.  Otherwise,
    ///   if <tt>I == LocalIndices</tt>, this method will store the
    ///   input indices as local indices.
    size_t
    insertIndices (const RowInfo& rowInfo,
                   const SLocalGlobalViews& newInds,
                   const ELocalGlobal lg,
                   const ELocalGlobal I);

    /// \brief Insert indices and their values into the given row.
    ///
    /// \tparam Scalar The type of a single value.  When this method
    ///   is called by CrsMatrix, \c Scalar corresponds to the first
    ///   template parameter of CrsMatrix.
    ///
    /// \pre <tt>! (lg == LocalIndices && I == GlobalIndices)</tt>.
    ///   It does not make sense to give this method local column
    ///   indices (meaning that the graph has a column Map), yet to
    ///   ask it to store global indices.
    ///
    /// \param rowInfo [in] Result of CrsGraph's getRowInfo() or
    ///   updateAllocAndValues() methods, for the locally owned row
    ///   (whose local index is <tt>rowInfo.localRow</tt>) for which
    ///   you want to insert indices.
    ///
    /// \param newInds [in] View of the column indices to insert.  If
    ///   <tt>lg == GlobalIndices</tt>, then newInds.ginds, a
    ///   <tt>Teuchos::ArrayView<const GlobalOrdinal></tt>, contains
    ///   the (global) column indices to insert.  Otherwise, if <tt>lg
    ///   == LocalIndices</tt>, then newInds.linds, a
    ///   <tt>Teuchos::ArrayView<const LocalOrdinal></tt>, contains
    ///   the (local) column indices to insert.
    ///
    /// \param oldRowVals [out] View of the current values.  They will
    ///   be overwritten with the new values.
    ///
    /// \param newRowVals [in] View of the new values.  They will be
    ///   copied over the old values.
    ///
    /// \param lg If <tt>lg == GlobalIndices</tt>, then the input
    ///   indices (in \c newInds) are global indices.  Otherwise, if
    ///   <tt>lg == LocalIndices</tt>, the input indices are local
    ///   indices.
    ///
    /// \param I If <tt>lg == GlobalIndices</tt>, then this method
    ///   will store the input indices as global indices.  Otherwise,
    ///   if <tt>I == LocalIndices</tt>, this method will store the
    ///   input indices as local indices.
    template<class Scalar>
    void
    insertIndicesAndValues (const RowInfo& rowInfo,
                            const SLocalGlobalViews& newInds,
                            const Teuchos::ArrayView<Scalar>& oldRowVals,
                            const Teuchos::ArrayView<const Scalar>& newRowVals,
                            const ELocalGlobal lg,
                            const ELocalGlobal I);
    void
    insertGlobalIndicesImpl (const LocalOrdinal myRow,
                             const Teuchos::ArrayView<const GlobalOrdinal> &indices);
    void
    insertLocalIndicesImpl (const LocalOrdinal myRow,
                            const Teuchos::ArrayView<const LocalOrdinal> &indices);
    //! Like insertLocalIndices(), but with column Map filtering.
    void
    insertLocalIndicesFiltered (const LocalOrdinal localRow,
                                const Teuchos::ArrayView<const LocalOrdinal> &indices);

    //! Like insertGlobalIndices(), but with column Map filtering.
    void
    insertGlobalIndicesFiltered (const GlobalOrdinal localRow,
                                 const Teuchos::ArrayView<const GlobalOrdinal> &indices);

    /// \brief Transform the given values using local indices.
    ///
    /// \param rowInfo [in] Information about a given row of the graph.
    ///
    /// \param rowVals [in/out] The values to be transformed.  They
    ///   correspond to the row indicated by rowInfo.
    ///
    /// \param inds [in] The (local) indices in the row, for which
    ///   to transform the corresponding values in rowVals.
    ///
    /// \param newVals [in] Values to use for transforming rowVals.
    ///   It's probably OK for these to alias rowVals.
    ///
    /// \param f [in] A binary function used to transform rowVals.
    ///
    /// This method transforms the values using the expression
    /// \code
    /// newVals[k] = f( rowVals[k], newVals[j] );
    /// \endcode
    /// where k is the local index corresponding to
    /// <tt>inds[j]</tt>.  It ignores elements of \c inds that are
    /// not owned by the calling process.
    ///
    /// \return The number of valid local column indices.  In case of
    ///   error other than one or more invalid column indices, this
    ///   method returns
    ///   Teuchos::OrdinalTraits<LocalOrdinal>::invalid().
    template<class Scalar, class BinaryFunction>
    LocalOrdinal
    transformLocalValues (const RowInfo& rowInfo,
                          const Teuchos::ArrayView<Scalar>& rowVals,
                          const Teuchos::ArrayView<const LocalOrdinal>& inds,
                          const Teuchos::ArrayView<const Scalar>& newVals,
                          BinaryFunction f) const
    {
      typedef typename Teuchos::ArrayView<Scalar>::size_type size_type;
      const size_t STINV = Teuchos::OrdinalTraits<size_t>::invalid ();
      const size_type numElts = inds.size ();
      size_t hint = 0; // Guess for the current index k into rowVals

      // Get a view of the column indices in the row.  This amortizes
      // the cost of getting the view over all the entries of inds.
      auto colInds = getLocalKokkosRowView (rowInfo);

      LocalOrdinal numValid = 0; // number of valid local column indices
      for (size_type j = 0; j < numElts; ++j) {
        const size_t k = findLocalIndex (rowInfo, inds[j], colInds, hint);
        if (k != STINV) {
          rowVals[k] = f (rowVals[k], newVals[j]); // use binary function f
          hint = k+1;
          ++numValid;
        }
      }
      return numValid;
    }

  private:
    /// \brief Whether sumIntoLocalValues should use atomic updates by
    ///   default.
    ///
    /// \warning This is an implementation detail.
    static const bool useAtomicUpdatesByDefault =
#ifdef KOKKOS_HAVE_SERIAL
      ! std::is_same<execution_space, Kokkos::Serial>::value;
#else
      true;
#endif // KOKKOS_HAVE_SERIAL

    /// \brief Implementation detail of CrsMatrix::sumIntoLocalValues.
    ///
    /// \tparam Scalar The type of each entry in the sparse matrix.
    /// \tparam InputMemorySpace Kokkos memory space / device in which
    ///   the input data live.  This may differ from the memory space
    ///   in which the current matrix values (rowVals) live.
    /// \tparam ValsMemorySpace Kokkos memory space / device in which
    ///   the matrix's current values live.  This may differ from the
    ///   memory space in which the input data (inds and newVals)
    ///   live.
    ///
    /// \param rowInfo [in] Result of getRowInfo on the index of the
    ///   local row of the matrix to modify.
    /// \param rowVals [in/out] On input: Values of the row of the
    ///   sparse matrix to modify.  On output: The modified values.
    /// \param inds [in] Local column indices of that row to modify.
    /// \param newVals [in] For each k, increment the value in rowVals
    ///   corresponding to local column index inds[k] by newVals[k].
    template<class Scalar, class InputMemorySpace, class ValsMemorySpace>
    LocalOrdinal
    sumIntoLocalValues (const RowInfo& rowInfo,
                        const Kokkos::View<Scalar*, ValsMemorySpace,
                          Kokkos::MemoryUnmanaged>& rowVals,
                        const Kokkos::View<const LocalOrdinal*, InputMemorySpace,
                          Kokkos::MemoryUnmanaged>& inds,
                        const Kokkos::View<const Scalar*, InputMemorySpace,
                        Kokkos::MemoryUnmanaged>& newVals,
                        const bool atomic = useAtomicUpdatesByDefault) const
    {
      // NOTE (mfh 11 Oct 2015) This method assumes UVM.  More
      // accurately, it assumes that the host execution space can
      // access data in both InputMemorySpace and ValsMemorySpace.

      const size_t STINV = Teuchos::OrdinalTraits<size_t>::invalid ();
      const LocalOrdinal numElts = static_cast<LocalOrdinal> (inds.dimension_0 ());
      size_t hint = 0; // Guess for the current index k into rowVals

      // Get a view of the column indices in the row.  This amortizes
      // the cost of getting the view over all the entries of inds.
      auto colInds = this->getLocalKokkosRowView (rowInfo);

      LocalOrdinal numValid = 0; // number of valid local column indices
      for (LocalOrdinal j = 0; j < numElts; ++j) {
        const size_t k = this->findLocalIndex (rowInfo, inds(j), colInds, hint);
        if (k != STINV) {
          if (atomic) {
            Kokkos::atomic_add (&rowVals(k), newVals(j));
          }
          else {
            rowVals(k) += newVals(j);
          }
          hint = k+1;
          ++numValid;
        }
      }
      return numValid;
    }

    /// \brief Implementation detail of CrsMatrix::replaceLocalValues.
    ///
    /// \tparam Scalar The type of each entry in the sparse matrix.
    /// \tparam InputMemorySpace Kokkos memory space / device in which
    ///   the input data live.  This may differ from the memory space
    ///   in which the current matrix values (rowVals) live.
    /// \tparam ValsMemorySpace Kokkos memory space / device in which
    ///   the matrix's current values live.  This may differ from the
    ///   memory space in which the input data (inds and newVals)
    ///   live.
    ///
    /// \param rowInfo [in] Result of getRowInfo on the index of the
    ///   local row of the matrix to modify.
    /// \param rowVals [in/out] On input: Values of the row of the
    ///   sparse matrix to modify.  On output: The modified values.
    /// \param inds [in] Local column indices of that row to modify.
    /// \param newVals [in] For each k, replace the value
    ///   corresponding to local column index inds[k] with newVals[k].
    template<class Scalar, class InputMemorySpace, class ValsMemorySpace>
    LocalOrdinal
    replaceLocalValues (const RowInfo& rowInfo,
                        const Kokkos::View<Scalar*, ValsMemorySpace,
                          Kokkos::MemoryUnmanaged>& rowVals,
                        const Kokkos::View<const LocalOrdinal*, InputMemorySpace,
                          Kokkos::MemoryUnmanaged>& inds,
                        const Kokkos::View<const Scalar*, InputMemorySpace,
                          Kokkos::MemoryUnmanaged>& newVals) const
    {
      // NOTE (mfh 11 Oct 2015) This method assumes UVM.  More
      // accurately, it assumes that the host execution space can
      // access data in both InputMemorySpace and ValsMemorySpace.

      const size_t STINV = Teuchos::OrdinalTraits<size_t>::invalid ();
      const LocalOrdinal numElts = static_cast<LocalOrdinal> (inds.size ());
      size_t hint = 0; // Guess for the current index k into rowVals

      // Get a view of the column indices in the row.  This amortizes
      // the cost of getting the view over all the entries of inds.
      auto colInds = this->getLocalKokkosRowView (rowInfo);

      LocalOrdinal numValid = 0; // number of valid local column indices
      for (LocalOrdinal j = 0; j < numElts; ++j) {
        const size_t k = this->findLocalIndex (rowInfo, inds(j), colInds, hint);
        if (k != STINV) {
          rowVals(k) = newVals[j];
          hint = k+1;
          ++numValid;
        }
      }
      return numValid;
    }

  protected:
    /// \brief Transform the given values using global indices.
    ///
    /// \param rowInfo [in] Information about a given row of the graph.
    ///
    /// \param rowVals [in/out] The values to be transformed.  They
    ///   correspond to the row indicated by rowInfo.
    ///
    /// \param inds [in] The (global) indices in the row, for which
    ///   to transform the corresponding values in rowVals.
    ///
    /// \param newVals [in] Values to use for transforming rowVals.
    ///   It's probably OK for these to alias rowVals.
    ///
    /// \param f [in] A binary function used to transform rowVals.
    ///
    /// \return The number of valid local column indices.  In case of
    ///   error other than one or more invalid column indices, this
    ///   method returns
    ///   Teuchos::OrdinalTraits<LocalOrdinal>::invalid().
    template<class Scalar, class BinaryFunction>
    LocalOrdinal
    transformGlobalValues (RowInfo rowInfo,
                           const Teuchos::ArrayView<Scalar>& rowVals,
                           const Teuchos::ArrayView<const GlobalOrdinal>& inds,
                           const Teuchos::ArrayView<const Scalar>& newVals,
                           BinaryFunction f,
                           const bool atomic = useAtomicUpdatesByDefault) const
    {
      typedef typename Teuchos::ArrayView<Scalar>::size_type size_type;
      typedef LocalOrdinal LO;
      const size_t STINV = Teuchos::OrdinalTraits<size_t>::invalid ();
      const size_type numElts = inds.size ();
      size_t hint = 0; // guess at the index's relative offset in the row

      LO numValid = 0; // number of valid input column indices

      if (isLocallyIndexed ()) {
        // NOTE (mfh 04 Nov 2015) Dereferencing an RCP or reading its
        // pointer do NOT change its reference count.  Thus, this code
        // is still thread safe.
        if (colMap_.is_null ()) {
          // NO input column indices are valid in this case, since if
          // the column Map is null on the calling process, then the
          // calling process owns no graph entries.
          return numValid;
        }
        const map_type& colMap = *colMap_;
        const LO LINV = Teuchos::OrdinalTraits<LO>::invalid ();

        for (size_type j = 0; j < numElts; ++j) {
          const LocalOrdinal lclColInd = colMap.getLocalElement (inds[j]);
          if (lclColInd != LINV) {
            const size_t k = findLocalIndex (rowInfo, lclColInd, hint);
            if (k != STINV) {
              if (atomic) {
                const Scalar newVal = f (rowVals[k], newVals[j]);
                Kokkos::atomic_assign (&rowVals[k], newVal);
              }
              else {
                rowVals[k] = f (rowVals[k], newVals[j]); // use binary function f
              }
              hint = k+1;
              numValid++;
            }
          }
        }
      }
      else if (isGloballyIndexed ()) {
        for (size_type j = 0; j < numElts; ++j) {
          const size_t k = findGlobalIndex (rowInfo, inds[j], hint);
          if (k != STINV) {
            if (atomic) {
                const Scalar newVal = f (rowVals[k], newVals[j]);
                Kokkos::atomic_assign (&rowVals[k], newVal);
            }
            else {
              rowVals[k] = f (rowVals[k], newVals[j]); // use binary function f
            }
            hint = k+1;
            numValid++;
          }
        }
      }
      // If the graph is neither locally nor globally indexed on the
      // calling process, that means the calling process has no graph
      // entries.  Thus, none of the input column indices are valid.

      return numValid;
    }

    //@}
    //! \name Methods for sorting and merging column indices.
    //@{

    //! Whether duplicate column indices in each row have been merged.
    bool isMerged () const;

    /// \brief Report that we made a local modification to its structure.
    ///
    /// Call this after making a local change to the graph's
    /// structure.  Changing the structure locally invalidates the "is
    /// sorted" and "is merged" states.
    void setLocallyModified ();

    //! Sort the column indices in all the rows.
    void sortAllIndices ();

    //! Sort the column indices in the given row.
    void sortRowIndices (const RowInfo rowinfo);

    /// \brief Sort the column indices and their values in the given row.
    ///
    /// \tparam Scalar The type of the values.  When calling this
    ///   method from CrsMatrix, this should be the same as the
    ///   <tt>Scalar</tt> template parameter of CrsMatrix.
    ///
    /// \param rowinfo [in] Result of getRowInfo() for the row.
    ///
    /// \param values [in/out] On input: values for the given row.  If
    ///   indices is an array of the column indices in the row, then
    ///   values and indices should have the same number of entries,
    ///   and indices[k] should be the column index corresponding to
    ///   values[k].  On output: the same values, but sorted in the
    ///   same order as the (now sorted) column indices in the row.
    template <class Scalar>
    void sortRowIndicesAndValues (const RowInfo rowinfo,
                                  const Teuchos::ArrayView<Scalar>& values);

    /// \brief Merge duplicate row indices in all of the rows.
    ///
    /// \pre The graph is locally indexed:
    ///   <tt>isGloballyIndexed() == false</tt>.
    ///
    /// \pre The graph has not already been merged: <tt>isMerged()
    ///   == false</tt>.  That is, this function would normally only
    ///   be called after calling sortIndices().
    void mergeAllIndices ();

    /// \brief Merge duplicate row indices in the given row.
    ///
    /// \pre The graph is not already storage optimized:
    ///   <tt>isStorageOptimized() == false</tt>
    void mergeRowIndices (RowInfo rowinfo);

    /// \brief Merge duplicate row indices in the given row, along
    ///   with their corresponding values.
    ///
    /// This method is only called by CrsMatrix, for a CrsMatrix whose
    /// graph is this CrsGraph instance.  It is only called when the
    /// matrix owns the graph, not when the matrix was constructed
    /// with a const graph.
    ///
    /// \pre The graph is not already storage optimized:
    ///   <tt>isStorageOptimized() == false</tt>
    template<class Scalar>
    void
    mergeRowIndicesAndValues (RowInfo rowinfo,
                              const Teuchos::ArrayView<Scalar>& rowValues);
    //@}

    /// Set the domain and range Maps, and invalidate the Import
    /// and/or Export objects if necessary.
    ///
    /// If the domain Map has changed, invalidate the Import object
    /// (if there is one).  Likewise, if the range Map has changed,
    /// invalidate the Export object (if there is one).
    ///
    /// \param domainMap [in] The new domain Map
    /// \param rangeMap [in] The new range Map
    void
    setDomainRangeMaps (const Teuchos::RCP<const map_type>& domainMap,
                        const Teuchos::RCP<const map_type>& rangeMap);

    void staticAssertions() const;
    void clearGlobalConstants();
    void computeGlobalConstants();

    /// \brief Get information about the locally owned row with local
    ///   index myRow.
    RowInfo getRowInfo (const size_t myRow) const;

    /// \brief Get a const, nonowned, locally indexed view of the
    ///   locally owned row myRow, such that rowinfo =
    ///   getRowInfo(myRow).
    Teuchos::ArrayView<const LocalOrdinal>
    getLocalView (const RowInfo rowinfo) const;

    /// \brief Get a nonconst, nonowned, locally indexed view of the
    ///   locally owned row myRow, such that rowinfo =
    ///   getRowInfo(myRow).
    Teuchos::ArrayView<LocalOrdinal>
    getLocalViewNonConst (const RowInfo rowinfo);

  private:

    /// \brief Get a const nonowned view of the local column indices
    ///   of row rowinfo.localRow.
    ///
    /// \param rowinfo [in] Result of calling getRowInfo with the
    ///   index of the local row to view.
    Kokkos::View<const LocalOrdinal*, execution_space, Kokkos::MemoryUnmanaged>
    getLocalKokkosRowView (const RowInfo& rowinfo) const;

    /// \brief Get a nonconst nonowned view of the local column
    ///   indices of row rowinfo.localRow.
    ///
    /// \param rowinfo [in] Result of calling getRowInfo with the
    ///   index of the local row to view.
    Kokkos::View<LocalOrdinal*, execution_space, Kokkos::MemoryUnmanaged>
    getLocalKokkosRowViewNonConst (const RowInfo& rowinfo);

  protected:

    /// \brief Get a const, nonowned, globally indexed view of the
    ///   locally owned row myRow, such that rowinfo =
    ///   getRowInfo(myRow).
    Teuchos::ArrayView<const GlobalOrdinal>
    getGlobalView (const RowInfo rowinfo) const;

    /// \brief Get a nonconst, nonowned, globally indexed view of the
    ///   locally owned row myRow, such that rowinfo =
    ///   getRowInfo(myRow).
    Teuchos::ArrayView<GlobalOrdinal>
    getGlobalViewNonConst (const RowInfo rowinfo);

    /// \brief Find the column offset corresponding to the given
    ///   (local) column index.
    ///
    /// The name of this method is a bit misleading.  It does not
    /// actually find the column index.  Instead, it takes a local
    /// column index \c ind, and returns the corresponding offset
    /// into the raw array of column indices (whether that be 1-D or
    /// 2-D storage).
    ///
    /// \param rowinfo [in] Result of getRowInfo() for the given row.
    /// \param ind [in] (Local) column index for which to find the offset.
    /// \param hint [in] Hint for where to find \c ind in the column
    ///   indices for the given row.  If colInds is the ArrayView of
    ///   the (local) column indices for the given row, and if
    ///   <tt>colInds[hint] == ind</tt>, then the hint is correct.
    ///   The hint is ignored if it is out of range (that is,
    ///   greater than or equal to the number of entries in the
    ///   given row).
    ///
    /// The hint optimizes for the case of calling this method several
    /// times with the same row (as it would be in
    /// transformLocalValues) when several index inputs occur in
    /// consecutive sequence.  This may occur (for example) when there
    /// are multiple degrees of freedom per mesh point, and users are
    /// handling the assignment of degrees of freedom to global
    /// indices manually (rather than letting some other class take
    /// care of it).  In that case, users might choose to assign the
    /// degrees of freedom for a mesh point to consecutive global
    /// indices.  Epetra implements the hint for this reason.
    ///
    /// The hint only costs two comparisons (one to check range, and
    /// the other to see if the hint was correct), and it can save
    /// searching for the indices (which may take a lot more than
    /// two comparisons).
    size_t
    findLocalIndex (const RowInfo& rowinfo,
                    const LocalOrdinal ind,
                    const size_t hint = 0) const;

    /// \brief Find the column offset corresponding to the given
    ///   (local) column index, given a view of the (local) column
    ///   indices.
    ///
    /// The name of this method is a bit misleading.  It does not
    /// actually find the column index.  Instead, it takes a local
    /// column index \c ind, and returns the corresponding offset
    /// into the raw array of column indices (whether that be 1-D or
    /// 2-D storage).
    ///
    /// It is best to use this method if you plan to call it several
    /// times for the same row, like in transformLocalValues().  In
    /// that case, it amortizes the overhead of calling
    /// getLocalKokkosRowView().
    ///
    /// \param rowinfo [in] Result of getRowInfo() for the given row.
    /// \param ind [in] (Local) column index for which to find the offset.
    /// \param colInds [in] View of all the (local) column indices
    ///   for the given row.
    /// \param hint [in] Hint for where to find \c ind in the column
    ///   indices for the given row.  If colInds is the ArrayView of
    ///   the (local) column indices for the given row, and if
    ///   <tt>colInds[hint] == ind</tt>, then the hint is correct.
    ///   The hint is ignored if it is out of range (that is,
    ///   greater than or equal to the number of entries in the
    ///   given row).
    ///
    /// See the documentation of the three-argument version of this
    /// method for an explanation and justification of the hint.
    size_t
    findLocalIndex (const RowInfo& rowinfo,
                    const LocalOrdinal ind,
                    const Kokkos::View<const LocalOrdinal*, device_type, Kokkos::MemoryUnmanaged>& colInds,
                    const size_t hint) const;

    /// \brief Legacy version of the 4-argument findLocalIndex above,
    ///   that takes a Teuchos::ArrayView instead of a Kokkos::View.
    size_t
    findLocalIndex (const RowInfo& rowinfo,
                    const LocalOrdinal ind,
                    const Teuchos::ArrayView<const LocalOrdinal>& colInds,
                    const size_t hint = 0) const;

    /// \brief Find the column offset corresponding to the given
    ///   (global) column index.
    ///
    /// The name of this method is a bit misleading.  It does not
    /// actually find the column index.  Instead, it takes a global
    /// column index \c ind, and returns the corresponding offset
    /// into the raw array of column indices (whether that be 1-D or
    /// 2-D storage).
    size_t findGlobalIndex (RowInfo rowinfo, GlobalOrdinal ind, size_t hint = 0) const;

    /// \brief Get the local graph.
    ///
    /// This is only a valid representation of the local graph if the
    /// (global) graph is fill complete.
    local_graph_type getLocalGraph () const;

    //! Get the local graph (DEPRECATED: call getLocalGraph() instead).
    TPETRA_DEPRECATED local_graph_type getLocalGraph_Kokkos () const;

    void fillLocalGraph (const Teuchos::RCP<Teuchos::ParameterList>& params);

    //! Whether it is correct to call getRowInfo().
    bool hasRowInfo () const;

    //! Throw an exception if the internal state is not consistent.
    void checkInternalState () const;

    //! The Map describing the distribution of rows of the graph.
    Teuchos::RCP<const map_type> rowMap_;
    //! The Map describing the distribution of columns of the graph.
    Teuchos::RCP<const map_type> colMap_;
    //! The Map describing the range of the (matrix corresponding to the) graph.
    Teuchos::RCP<const map_type> rangeMap_;
    //! The Map describing the domain of the (matrix corresponding to the) graph.
    Teuchos::RCP<const map_type> domainMap_;

    /// \brief The Import from the domain Map to the column Map.
    ///
    /// This gets constructed by fillComplete.  It may be null if
    /// the domain Map and the column Map are the same, since no
    /// Import is necessary in that case for sparse matrix-vector
    /// multiply.
    Teuchos::RCP<const import_type> importer_;

    /// \brief The Export from the row Map to the range Map.
    ///
    /// This gets constructed by fillComplete.  It may be null if
    /// the row Map and the range Map are the same, since no Export
    /// is necessary in that case for sparse matrix-vector multiply.
    Teuchos::RCP<const export_type> exporter_;

    //! Local graph; only initialized after first fillComplete() call.
    local_graph_type lclGraph_;

    // Local and Global Counts
    // nodeNumEntries_ and nodeNumAllocated_ are required to be always consistent
    // nodeMaxNumEntries_, nodeNumDiags_ and the global quantities are computed during fillComplete() and only valid when isFillComplete()
    global_size_t globalNumEntries_, globalNumDiags_, globalMaxNumRowEntries_;
    size_t          nodeNumEntries_,   nodeNumDiags_,   nodeMaxNumRowEntries_, nodeNumAllocated_;

    //! Whether the graph was allocated with static or dynamic profile.
    ProfileType pftype_;

    /// \brief The maximum number of entries to allow in each locally
    ///   owned row, per row.
    ///
    /// This is an argument to some of the graph's constructors.
    /// Either this or numAllocForAllRows_ is used, but not both.
    /// allocateIndices, setAllIndices, and expertStaticFillComplete
    /// all deallocate this array once they are done with it.
    ///
    /// This array <i>only</i> exists on a process before the graph's
    /// indices are allocated on that process.  After that point, it
    /// is discarded, since the graph's allocation implicitly or
    /// explicitly represents the same information.
    ///
    /// FIXME (mfh 07 Aug 2014) We want graph's constructors to
    /// allocate, rather than doing lazy allocation at first insert.
    /// This will make both k_numAllocPerRow_ and numAllocForAllRows_
    /// obsolete.
    Kokkos::DualView<const size_t*, Kokkos::LayoutLeft, execution_space> k_numAllocPerRow_;

    /// \brief The maximum number of entries to allow in each locally owned row.
    ///
    /// This is an argument to some of the graph's constructors.
    /// Either this or k_numAllocPerRow_ is used, but not both.
    ///
    /// FIXME (mfh 07 Aug 2014) We want graph's constructors to
    /// allocate, rather than doing lazy allocation at first insert.
    /// This will make both k_numAllocPerRow_ and numAllocForAllRows_
    /// obsolete.
    size_t numAllocForAllRows_;

    //! \name 1-D storage (StaticProfile) data structures
    //@{

    /// \brief Local column indices for all rows.
    ///
    /// This is only allocated if
    ///
    ///   - The calling process has a nonzero number of entries
    ///   - The graph has StaticProfile (1-D storage)
    ///   - The graph is locally indexed
    typename local_graph_type::entries_type::non_const_type k_lclInds1D_;

    //! Type of the k_gblInds1D_ array of global column indices.
    typedef Kokkos::View<GlobalOrdinal*, execution_space> t_GlobalOrdinal_1D;

    /// \brief Global column indices for all rows.
    ///
    /// This is only allocated if
    ///
    ///   - The calling process has a nonzero number of entries
    ///   - The graph has StaticProfile (1-D storage)
    ///   - The graph is globally indexed
    t_GlobalOrdinal_1D k_gblInds1D_;

    /// \brief Row offsets for "1-D" storage.
    ///
    /// This is only allocated if "1-D" (StaticProfile) storage is
    /// active.  In that case, if beg = k_rowPtrs_(i_lcl) and end =
    /// k_rowPtrs_(i_lcl+1) for local row index i_lcl, then
    ///
    ///   - if the graph is locally indexed, k_lclInds1D_(beg:end-1)
    ///     (inclusive range) is the space for any local column
    ///     indices in local row i_lcl, else
    ///   - if the graph is globally indexed, k_gblInds1D_(beg:end-1)
    ///     (inclusive range) is the space for any global column
    ///     indices in local row i_lcl.
    ///
    /// Only the first k_numRowEntries_(i_lcl) of these entries are
    /// actual valid column indices.  Any remaining entries are "extra
    /// space."  If the graph's storage is packed, then there is no
    /// extra space, and the k_numRowEntries_ array is invalid.
    ///
    /// Both the k_rowPtrs_ and k_numRowEntries_ arrays are not
    /// allocated if the graph has 2-D (DynamicProfile) storage.
    ///
    /// If it is allocated, k_rowPtrs_ has length getNodeNumRows()+1.
    /// The k_numRowEntries_ array has has length getNodeNumRows(),
    /// again if it is allocated.
    ///
    /// [we may delete this to save memory on fillComplete, if "Delete
    /// Row Pointers" is specified.]
    typename local_graph_type::row_map_type::const_type k_rowPtrs_;

    //@}
    /// \name 2-D storage (DynamicProfile) data structures
    ///
    /// 2-D storage exists only if the graph was allocated with
    /// DynamicProfile.  All of these data structures exist in host
    /// memory.  Currently, NONE of them are thread safe, let alone
    /// thread scalable.  These data structures only exist to support
    /// legacy use cases.  At some point, we may add a thread-scalable
    /// intermediate level of "dynamicity" between 2-D storage and 1-D
    /// storage (StaticProfile), which bounds the <i>total</i> number
    /// of entries allowed per process, but does <i>not</i> otherwise
    /// bound the number of entries per row.
    //@{

    /// \brief Local column indices for all rows.
    ///
    /// This is only allocated if
    ///
    ///   - The calling process has a nonzero number of entries
    ///   - The graph has DynamicProfile (2-D storage)
    ///   - The graph is locally indexed
    ///
    /// In that case, if i_lcl is the local index of a locally owned
    /// row, then lclInds2D_[i_lcl] stores the local column indices
    /// for that row.
    Teuchos::ArrayRCP<Teuchos::Array<LocalOrdinal> > lclInds2D_;

    /// \brief Global column indices for all rows.
    ///
    /// This is only allocated if
    ///
    ///   - The calling process has a nonzero number of entries
    ///   - The graph has DynamicProfile (2-D storage)
    ///   - The graph is globally indexed
    ///
    /// In that case, if i_gbl is the global index of a globally owned
    /// row, then gblInds2D_[i_gbl] stores the global column indices
    /// for that row.
    Teuchos::ArrayRCP<Teuchos::Array<GlobalOrdinal> > gblInds2D_;

    typedef Kokkos::DualView<size_t*, Kokkos::LayoutLeft, execution_space> t_numRowEntries_;

    /// \brief The number of local entries in each locally owned row.
    ///
    /// This is deallocated in fillComplete() if fillComplete()'s
    /// "Optimize Storage" parameter is set to \c true.
    ///
    /// This may also exist with 1-D storage, if storage is unpacked.
    t_numRowEntries_ k_numRowEntries_;

    //@}

    /// \brief Status of the graph's storage, when not in a
    ///   fill-complete state.
    ///
    /// The phrase "When not in a fill-complete state" is important.
    /// When the graph is fill complete, it <i>always</i> uses 1-D
    /// "packed" storage.  However, if the "Optimize Storage"
    /// parameter to fillComplete was false, the graph may keep
    /// unpacked 1-D or 2-D storage around and resume it on the next
    /// resumeFill call.
    Details::EStorageStatus storageStatus_;

    bool indicesAreAllocated_;
    bool indicesAreLocal_;
    bool indicesAreGlobal_;
    bool fillComplete_;

    //! Whether the graph is locally lower triangular.
    bool lowerTriangular_;
    //! Whether the graph is locally upper triangular.
    bool upperTriangular_;
    //! Whether the graph's indices are sorted in each row, on this process.
    bool indicesAreSorted_;
    /// \brief Whether the graph's indices are non-redundant (merged)
    ///   in each row, on this process.
    bool noRedundancies_;
    //! Whether this process has computed local constants.
    bool haveLocalConstants_;
    //! Whether all processes have computed global constants.
    bool haveGlobalConstants_;

    //! Nonlocal data given to insertGlobalValues or sumIntoGlobalValues.
    std::map<GlobalOrdinal, std::vector<GlobalOrdinal> > nonlocals_;

    /// \brief Whether to require makeColMap() (and therefore
    ///   fillComplete()) to order column Map GIDs associated with
    ///   each remote process in ascending order.
    ///
    /// makeColMap() always groups remote GIDs by process rank, so
    /// that all remote GIDs with the same owning rank occur
    /// contiguously.  By default, it always sorts remote GIDs in
    /// increasing order within those groups.  This behavior differs
    /// from Epetra, which does not sort remote GIDs with the same
    /// owning process.
    ///
    /// This is \c true by default, which means "sort remote GIDs."
    /// If you don't want to sort (for compatibility with Epetra),
    /// call sortGhostColumnGIDsWithinProcessBlock(false).
    bool sortGhostsAssociatedWithEachProcessor_;

  }; // class CrsGraph

  /// \brief Nonmember function to create an empty CrsGraph given a
  ///   row Map and the max number of entries allowed locally per row.
  ///
  /// \return A dynamically allocated (DynamicProfile) graph with
  ///   specified number of nonzeros per row (defaults to zero).
  /// \relatesalso CrsGraph
  template <class LocalOrdinal, class GlobalOrdinal, class Node, const bool classic = Node::classic>
  Teuchos::RCP<CrsGraph<LocalOrdinal, GlobalOrdinal, Node, classic> >
  createCrsGraph (const Teuchos::RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &map,
                  size_t maxNumEntriesPerRow = 0,
                  const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null)
  {
    using Teuchos::rcp;
    typedef CrsGraph<LocalOrdinal, GlobalOrdinal, Node, classic> graph_type;
    return rcp (new graph_type (map, maxNumEntriesPerRow, DynamicProfile, params));
  }

  namespace Details {

    template<class LocalOrdinal,
             class GlobalOrdinal,
             class OutputNodeType,
             class InputNodeType>
    class CrsGraphCopier<CrsGraph<LocalOrdinal, GlobalOrdinal, OutputNodeType>,
                         CrsGraph<LocalOrdinal, GlobalOrdinal, InputNodeType> > {
    public:
      typedef CrsGraph<LocalOrdinal, GlobalOrdinal, InputNodeType> input_crs_graph_type;
      typedef CrsGraph<LocalOrdinal, GlobalOrdinal, OutputNodeType> output_crs_graph_type;

      static Teuchos::RCP<output_crs_graph_type>
      clone (const input_crs_graph_type& graphIn,
             const Teuchos::RCP<OutputNodeType> &nodeOut,
             const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null)
      {
        using Teuchos::arcp;
        using Teuchos::ArrayRCP;
        using Teuchos::ArrayView;
        using Teuchos::null;
        using Teuchos::outArg;
        using Teuchos::ParameterList;
        using Teuchos::parameterList;
        using Teuchos::RCP;
        using Teuchos::rcp;
        using Teuchos::REDUCE_MIN;
        using Teuchos::reduceAll;
        using Teuchos::sublist;
        using std::cerr;
        using std::endl;
        typedef LocalOrdinal LO;
        typedef GlobalOrdinal GO;
        typedef typename ArrayView<const GO>::size_type size_type;
        typedef ::Tpetra::Map<LO, GO, InputNodeType> input_map_type;
        typedef ::Tpetra::Map<LO, GO, OutputNodeType> output_map_type;
        const char prefix[] = "Tpetra::Details::CrsGraphCopier::clone: ";

        // Set parameters' default values.
        bool debug = false;
        bool fillCompleteClone = true;
        bool useLocalIndices = graphIn.hasColMap ();
        ProfileType pftype = StaticProfile;
        // If the user provided a ParameterList, get values from there.
        if (! params.is_null ()) {
          fillCompleteClone = params->get ("fillComplete clone", fillCompleteClone);
          useLocalIndices = params->get ("Locally indexed clone", useLocalIndices);
          if (params->get ("Static profile clone", true) == false) {
            pftype = DynamicProfile;
          }
          debug = params->get ("Debug", debug);
        }

        const Teuchos::Comm<int>& comm = * (graphIn.getRowMap ()->getComm ());
        const int myRank = comm.getRank ();

        TEUCHOS_TEST_FOR_EXCEPTION(
                                   ! graphIn.hasColMap () && useLocalIndices, std::runtime_error,
                                   prefix << "You asked clone() to use local indices (by setting the "
                                   "\"Locally indexed clone\" parameter to true), but the source graph "
                                   "does not yet have a column Map, so this is impossible.");

        if (debug) {
          std::ostringstream os;
          os << "Process " << myRank << ": Cloning row Map" << endl;
          cerr << os.str ();
        }

        RCP<const output_map_type> clonedRowMap =
          graphIn.getRowMap ()->template clone<OutputNodeType> (nodeOut);

        // Invoke the output graph's constructor, using the input graph's
        // upper bounds on the number of entries in each local row.
        RCP<output_crs_graph_type> clonedGraph; // returned by this function
        {
          ArrayRCP<const size_t> numEntriesPerRow;
          size_t numEntriesForAll = 0;
          bool boundSameForAllLocalRows = true;

          if (debug) {
            std::ostringstream os;
            os << "Process " << myRank << ": Getting per-row bounds" << endl;
            cerr << os.str ();
          }
          graphIn.getNumEntriesPerLocalRowUpperBound (numEntriesPerRow,
                                                      numEntriesForAll,
                                                      boundSameForAllLocalRows);
          if (debug) {
            std::ostringstream os;
            os << "Process " << myRank << ": numEntriesForAll = "
               << numEntriesForAll << endl;
            cerr << os.str ();
          }

          if (debug) {
            std::ostringstream os;
            os << "Process " << myRank << ": graphIn.getNodeMaxNumRowEntries() = "
               << graphIn.getNodeMaxNumRowEntries () << endl;
            cerr << os.str ();
          }

          RCP<ParameterList> graphparams;
          if (params.is_null ()) {
            graphparams = parameterList ("CrsGraph");
          } else {
            graphparams = sublist (params, "CrsGraph");
          }
          if (useLocalIndices) {
            RCP<const output_map_type> clonedColMap =
              graphIn.getColMap ()->template clone<OutputNodeType> (nodeOut);
            if (boundSameForAllLocalRows) {
              clonedGraph = rcp (new output_crs_graph_type (clonedRowMap, clonedColMap,
                                                            numEntriesForAll, pftype,
                                                            graphparams));
            } else {
              clonedGraph = rcp (new output_crs_graph_type (clonedRowMap, clonedColMap,
                                                            numEntriesPerRow, pftype,
                                                            graphparams));
            }
          } else {
            if (boundSameForAllLocalRows) {
              clonedGraph = rcp (new output_crs_graph_type (clonedRowMap,
                                                            numEntriesForAll, pftype,
                                                            graphparams));
            } else {
              clonedGraph = rcp (new output_crs_graph_type (clonedRowMap,
                                                            numEntriesPerRow,
                                                            pftype, graphparams));
            }
          }

          if (debug) {
            std::ostringstream os;
            os << "Process " << myRank << ": Invoked output graph's constructor" << endl;
            cerr << os.str ();
          }

          // done with these
          numEntriesPerRow = null;
          numEntriesForAll = 0;
        }

        const input_map_type& inputRowMap = * (graphIn.getRowMap ());
        const size_type numRows =
          static_cast<size_type> (inputRowMap.getNodeNumElements ());

        bool failed = false;

        if (useLocalIndices) {
          const LO localMinLID = inputRowMap.getMinLocalIndex ();
          const LO localMaxLID = inputRowMap.getMaxLocalIndex ();

          if (graphIn.isLocallyIndexed ()) {
            if (numRows != 0) {
              try {
                ArrayView<const LO> linds;
                for (LO lrow = localMinLID; lrow <= localMaxLID; ++lrow) {
                  graphIn.getLocalRowView (lrow, linds);
                  if (linds.size () != 0) {
                    clonedGraph->insertLocalIndices (lrow, linds);
                  }
                }
              }
              catch (std::exception& e) {
                std::ostringstream os;
                os << "Process " << myRank << ": copying (reading local by view, "
                  "writing local) indices into the output graph threw an "
                  "exception: " << e.what () << endl;
                cerr << os.str ();
                failed = true;
              }
            }
          }
          else { // graphIn.isGloballyIndexed()
            TEUCHOS_TEST_FOR_EXCEPTION(
                                       ! graphIn.hasColMap () && useLocalIndices, std::invalid_argument,
                                       prefix << "You asked clone() to use local indices (by setting the "
                                       "\"Locally indexed clone\" parameter to true), but the source graph "
                                       "does not yet have a column Map, so this is impossible.");

            // The input graph has a column Map, but is globally indexed.
            // That's a bit weird, but we'll run with it.  In this case,
            // getLocalRowView won't work, but getLocalRowCopy should
            // still work; it will just have to convert from global to
            // local indices internally.

            try {
              // Make space for getLocalRowCopy to put column indices.
              //
              // This is only a hint; we may have to resize in the loop
              // below.  getNodeMaxNumRowEntries() may return nonsense if
              // fill is active.  The key bool in CrsGraph is
              // haveLocalConstants_.
              size_t myMaxNumRowEntries =
                graphIn.isFillActive () ? static_cast<size_t> (0) :
                graphIn.getNodeMaxNumRowEntries ();

              Array<LO> linds (myMaxNumRowEntries);

              // Copy each row into the new graph, using local indices.
              for (LO lrow = localMinLID; lrow <= localMaxLID; ++lrow) {
                size_t theNumEntries = graphIn.getNumEntriesInLocalRow (lrow);
                if (theNumEntries > myMaxNumRowEntries) {
                  myMaxNumRowEntries = theNumEntries;
                  linds.resize (myMaxNumRowEntries);
                }
                graphIn.getLocalRowCopy (lrow, linds (), theNumEntries);
                if (theNumEntries != 0) {
                  clonedGraph->insertLocalIndices (lrow, linds (0, theNumEntries));
                }
              }
            }
            catch (std::exception& e) {
              std::ostringstream os;
              os << "Process " << myRank << ": copying (reading local by copy, "
                "writing local) indices into the output graph threw an exception: "
                 << e.what () << endl;
              cerr << os.str ();
              failed = true;
            }
          }
        }
        else { /* useGlobalIndices */
          if (numRows != 0) {
            const GlobalOrdinal localMinGID = inputRowMap.getMinGlobalIndex ();
            const GlobalOrdinal localMaxGID = inputRowMap.getMaxGlobalIndex ();
            const bool inputRowMapIsContiguous = inputRowMap.isContiguous ();

            if (graphIn.isGloballyIndexed ()) {
              ArrayView<const GlobalOrdinal> ginds;

              if (inputRowMapIsContiguous) {
                try {
                  for (GO grow = localMinGID; grow <= localMaxGID; ++grow) {
                    graphIn.getGlobalRowView (grow, ginds);
                    if (ginds.size () != 0) {
                      clonedGraph->insertGlobalIndices (grow, ginds);
                    }
                  }
                }
                catch (std::exception& e) {
                  std::ostringstream os;
                  os << "Process " << myRank << ": copying (reading global by view, "
                    "writing global) indices into the output graph threw an "
                    "exception: " << e.what () << endl;
                  cerr << os.str ();
                  failed = true;
                }
              }
              else { // input row Map is not contiguous
                try {
                  ArrayView<const GO> inputRowMapGIDs = inputRowMap.getNodeElementList ();
                  for (size_type k = 0; k < numRows; ++k) {
                    const GO grow = inputRowMapGIDs[k];
                    graphIn.getGlobalRowView (grow, ginds);
                    if (ginds.size () != 0) {
                      clonedGraph->insertGlobalIndices (grow, ginds);
                    }
                  }
                }
                catch (std::exception& e) {
                  std::ostringstream os;
                  os << "Process " << myRank << ": copying (reading global by view, "
                    "writing global) indices into the output graph threw an "
                    "exception: " << e.what () << endl;
                  cerr << os.str ();
                  failed = true;
                }
              }
            }
            else { // graphIn.isLocallyIndexed()
              // Make space for getGlobalRowCopy to put column indices.
              //
              // This is only a hint; we may have to resize in the loop
              // below.  getNodeMaxNumRowEntries() may return nonsense if
              // fill is active.  The key bool in CrsGraph is
              // haveLocalConstants_.
              size_t myMaxNumRowEntries =
                graphIn.isFillActive () ? static_cast<size_t> (0) :
                graphIn.getNodeMaxNumRowEntries ();

              Array<GO> ginds (myMaxNumRowEntries);

              if (inputRowMapIsContiguous) {
                try {
                  for (GO grow = localMinGID; grow <= localMaxGID; ++grow) {
                    size_t theNumEntries = graphIn.getNumEntriesInGlobalRow (grow);
                    if (theNumEntries > myMaxNumRowEntries) {
                      myMaxNumRowEntries = theNumEntries;
                      ginds.resize (myMaxNumRowEntries);
                    }
                    graphIn.getGlobalRowCopy (grow, ginds (), theNumEntries);
                    if (theNumEntries != 0) {
                      clonedGraph->insertGlobalIndices (grow, ginds (0, theNumEntries));
                    }
                  }
                }
                catch (std::exception& e) {
                  std::ostringstream os;
                  os << "Process " << myRank << ": copying (reading global by copy, "
                    "writing global) indices into the output graph threw an "
                    "exception: " << e.what () << endl;
                  cerr << os.str ();
                  failed = true;
                }
              }
              else { // input row Map is not contiguous
                try {
                  ArrayView<const GO> inputRowMapGIDs = inputRowMap.getNodeElementList ();
                  for (size_type k = 0; k < numRows; ++k) {
                    const GO grow = inputRowMapGIDs[k];

                    size_t theNumEntries = graphIn.getNumEntriesInGlobalRow (grow);
                    if (theNumEntries > myMaxNumRowEntries) {
                      myMaxNumRowEntries = theNumEntries;
                      ginds.resize (myMaxNumRowEntries);
                    }
                    graphIn.getGlobalRowCopy (grow, ginds (), theNumEntries);
                    if (theNumEntries != 0) {
                      clonedGraph->insertGlobalIndices (grow, ginds (0, theNumEntries));
                    }
                  }
                }
                catch (std::exception& e) {
                  std::ostringstream os;
                  os << "Process " << myRank << ": copying (reading global by copy, "
                    "writing global) indices into the output graph threw an "
                    "exception: " << e.what () << endl;
                  cerr << os.str ();
                  failed = true;
                }
              }
            }
          } // numRows != 0
        }

        if (debug) {
          std::ostringstream os;
          os << "Process " << myRank << ": copied entries" << endl;
          cerr << os.str ();
        }

        if (fillCompleteClone) {
          RCP<ParameterList> fillparams = params.is_null () ?
            parameterList ("fillComplete") :
            sublist (params, "fillComplete");
          try {
            RCP<const output_map_type> clonedRangeMap;
            RCP<const output_map_type> clonedDomainMap;
            if (! graphIn.getRangeMap ().is_null () &&
                graphIn.getRangeMap () != graphIn.getRowMap ()) {
              clonedRangeMap =
                graphIn.getRangeMap ()->template clone<OutputNodeType> (nodeOut);
            }
            else {
              clonedRangeMap = clonedRowMap;
            }
            if (! graphIn.getDomainMap ().is_null ()
                && graphIn.getDomainMap () != graphIn.getRowMap ()) {
              clonedDomainMap =
                graphIn.getDomainMap ()->template clone<OutputNodeType> (nodeOut);
            }
            else {
              clonedDomainMap = clonedRowMap;
            }

            if (debug) {
              std::ostringstream os;
              os << "Process " << myRank << ": About to call fillComplete on "
                "cloned graph" << endl;
              cerr << os.str ();
            }
            clonedGraph->fillComplete (clonedDomainMap, clonedRangeMap, fillparams);
          }
          catch (std::exception &e) {
            failed = true;
            std::ostringstream os;
            os << prefix << "Process " << myRank << ": Caught the following "
              "exception while calling fillComplete() on clone of type"
               << endl << Teuchos::typeName (*clonedGraph) << endl;
            cerr << os.str ();
          }
        }

        int lclSuccess = failed ? 0 : 1;
        int gblSuccess = 1;
        reduceAll<int, int> (comm, REDUCE_MIN, lclSuccess, outArg (gblSuccess));
        TEUCHOS_TEST_FOR_EXCEPTION(
                                   gblSuccess != 1, std::logic_error, prefix <<
                                   "Clone failed on at least one process.");

        if (debug) {
          std::ostringstream os;
          os << "Process " << myRank << ": Done with CrsGraph::clone" << endl;
          cerr << os.str ();
        }
        return clonedGraph;
      }
    };

  } // namespace Details
} // namespace Tpetra

#endif // TPETRA_CRSGRAPH_DECL_HPP
