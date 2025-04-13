cmake_minimum_required(VERSION 3.27)
project(PuzzlemakerPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Gui Quick REQUIRED)

qt_standard_project_setup()

qt_add_executable(PuzzlemakerPlus src/main.cpp
        src/palette/palette.cpp
        src/palette/palette.h)

qt_add_qml_module(PuzzlemakerPlus
        URI puzzlemakerplus
        QML_FILES
        qml/HelloWorld.qml
)

target_link_libraries(PuzzlemakerPlus PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
)

