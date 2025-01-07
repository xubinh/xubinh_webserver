#!/usr/bin/env bash

BUILD_DIR=build
CMAKE_BUILD_TYPE="Release" # Debug / Release / RelWithDebInfo / MinSizeRel
NEED_TEST=off
USE_BLOCKING_QUEUE_WITH_RAW_POINTER=off
USE_LOCK_FREE_QUEUE=on
USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER=on

# Exit immediately if any command fails
set -e

if ! [ -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."

    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

echo "Configuring project with CMake..."
cmake \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DENABLE_TESTING="$NEED_TEST" \
    -DUSE_BLOCKING_QUEUE_WITH_RAW_POINTER="$USE_BLOCKING_QUEUE_WITH_RAW_POINTER" \
    -DUSE_LOCK_FREE_QUEUE="$USE_LOCK_FREE_QUEUE" \
    -DUSE_LOCK_FREE_QUEUE_WITH_RAW_POINTER="$USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER" \
    ..

echo "Building project..."

make -j"$(nproc)"

if [ "$NEED_TEST" = on ]; then
    echo "Running tests..."

    make test

    echo "All tests are passed. ✔️"

else
    echo "Building completed. ✔️"

fi
