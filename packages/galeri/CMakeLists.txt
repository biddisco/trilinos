

#
# A) Define the package
#

TRIBITS_PACKAGE(Galeri)

#
# B) Set up package-specific options
#

# ...

#
# C) Add the libraries, tests, and examples
#

ADD_SUBDIRECTORY(src)

#this dir only had "EXTRA_DIST" files in it
#ADD_SUBDIRECTORY(src-fem)  

ADD_SUBDIRECTORY(src-xpetra)


TRIBITS_ADD_TEST_DIRECTORIES(test)

TRIBITS_ADD_EXAMPLE_DIRECTORIES(example example-fem example-xpetra)

#
# Exclude files for source package.
#

TRIBITS_EXCLUDE_AUTOTOOLS_FILES()

TRIBITS_EXCLUDE_FILES(
  src-pfem   
  example-pfem
  doc/AdvDiffSquare.png
  doc/L.*.png
  example-fem/TwoSquares.cpp
  src-fem/Galeri_FileGrid.h
  )

#
# D) Do standard postprocessing
#

TRIBITS_PACKAGE_POSTPROCESS()
