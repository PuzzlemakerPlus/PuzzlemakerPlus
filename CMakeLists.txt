cmake_minimum_required(VERSION 3.27)
project(PuzzlemakerPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt Resources support
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets REQUIRED)

qt_standard_project_setup()

qt_add_executable(PuzzlemakerPlus
        # Entry point
        src/main.cpp

        # Widgets
        src/widgets/palette.cpp
        src/widgets/palette.h
        src/widgets/viewport.cpp
        src/widgets/viewport.h
        src/widgets/PaletteElement.cpp

        # Utils
        src/util/optional_array.h

        # This is a test chamber. Four walls, ceiling and a floor
        # ✅ Good enough for science! ❎ Not Aperture Science!
        src/test_chamber.cpp
        src/test_chamber.h

        # Resources
        res/resources.qrc
)

target_link_libraries(PuzzlemakerPlus PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)
