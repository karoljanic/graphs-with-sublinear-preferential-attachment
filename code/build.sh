mkdir -p build && cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release

cmake --build . -- -j1
