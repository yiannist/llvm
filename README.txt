Low Level Virtual Machine (LLVM)
================================

This directory and its subdirectories contain source code for the Low Level
Virtual Machine, a toolkit for the construction of highly optimized compilers,
optimizers, and runtime environments.

LLVM is open source software. You may freely distribute it under the terms of
the license agreement found in [LICENSE.txt] [1].

Please see the HTML documentation provided in [docs/index.html] [2] for further
assistance with LLVM.

If you're writing a package for LLVM, see [docs/Packaging.html] [3] for our
suggestions.


Why forked?
===========

We created this project at Github in order to work on implementing an
ABI-compliant back-end for Erlang/OTP. More information on the need
and design decisions of the new back-end to be announced.

Also see: [custom Erlang/OTP] [4]


Installing custom LLVM
======================

*  Getting latest source code from [Github] [5]:

        git clone git://github.com/yiannist/llvm.git llvm

*  Compiling:

        cd llvm/
        ./configure --enable-optimized --disable-assertions
        make -j N (where *N* is the number of CPU cores you want to use for the compilation process)

*  Installing:

        sudo make install

*  Verifying that the installation was successful. Compile [hello_1.ll] [6] and compare its output (hello_1.s) to [hello_1_correct.s] [7]:

        yiannis@mosby [~/git/llvm]>>= llc -load=ErlangGC.so hello_1.ll
        yiannis@mosby [~/git/llvm]>>= diff hello_1.s hello_1_correct.s
        yiannis@mosby [~/git/llvm]>>=

* You may now procceed installing [Erlang/OTP] [4] with the new LLVM back-end.


[1]: http://github.com/yiannist/llvm/blob/master/LICENSE.TXT
[2]: http://github.com/yiannist/llvm/blob/master/docs/index.html
[3]: http://github.com/yiannist/llvm/blob/master/docs/Packaging.html
[4]: http://github.com/yiannist/otp
[5]: http://github.com/yiannist/llvm
[6]: http://erllvm.softlab.ntua.gr/files/hello_1.ll
[7]: http://erllvm.softlab.ntua.gr/files/hello_1_correct.s
