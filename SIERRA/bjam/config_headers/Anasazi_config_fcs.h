/* src/Anasazi_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if want to build with amesos enabled */
#define HAVE_ANASAZI_AMESOS

/* Define if want to build with aztecoo enabled */
#define HAVE_ANASAZI_AZTECOO

/* Define if want to build with ifpack enabled */
#define HAVE_ANASAZI_IFPACK

/* Define if want to build with belos enabled */
#define HAVE_ANASAZI_BELOS

/* Define if want to build with epetra enabled */
#define HAVE_ANASAZI_EPETRA

/* Define if want to build with epetraext enabled */
#define HAVE_ANASAZI_EPETRAEXT

/* Define if want to build anasazi-experimental */
/* #undef HAVE_ANASAZI_EXPERIMENTAL */

/* Define if want to build with RBGen enabled */
/* #undef HAVE_ANASAZI_RBGEN */

/* Define if want to build with tpetra enabled */
#define HAVE_ANASAZI_TPETRA

/* Define whether we have Tpetra MultiVector timers */
/* #undef HAVE_ANASAZI_TPETRA_TIMERS */

/* Define if want to build with thyra enabled */
#define HAVE_ANASAZI_THYRA

/* Define if want to build with triutils enabled */
#define HAVE_ANASAZI_TRIUTILS

/* Define if want to build with epetra enabled */
#define HAVE_EPETRA_THYRA 1

/* Define if want to build anasazi-examples */
/* #undef HAVE_ANASAZI_EXAMPLES */

/* Define if want to build examples */
/* #undef HAVE_EXAMPLES */

/* Define if want to build anasazi-tests */
/* #undef HAVE_ANASAZI_TESTS */

/* Define if want to build tests */
/* #undef HAVE_TESTS */

/* define if we want to use MPI */
#define HAVE_MPI

/* Define if want to build teuchos-complex */
#define HAVE_TEUCHOS_COMPLEX

/* Define if building with TSQR enabled.  The TSQR implementation
   lives in the TSQR subpackage of Kokkos, and the Epetra and Tpetra
   adapters both live in Tpetra. */
/* #undef HAVE_ANASAZI_TSQR */

#define ANASAZI_TEUCHOS_TIME_MONITOR

/* Add macros for declaring functions deprecated */
#define ANASAZI_DEPRECATED
#define ANASAZI_DEPRECATED_MSG(MSG)


/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
