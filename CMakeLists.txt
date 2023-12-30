cmake_minimum_required (VERSION 3.15)
cmake_policy(VERSION 3.15)

project(OpenGLSamples)
# version number
set (${PROJECT_NAME}_VERSION_MAJOR 1)
set (${PROJECT_NAME}_VERSION_MINOR 0)

set(CMAKE_CXX_STANDARD 17) # this does nothing for MSVC, use target_compile_options below
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
# optimization
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# enable folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif(WIN32)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# find the required packages
# GLM
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

# GLFW3
find_package(GLFW3 REQUIRED)
FIND_PATH(GLFW3_INCLUDE_DIRS "GLFW/glfw3.h" PATHS ${GLFW3_DIR}/../../../include )
FIND_LIBRARY(GLFW3_LIBRARY_DIRS NAMES glfw3 glfw PATHS ${GLFW3_DIR}/../../ )
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIRS}")

# ASSIMP
find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_ROOT_DIR}")

# GLAD
IF(DEFINED ENV{glad_DIR}) 
	set(GLAD_SRC_DIR $ENV{glad_DIR})
	set(GLAD_INCLUDE_DIRS ${GLAD_SRC_DIR}/include )
	message(STATUS "Found GLAD in ${GLAD_SRC_DIR}")
ELSE()
	message(FATAL_ERROR "GLAD ENVIRONMENT VARIABLE NOT SET")
ENDIF()

# STB
IF(DEFINED ENV{stb_DIR}) 
	set(STB_INCLUDE_DIRS $ENV{stb_DIR})
	message(STATUS "Found STB in $ENV{stb_DIR}")
ELSE()
	message(FATAL_ERROR "STB ENVIRONMENT VARIABLE NOT SET")
ENDIF()

# Freetype
find_package(Freetype REQUIRED)
message(STATUS "Found Freetype in ${FREETYPE_INCLUDE_DIRS}")

# find_package(SOIL REQUIRED)
# message(STATUS "Found SOIL in ${SOIL_INCLUDE_DIR}")
# find_package(GLEW REQUIRED)
# message(STATUS "Found GLEW in ${GLEW_INCLUDE_DIR}")

if(WIN32)
  set(LIBS ${GLFW3_LIBRARY_DIRS} opengl32 ${ASSIMP_LIBRARIES} ${FREETYPE_LIBRARIES})
  # set(LIBS glfw3 opengl32 assimp freetype irrKlang)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
  find_package(OpenGL REQUIRED)
  add_definitions(${OPENGL_DEFINITIONS})
  find_package(X11 REQUIRED)
  # note that the order is important for setting the libs
  # use pkg-config --libs $(pkg-config --print-requires --print-requires-private glfw3) in a terminal to confirm
  set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread freetype ${ASSIMP_LIBRARY})
  set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
elseif(APPLE)
  INCLUDE_DIRECTORIES(/System/Library/Frameworks)
  FIND_LIBRARY(COCOA_LIBRARY Cocoa)
  FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
  FIND_LIBRARY(IOKit_LIBRARY IOKit)
  FIND_LIBRARY(CoreVideo_LIBRARY CoreVideo)
  MARK_AS_ADVANCED(COCOA_LIBRARY OpenGL_LIBRARY)
  SET(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
  SET(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY} ${ASSIMP_LIBRARY} ${FREETYPE_LIBRARIES})
  set(LIBS ${LIBS} ${APPLE_LIBS})
else()
  set(LIBS )
endif(WIN32)

set(FOLDERS
    Basics
    Model
    PBR
    Shadows
)

set(Basics
    CoordinateSystem
    Camera
    Shader
    Text
    Texture
    Transformation
)

set(Model
    Basic
    Advanced
)

set(PBR
    Lighting
    Diffuse_irradiance
    Specular_IBL
)

set(Shadows
    Cascaded_Shadow_Mapping
    Point_Shadows
    Shadow_Mapping
    Shadow_Volumes
)

# configure file paths
configure_file(configuration/root_directory.h.in configuration/root_directory.h)
configure_file(configuration/sample_directories.h.in configuration/sample_directories.h)

include_directories(${CMAKE_BINARY_DIR}/configuration)

# first create relevant static libraries requried for other projects
add_library(STB_IMAGE "samples/stb_image.cpp")
set_property(TARGET STB_IMAGE PROPERTY FOLDER "Static Libraries")
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD "${GLAD_SRC_DIR}/src/glad.c")
set_property(TARGET GLAD PROPERTY FOLDER "Static Libraries")
set(LIBS ${LIBS} GLAD)



macro(makeLink src dest target)
  add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dest}  DEPENDS  ${dest} COMMENT "mklink ${src} -> ${dest}")
endmacro()

function(add_sample_directory target_name samplePath)
  string(CONCAT SAMPLE_DIR_CODE "#ifdef SAMPLE_${target_name}\n"
  "const char * logl_sample = \"${samplePath}\";\n"
  "#endif //SAMPLE_${target_name} \n"
  )

  write_file(${CMAKE_BINARY_DIR}/configuration/sample_directories.h "${SAMPLE_DIR_CODE}" APPEND)

  target_compile_definitions(${target_name} PUBLIC SAMPLE_${NAME})
endfunction()

function(create_project_from_sources folder sample)
  set(SAMPLE_PATH "samples/${folder}/${sample}")
  set(SAMPLE_PATH_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_PATH}")
  
	file(GLOB SOURCE
    "${SAMPLE_PATH}/src/*.h"
    "${SAMPLE_PATH}/src/*.cpp"
  )
  file(GLOB_RECURSE SHADERS
    "${SAMPLE_PATH}/shader/*"
  )
  if (sample STREQUAL "")
    SET(replaced "")
    string(REPLACE "/" "_" replaced ${folder})
    set(NAME ${replaced})
  else()
    set(NAME "${sample}")
  endif()
  
  add_executable(${NAME} ${SOURCE} ${SHADERS})
  target_link_libraries(${NAME} ${LIBS})
  if(MSVC)
  target_compile_options(${NAME} PRIVATE /std:c++17 /MP)
      target_link_options(${NAME} PUBLIC /ignore:4099)
  endif(MSVC)
  if(WIN32)
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")
      set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}/Debug")
  elseif(UNIX AND NOT APPLE)
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")
  elseif(APPLE)
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/${folder}")
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/${folder}")
  endif(WIN32)

  add_sample_directory(${NAME} ${SAMPLE_PATH})

  # copy shader files to build directory
  
  # copy dlls
  #if(WIN32)
  #   file(GLOB DLLS "dlls/*.dll")
  #   add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${DLLS} $<TARGET_FILE_DIR:${NAME}>)
  #endif(WIN32)

  foreach(SHADER ${SHADERS})
      if(WIN32)
          # configure_file(${SHADER} "test")
          add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} $<TARGET_FILE_DIR:${NAME}>)
      elseif(UNIX AND NOT APPLE)
          file(COPY ${SHADER} DESTINATION ${CMAKE_SOURCE_DIR}/bin/${folder})
      elseif(APPLE)
          # create symbolic link for shaders
          get_filename_component(SHADERNAME ${SHADER} NAME)
          makeLink(${SHADER} ${CMAKE_SOURCE_DIR}/bin/${folder}/${SHADERNAME} ${NAME})
      endif(WIN32)
  endforeach(SHADER)
  
  # if compiling for visual studio, also use configure file for each project (specifically to set up working directory)
  if(MSVC)
    # configure_file(${CMAKE_SOURCE_DIR}/configuration/visualstudio.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.vcxproj.user @ONLY)

    # create filter for shaders
    foreach(source IN LISTS SHADERS)
      get_filename_component(source_path "${source}" PATH)
      file(RELATIVE_PATH pathR "${SAMPLE_PATH_ABS}" "${source_path}")
      string(REPLACE "/" "\\" source_path_ide "${pathR}")
      source_group("${source_path_ide}" FILES "${source}")
    endforeach()

    set_property(TARGET ${NAME} PROPERTY FOLDER "${folder}")
  endif(MSVC)
endfunction()

# then create a project file per tutorial
foreach(FOLDER ${FOLDERS})
    foreach(SAMPLE ${${FOLDER}})
		create_project_from_sources(${FOLDER} ${SAMPLE})
    endforeach(SAMPLE)
endforeach(FOLDER)

include_directories(${OPENGL_INCLUDE_DIR} 
                    ${GLM_INCLUDE_DIRS}
                    ${GLFW3_INCLUDE_DIRS}
                    ${ASSIMP_INCLUDE_DIRS}
                    ${GLAD_INCLUDE_DIRS}
                    ${STB_INCLUDE_DIRS}
                    ${FREETYPE_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_SOURCE_DIR}/include)
