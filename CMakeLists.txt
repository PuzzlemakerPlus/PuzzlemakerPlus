cmake_minimum_required(VERSION 3.27)
project(PuzzlemakerPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Quick
        REQUIRED)

add_executable(PuzzlemakerPlus main.cpp)
target_link_libraries(PuzzlemakerPlus
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
)

