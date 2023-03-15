#!/bin/bash

CURRENT_DIR=

# 1. remove build directory
rm -rf build/

# 2. create build directory
mkdir build/

# 3. enter build directory
pushd build/

# 4. cmake and make
cmake ../src
#make VERBOSE=1
make 


# exit build/ direcoty
popd
