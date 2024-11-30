cmake_minimum_required(VERSION 3.22)

project(audasm)

add_executable(aus
    "src/main.cpp"
    "src/context.cpp"
    "src/parsing_utils.cpp"
    "src/registers.cpp"
    "src/formats/prefix.cpp"
    "src/formats/zo.cpp"
)

target_include_directories(aus PRIVATE "include/")
target_compile_features(aus PRIVATE cxx_std_23)