include(FetchContent)

option(OPENGL_FETCH_DEPENDENCIES "Download and build pinned dependencies" ON)

if(OPENGL_FETCH_DEPENDENCIES)
    # GLAD 0.1.36 declares a policy version older than CMake 4 supports.
    # Keep pinned dependencies configurable without overriding a user setting.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0" AND
       NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()

    # Static dependencies keep the samples runnable without copying DLLs.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    # The samples load OBJ, FBX, Collada and glTF assets; omit unused importers.
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    foreach(format OBJ FBX COLLADA GLTF)
        set(ASSIMP_BUILD_${format}_IMPORTER ON CACHE BOOL "" FORCE)
    endforeach()
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    foreach(package ZLIB BZIP2 PNG HARFBUZZ BROTLI)
        set(FT_DISABLE_${package} ON CACHE BOOL "" FORCE)
    endforeach()
    set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLAD_PROFILE core CACHE STRING "" FORCE)
    set(GLAD_API "gl=4.6" CACHE STRING "" FORCE)
    set(GLAD_ALL_EXTENSIONS ON CACHE BOOL "" FORCE)
    set(GLAD_REPRODUCIBLE ON CACHE BOOL "" FORCE)
    find_package(Python REQUIRED COMPONENTS Interpreter)

    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
    FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v5.4.3 GIT_SHALLOW TRUE)
    FetchContent_Declare(freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git GIT_TAG VER-2-13-3 GIT_SHALLOW TRUE)
    FetchContent_Declare(glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.36 GIT_SHALLOW TRUE)
    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG f0569113c93ad095470c54bf34a17b36646bbbb5)
    FetchContent_MakeAvailable(glm glfw assimp freetype glad stb)
    set(STB_INCLUDE_DIRS "${stb_SOURCE_DIR}")
    set(FREETYPE_TARGET freetype)
else()
    find_package(glm CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(assimp 5.3 CONFIG REQUIRED)
    find_package(Freetype REQUIRED)
    set(FREETYPE_TARGET Freetype::Freetype)
    set(glad_DIR "$ENV{glad_DIR}" CACHE PATH "GLAD 1.x source tree")
    set(stb_DIR "$ENV{stb_DIR}" CACHE PATH "Directory containing stb_image.h")
    if(NOT EXISTS "${glad_DIR}/include/glad/glad.h" OR
       NOT EXISTS "${glad_DIR}/src/glad.c")
        message(FATAL_ERROR "Set glad_DIR to a GLAD 1.x source tree with include/glad/glad.h and src/glad.c")
    endif()
    if(NOT EXISTS "${stb_DIR}/stb_image.h")
        message(FATAL_ERROR "Set stb_DIR to the directory containing stb_image.h")
    endif()
    add_library(glad STATIC "${glad_DIR}/src/glad.c")
    target_include_directories(glad PUBLIC "${glad_DIR}/include")
    set(STB_INCLUDE_DIRS "${stb_DIR}")
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
add_library(STB_IMAGE STATIC "${PROJECT_SOURCE_DIR}/samples/stb_image.cpp")
target_include_directories(STB_IMAGE PUBLIC "${STB_INCLUDE_DIRS}")
set_property(TARGET STB_IMAGE PROPERTY FOLDER "Static Libraries")
set(LIBS glm::glm glfw assimp::assimp ${FREETYPE_TARGET} OpenGL::GL glad STB_IMAGE)
