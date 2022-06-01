ThreadPermuter
==============

Run two or more threads in "every" possible permutation of
execution order (as if everything is fully sequentially consistent:
the threads are zipped together and only run one at a time).

In order to use this, add one of two macro's to the code
under test:

- TPY : Yield the thread: allow another thread to be run, or run the same thread again.
- TPB : Blocking thread: force running of another thread first.

For a usage example see [permute_test.cxx](https://github.com/CarloWood/threadpermuter/blob/master/permute_test.cxx).

To build that test program, run,

    git clone --recursive https://github.com/CarloWood/threadpermuter.git
    cd threadpermuter
    ./autogen.sh
    # This must be your gitache cache directory (or some existing (empty) writable directory).
    export GITACHE_ROOT=/opt/gitache
    mkdir build
    cmake -S . -B build -DCMAKE_MESSAGE_LOG_LEVEL=STATUS -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON -DEnableDebugGlobal:BOOL=OFF -DCMAKE_MESSAGE_LOG_LEVEL=DEBUG
    make -C build
    build/permute_test

If for some reason you can't set GITACHE_ROOT then alternatively you
may download libcwd and configure, compile and install that in a path
that cmake will find it in. See cmake/gitache-configs/libcwd_r.cmake
for the configure options to use for libcwd.
