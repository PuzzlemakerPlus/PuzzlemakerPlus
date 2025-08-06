cmake_minimum_required(VERSION 3.27)
include(FetchContent)
project(PuzzlemakerPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt Resources support
set(CMAKE_AUTORCC ON)

enable_testing()


find_package(Qt6 COMPONENTS 
            Core 
            Gui 
            Widgets 
            OpenGLWidgets 
            REQUIRED)

# FetchContent_Declare(
#     assimp
#     GIT_REPOSITORY https://github.com/assimp/assimp.git
#     GIT_TAG        master
# )

# Set before FetchContent_MakeAvailable if you want shared/static toggle
#set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Pull in Assimp
#FetchContent_MakeAvailable(assimp)


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
        src/util/3d/mesh.cpp
        src/util/3d/mesh.h
        src/util/3d/ray.cpp
        src/util/3d/ray.h

        # This is a test chamber. Four walls, ceiling and a floor
        # ✅ Good enough for science! ❎ Not Aperture Science!
        src/test_chamber_types.h
        src/test_chamber.cpp
        src/test_chamber.h

        src/selection.h
        src/selection.cpp

        src/undo.h
        src/undo.cpp

        # Resources
        res/resources.qrc
)
set_target_properties(PuzzlemakerPlus
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_target_properties(PuzzlemakerPlus
    PROPERTIES AUTOGEN_BUILD_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/autogen/PuzzlemakerPlus")

target_link_libraries(PuzzlemakerPlus PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
        # assimp::assimp
)

add_executable(octtreetest ${CMAKE_CURRENT_SOURCE_DIR}/tests/octtree_test.cpp
    src/test_chamber_types.h
    src/test_chamber.cpp
    src/test_chamber.h
    src/selection.cpp
    src/selection.h
    src/util/3d/mesh.cpp
    src/util/3d/mesh.h
    src/util/3d/ray.cpp
    src/util/3d/ray.h
    )
set_target_properties(octtreetest
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_target_properties(octtreetest
    PROPERTIES AUTOGEN_BUILD_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/autogen/octtreetest")
add_test(NAME OctTreeTest COMMAND octtreetest)
target_link_libraries(octtreetest PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)

qt_add_executable(importmodeltest ${CMAKE_CURRENT_SOURCE_DIR}/tests/import_model.cpp
    src/widgets/viewport.h
    src/widgets/viewport.cpp
    
    src/util/3d/mesh.cpp
    src/util/3d/mesh.h
    src/util/3d/ray.cpp
    src/util/3d/ray.h

    src/test_chamber_types.h
    src/test_chamber.cpp
    src/test_chamber.h
    src/selection.h
    src/selection.cpp
    src/undo.h
    src/undo.cpp
    )
set_target_properties(importmodeltest
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_target_properties(importmodeltest
    PROPERTIES AUTOGEN_BUILD_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/autogen/importmodeltest")
add_test(NAME ImportModelTest COMMAND importmodeltest)
target_link_libraries(importmodeltest PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)