cmake_minimum_required(VERSION 3.27)
include(FetchContent)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(QtSetup)
project(PuzzlemakerPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt Resources support
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOGEN_SOURCE_GROUP "")

enable_testing()

find_package(Qt6 COMPONENTS 
			Core 
			Gui 
			Widgets 
			OpenGLWidgets 
			REQUIRED)

# fetch libraries we want to use
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240116.2
)

FetchContent_Declare(
  sourcepp
  GIT_REPOSITORY https://github.com/craftablescience/sourcepp.git
  GIT_TAG v2026.1.3
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs")  # ON if you want .so/.dll
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "Abseil uses your C++ standard")

FetchContent_MakeAvailable(absl)
FetchContent_MakeAvailable(sourcepp)

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
	src/widgets/main_window.cpp
	src/widgets/main_window.h
	src/widgets/palette.cpp
	src/widgets/palette.h
	src/widgets/palette_element.cpp
	src/widgets/palette_element.h
	src/widgets/viewport_control.cpp
	src/widgets/viewport_draw.cpp
	src/widgets/viewport.cpp
	src/widgets/viewport.h

	# Utils
	src/util/optional_array.h
	src/util/3d/mesh.cpp
	src/util/3d/mesh.h
	src/util/3d/ray.cpp
	src/util/3d/ray.h
	src/util/debug.cpp
	src/util/debug.h
	src/util/math.cpp
	src/util/math.h
	src/util/scoped_timer.h
	# May remove in favor of QFile
	src/util/file_handler.cpp
	src/util/file_handler.h


	# This is a test chamber. Four walls, ceiling and a floor
	# ✅ Good enough for science! ❎ Not Aperture Science!
	src/test_chamber_actions.h
	src/test_chamber_graphical.cpp
	src/test_chamber_utils.cpp
	src/test_chamber_types.h
	src/test_chamber.cpp
	src/test_chamber.h

	src/selection.h
	src/selection.cpp

	src/undo.h
	src/undo.cpp

	# Package system
	src/package/package_loader.cpp
	src/package/package_loader.h
	src/package/package.cpp
	src/package/package.h
	src/package/package_format.h
	src/package/formats/dir_format.cpp
	src/package/formats/dir_format.h
	src/package/formats/archive_format.cpp
	src/package/formats/archive_format.h

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

	absl::base
	absl::strings
	absl::str_format
	absl::flat_hash_map
	absl::log

	sourcepp::vpkpp
    sourcepp::kvpp
)

# add_executable(octtreetest ${CMAKE_CURRENT_SOURCE_DIR}/tests/octtree_test.cpp
# 	src/test_chamber_actions.h
# 	src/test_chamber_actions.cpp
# 	src/test_chamber_types.h
# 	src/test_chamber.cpp
# 	src/test_chamber.h
# 	src/selection.cpp
# 	src/selection.h
# 	src/undo.h
# 	src/undo.cpp
# 	src/util/3d/mesh.cpp
# 	src/util/3d/mesh.h
# 	src/util/3d/ray.cpp
# 	src/util/3d/ray.h
# 	src/util/math.h
# 	src/util/math.h
#  "src/util/safemap.h" "src/test_chamber_graphical.cpp" "src/test_chamber_utils.cpp" "src/widgets/viewport_control.cpp")
# set_target_properties(octtreetest
# 	PROPERTIES RUNTIME_OUTPUT_DIRECTORY
# 	"${CMAKE_CURRENT_SOURCE_DIR}/bin")
# set_target_properties(octtreetest
# 	PROPERTIES AUTOGEN_BUILD_DIR
# 	"${CMAKE_CURRENT_SOURCE_DIR}/bin/autogen/octtreetest")
# add_test(NAME OctTreeTest COMMAND octtreetest)
# target_link_libraries(octtreetest PRIVATE
# 	Qt6::Core
# 	Qt6::Gui
# 	Qt6::Widgets
# 	Qt6::OpenGLWidgets
# )

set_property(TARGET PuzzlemakerPlus PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:PuzzlemakerPlus>")
set_property(TARGET PuzzlemakerPlus PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PuzzlemakerPlus)

message("windeployqt6.exe at ${qt_bin_path}/windeployqt6.exe")
function(windeployqt target)

	if (WIN32)

		# POST_BUILD step
		# - after build, we have a bin/lib for analyzing qt dependencies
		# - we run windeployqt on target and deploy Qt libs
		add_custom_command(TARGET ${target} POST_BUILD
				COMMAND "${qt_bin_path}/windeployqt6.exe"
				--verbose 1
				$<IF:$<CONFIG:Debug>,--debug,--release>
				\"$<TARGET_FILE:${target}>\"
				COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}' ..."
				)

	endif ()

endfunction()
windeployqt(PuzzlemakerPlus)