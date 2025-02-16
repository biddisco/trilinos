/* src/Stokhos_config.h.in.  Generated from configure.ac by autoheader.  */

/* define if we want to use MPI */
#define HAVE_MPI

/* Define if want to build stokhos-examples */
/* #undef HAVE_STOKHOS_EXAMPLES */

/* Define if want to build stokhos-tests */
/* #undef HAVE_STOKHOS_TESTS */

/* Define if explicit instantiation is turned on */
/* #undef HAVE_STOKHOS_EXPLICIT_INSTANTIATION */

/* Define to enable extra debugging checks */
/* #undef STOKHOS_DEBUG */

/* Define if Sacado is enabled */
#define HAVE_STOKHOS_SACADO

/* Define if Dakota is enabled */
/* #undef HAVE_STOKHOS_DAKOTA */

/* Define if Fortran UQTK is enabled */
/* #undef HAVE_STOKHOS_FORUQTK */

/* Define if EpetraExt is enabled */
#define HAVE_STOKHOS_EPETRAEXT

/* Define if Ifpack is enabled */
/* #undef HAVE_STOKHOS_IFPACK */

/* Define if ML is enabled */
/* #undef HAVE_STOKHOS_ML */

/* Define if MueLu is enabled */
#define HAVE_STOKHOS_MUELU

/* Define if Anasazi is enabled */
/* #undef HAVE_STOKHOS_ANASAZI */

/* Define if NOX is enabled */
/* #undef HAVE_STOKHOS_NOX */

/* Define if Isorropia is enabled */
/* #undef HAVE_STOKHOS_ISORROPIA */

/* Define if Teuchos timers are enabled */
#define STOKHOS_TEUCHOS_TIME_MONITOR

/* Define if CUDA is enabled */
/* #undef HAVE_STOKHOS_CUDA */

/* Define if Thrust is enabled */
/* #undef HAVE_STOKHOS_THRUST */

/* Define if Thrust is enabled */
/* #undef HAVE_STOKHOS_CUSPARSE */

/* Define if Clp is enabled */
/* #undef HAVE_STOKHOS_CLP */

/* Define if GLPK is enabled */
/* #undef HAVE_STOKHOS_GLPK */

/* Define if qpOASES is enabled */
/* #undef HAVE_STOKHOS_QPOASES */

/* Define if Boost is enabled */
/* #undef HAVE_STOKHOS_BOOST */

/* Define if TpetraKernels is enabled */
#define HAVE_STOKHOS_TPETRAKERNELS
#ifdef HAVE_STOKHOS_TPETRAKERNELS
// Define old macro for backwards compatibility.
#  define HAVE_STOKHOS_KOKKOSLINALG
#endif // HAVE_STOKHOS_TPETRAKERNELS

/* Define if KokkosMpiCom is enabled */
#define HAVE_STOKHOS_TEUCHOSKOKKOSCOMM
#ifdef HAVE_STOKHOS_TEUCHOSKOKKOSCOMM
// For backwards compatibility
#  define HAVE_STOKHOS_KOKKOSMPICOMM
#endif // HAVE_STOKHOS_TEUCHOSKOKKOSCOMM

/* Define if KokkosAlgorithms is enabled */
#define HAVE_STOKHOS_KOKKOSALGORITHMS

/* Define if MueLu is enabled */
#define HAVE_STOKHOS_MUELU

/* Define if Ifpack2 is enabled */
/* #undef HAVE_STOKHOS_IFPACK2 */

/* Define if Belos is enabled */
/* #undef HAVE_STOKHOS_BELOS */

/* Define if Amesos2 is enabled */
#define HAVE_STOKHOS_AMESOS2

/* Define if MATLAB is enabled */
/* #undef HAVE_STOKHOS_MATLABLIB */

/* Define if MKL is enabled */
/* #undef HAVE_STOKHOS_MKL */

/* Define if OpenMP is enabled */
/* #undef HAVE_STOKHOS_OPENMP */

/* Define if intrinsics are enabled */
#define HAVE_STOKHOS_INTRINSICS

/* Define if C++11 is enabled */
#define HAVE_STOKHOS_CXX11

/* Define if Thyra is enabled */
#define HAVE_STOKHOS_THYRA
