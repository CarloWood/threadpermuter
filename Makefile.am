include $(srcdir)/cwm4/root_makefile_top.am

SUBDIRS = @CW_SUBDIRS@

bin_PROGRAMS = permute_test

permute_test_SOURCES = permute_test.cxx
permute_test_CXXFLAGS = @LIBCWD_R_FLAGS@
permute_test_CXXFLAGS = $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

include $(srcdir)/cwm4/root_makefile_bottom.am
