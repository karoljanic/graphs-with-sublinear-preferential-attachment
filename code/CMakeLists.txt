cmake_minimum_required(VERSION 3.5)

project(PreferentialColouringOfRandomGraphs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(FetchContent)

FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(OGDF
        GIT_REPOSITORY https://github.com/ogdf/ogdf.git)
FetchContent_MakeAvailable(OGDF)

FetchContent_Declare(Peregrine
        GIT_REPOSITORY https://github.com/pdclab/peregrine.git)
FetchContent_MakeAvailable(Peregrine)

add_executable(generate_pa app/generate_pa.cpp)
set_target_properties(generate_pa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(generate_pa PROPERTIES OUTPUT_NAME "generate_pa")
target_link_libraries(generate_pa PRIVATE nlohmann_json::nlohmann_json)

add_executable(generate_and_track_pa app/generate_and_track_pa.cpp)
set_target_properties(generate_and_track_pa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(generate_and_track_pa PROPERTIES OUTPUT_NAME "generate_and_track_pa")
target_link_libraries(generate_and_track_pa PRIVATE nlohmann_json::nlohmann_json)

add_executable(test_pa_weight_sums app/test_pa_weight_sums.cpp)
set_target_properties(test_pa_weight_sums PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(test_pa_weight_sums PROPERTIES OUTPUT_NAME "test_pa_weight_sums")
target_link_libraries(test_pa_weight_sums PRIVATE nlohmann_json::nlohmann_json)

add_executable(test_pa_degrees app/test_pa_degrees.cpp)
set_target_properties(test_pa_degrees PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(test_pa_degrees PROPERTIES OUTPUT_NAME "test_pa_degrees")
target_link_libraries(test_pa_degrees PRIVATE nlohmann_json::nlohmann_json)

add_executable(generate_planar_pa app/generate_planar_pa.cpp)
set_target_properties(generate_planar_pa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(generate_planar_pa PROPERTIES OUTPUT_NAME "generate_planar_pa")
target_link_libraries(generate_planar_pa PRIVATE nlohmann_json::nlohmann_json OGDF)

add_executable(select_planar_subgraph app/select_planar_subgraph.cpp)
set_target_properties(select_planar_subgraph PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(select_planar_subgraph PROPERTIES OUTPUT_NAME "select_planar_subgraph")
target_link_libraries(select_planar_subgraph PRIVATE nlohmann_json::nlohmann_json OGDF)

add_executable(test_planarity_restoring app/test_planarity_restoring.cpp)
set_target_properties(test_planarity_restoring PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(test_planarity_restoring PROPERTIES OUTPUT_NAME "test_planarity_restoring")
target_link_libraries(test_planarity_restoring PRIVATE nlohmann_json::nlohmann_json OGDF)