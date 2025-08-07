
message(STATUS "Setting up Qt6")

# Let user override with environment variable
if(DEFINED ENV{Qt6_DIR})
    set(Qt6_DIR $ENV{Qt6_DIR} CACHE PATH "Qt6 config path from env")
endif()

# Only try to auto-detect if Qt6_DIR not already set
if(NOT Qt6_DIR)
    if(WIN32)
        # Windows: check user's home Qt installation
        file(GLOB QT6_CMAKE_PATHS
            "C:/Qt/6*/mingw_64/lib/cmake/Qt6"
        )

        list(LENGTH QT6_CMAKE_PATHS _qt_path_count)
        if(_qt_path_count GREATER 0)
            list(GET QT6_CMAKE_PATHS 0 Qt6_DIR)
            message(STATUS "Auto-detected Qt6_DIR (Windows): ${Qt6_DIR}")
            set(Qt6_DIR "${Qt6_DIR}" CACHE PATH "Qt6 config path")
        endif()
    elseif(UNIX)
        # Linux: try user home path
        set(QT_POSSIBLE_HOME "$ENV{HOME}/Qt")

        # Check for known subfolders like gcc_64 or clang_64
        file(GLOB QT6_CMAKE_PATHS
            "${QT_POSSIBLE_HOME}/6*/gcc_64/lib/cmake/Qt6"
            "${QT_POSSIBLE_HOME}/6*/clang_64/lib/cmake/Qt6"
        )

        list(LENGTH QT6_CMAKE_PATHS _qt_path_count)
        if(_qt_path_count GREATER 0)
            list(GET QT6_CMAKE_PATHS 0 Qt6_DIR)
            message(STATUS "Auto-detected Qt6_DIR: ${Qt6_DIR}")
            set(Qt6_DIR "${Qt6_DIR}" CACHE PATH "Qt6 config path")
        elseif(EXISTS "/usr/lib/cmake/Qt6")
            set(Qt6_DIR "/usr/lib/cmake/Qt6" CACHE PATH "Qt6 config path")
        elseif(EXISTS "/usr/lib/x86_64-linux-gnu/cmake/Qt6")
            set(Qt6_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6" CACHE PATH "Qt6 config path")
        endif()
    endif()
endif()