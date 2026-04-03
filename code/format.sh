clang-format -i -style=file src/*.cpp src/*.hpp app/*.cpp app/*.hpp
clang-tidy src/*.cpp app/*.cpp -fix -header-filter=.*
