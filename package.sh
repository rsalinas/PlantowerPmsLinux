#! /bin/bash -eux


rm -rf installed*
cmake -DCOMPONENT=Production  -DCMAKE_INSTALL_PREFIX:PATH=installed/Production -P cmake_install.cmake 
cpack 

dpkg -c *.deb
