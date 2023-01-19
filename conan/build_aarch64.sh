#!/bin/bash
cd build
echo "aarch64 conan build"
rm -rf *
conan install .. -pr:b=default  -pr:h=aarch64 --build=missing
cd ..