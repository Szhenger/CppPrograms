cmake_minimum_required(VERSION 3.20)
project(BertAttention)

set(CMAKE_CXX_STANDARD 23)

find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(bert_attention bert_attention.cpp)
target_link_libraries(bert_attention "${TORCH_LIBRARIES}" ${OpenCV_LIBS})
