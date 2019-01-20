include $(srcdir)/cwm4/root_makefile_top.am

SUBDIRS = @CW_SUBDIRS@

AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = bstest permute_test

bstest_SOURCES = bstest.cxx
bstest_CXXFLAGS = @LIBCWD_R_FLAGS@
bstest_LDFLAGS = $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

permute_test_SOURCES = permute_test.cxx ThreadPermuter.cxx Permutation.cxx Thread.cxx
permute_test_CXXFLAGS = @LIBCWD_R_FLAGS@
permute_test_LDFLAGS = $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

include $(srcdir)/cwm4/root_makefile_bottom.am
