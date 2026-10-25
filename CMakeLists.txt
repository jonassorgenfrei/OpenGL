cmake_minimum_required (VERSION 3.24)
cmake_policy(VERSION 3.24)

# project name
project(OpenGLSamples LANGUAGES C CXX)
# version number
set (${PROJECT_NAME}_VERSION_MAJOR 1)
set (${PROJECT_NAME}_VERSION_MINOR 0)

# Folders 
set(FOLDERS
    Advanced_OpenGL
    Basics
    Lighting
    Model
    PBR
    Shadows
)

#Targets
set(Advanced_OpenGL
    AdvancedDataGLSL
    AdvancedOGL
    Anti_Aliasing_Off_Screen
    Anti_Aliasing_On_Screen
    Batching
    Billboards
    Compute_Shader
    Compute_Shader_Particles
    Geometry_Shader
    Geometry_Shader_Exploding
    Instancing
    no_Instancing_Space
    Instancing_Space
    Mesh_Shader
    Order_Independent_Transparency_Depth_Peeling
    Order_Independent_Transparency_Weighted_Blended
    Stencil_Silhouette
    Tessellation
    Tessellation_Terrain
    Transform_Feedback
    Transform_Feedback_Particles
)

set(Basics
    CoordinateSystem
    Camera
    Shader
    Text
    Texture
    Transformation
)

set(Lighting
    Advanced_Lighting
    Bloom
    Physically_Based_Bloom
    Colors
    Deferred_Shading
    Forward_Plus
    Gamma_Correction
    Gooch_Model
    HDR
    Lights
    Normal_Mapping
    Parallax_Mapping
    SSAO
)

set(Model
    Basic
    Advanced_Model
    Skeletal_Animation
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


set(CMAKE_CXX_STANDARD 17)
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

include(cmake/Dependencies.cmake)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# configure file paths
configure_file(configuration/root_directory.h.in configuration/root_directory.h)
configure_file(configuration/sample_directories.h.in configuration/sample_directories.h)

include_directories(${CMAKE_BINARY_DIR}/configuration)

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
  # Provide the sample title displayed by the shared help overlay.
  target_compile_definitions(${NAME} PRIVATE OPENGL_SAMPLE_NAME="${folder}/${sample}")
  target_link_libraries(${NAME} ${LIBS})
  if(MSVC)
  target_compile_options(${NAME} PRIVATE /std:c++17 /MP)
      target_link_options(${NAME} PUBLIC /ignore:4099)
  endif(MSVC)
  if(WIN32)
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")
      set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}/${CMAKE_BUILD_TYPE}")
  elseif(UNIX AND NOT APPLE)
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")
  elseif(APPLE)
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/${folder}")
      set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/${folder}")
  endif(WIN32)

  add_sample_directory(${NAME} ${SAMPLE_PATH})

  if(WIN32 AND NOT OPENGL_FETCH_DEPENDENCIES)
    # Resolve the selected configuration's DLLs, including transitive dependencies.
    add_custom_command(TARGET ${NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
        "-DRUNTIME_DLLS=$<TARGET_RUNTIME_DLLS:${NAME}>"
        "-DDESTINATION=$<TARGET_FILE_DIR:${NAME}>"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake"
      VERBATIM
    )
  endif()

  # copy shader files to build directory
  
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
