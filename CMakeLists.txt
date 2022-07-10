project(GCHello)
set(DNNL_GRAPH_LIBRARY_TYPE STATIC)
set(DNNL_GRAPH_BUILD_COMPILER_BACKEND ON)
add_subdirectory(oneDNN)
include_directories(${PROJECT_SOURCE_DIR}/oneDNN/src/backend/graph_compiler/core/src)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

add_executable(hello Ch3-HelloCompiler/hello.cpp)
target_link_libraries(hello graphcompiler dnnl)

add_executable(tensorir Ch5-TensorIR/tensorir.cpp)
target_link_libraries(tensorir graphcompiler)

add_executable(graphir Ch7-GraphIR/graphir.cpp)
target_link_libraries(graphir graphcompiler dnnl)

add_executable(graphlower Ch8-GraphLower/graphlower.cpp)
target_link_libraries(graphlower graphcompiler dnnl)

add_executable(irvisitor Ch12-IRVisitor/irvisitor.cpp)
target_link_libraries(irvisitor graphcompiler dnnl)