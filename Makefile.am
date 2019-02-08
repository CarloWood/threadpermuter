include $(srcdir)/cwm4/root_makefile_top.am

SUBDIRS = @CW_SUBDIRS@

AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

lib_LTLIBRARIES = libthreadpermuter.la
noinst_PROGRAMS = bstest permute_test FuzzyLock_test StateChanger_test

libthreadpermuter_la_SOURCES = ThreadPermuter.cxx Permutation.cxx Thread.cxx
libthreadpermuter_la_CXXFLAGS = @LIBCWD_R_FLAGS@
include_HEADERS = Permutation.h Thread.h ThreadPermuter.h

bstest_SOURCES = bstest.cxx
bstest_CXXFLAGS = @LIBCWD_R_FLAGS@
bstest_LDADD = libthreadpermuter.la $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

permute_test_SOURCES = permute_test.cxx ThreadPermuter.cxx Permutation.cxx Thread.cxx
permute_test_CXXFLAGS = @LIBCWD_R_FLAGS@
permute_test_LDADD = libthreadpermuter.la $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

FuzzyLock_test_SOURCES = FuzzyLock_test.cxx
FuzzyLock_test_CXXFLAGS = @LIBCWD_R_FLAGS@
FuzzyLock_test_LDADD = $(top_builddir)/libthreadpermuter.la $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

StateChanger_test_SOURCES = StateChanger_test.cxx
StateChanger_test_CXXFLAGS = @LIBCWD_R_FLAGS@
StateChanger_test_LDADD = $(top_builddir)/libthreadpermuter.la $(top_builddir)/utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

include $(srcdir)/cwm4/root_makefile_bottom.am
