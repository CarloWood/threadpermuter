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

To build that test program,
first [get libcwd](https://github.com/CarloWood/libcwd) and configure, compile and install that.
Then run,

    git clone --recursive https://github.com/CarloWood/threadpermuter.git
    cd threadpermuter
    ./autogen.sh
    ./configure --enable-maintainer-mode
    make
    ./permute_test
