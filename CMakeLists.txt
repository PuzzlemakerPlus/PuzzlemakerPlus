cmake_minimum_required(VERSION 3.27)
project(PuzzlemakerPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets REQUIRED)

qt_standard_project_setup()

qt_add_executable(PuzzlemakerPlus
        src/main.cpp
        src/widgets/palette.cpp
        src/widgets/palette.h
        src/widgets/viewport.cpp
        src/widgets/viewport.h
        src/widgets/PaletteElement.cpp
)

target_link_libraries(PuzzlemakerPlus PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)
