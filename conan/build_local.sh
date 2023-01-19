#!/bin/bash
cd build
echo "local conan build"
rm -rf *
conan install .. --build=missing
cd ..