#!/bin/bash
cd build
echo "local conan build"
rm -rf *
conan remove --locks
conan install .. --build=missing
cd ..