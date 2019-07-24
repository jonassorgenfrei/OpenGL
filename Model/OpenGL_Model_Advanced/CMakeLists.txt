# CMake Test File GLFW
# Minimum CMake Version
cmake_minimum_required (VERSION 3.3)
# Project Name
project(Model_Advanced)

# Output Dir
set(RuntimeOutputDir ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RuntimeOutputDir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${RuntimeOutputDir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RuntimeOutputDir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RuntimeOutputDir})

list(APPEND CMAKE_CXX_FLAGS "-std=c++11")

# Compiler Flags
if(MSVC)
  # Setzten des Warnunglevels auf (W4) unter Windows
  # behandeln der Warnungen als Fehler (WX) außer 4996
  if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX /wd4996")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /wd4996")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	if(APPLE) 
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long")
	else()
	  	# Setztes 
	  	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long -Werror")
	endif()
endif()

# Optimierung 
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Versionsnummer
set (${PROJECT_NAME}_VERSION_MAJOR 1)
set (${PROJECT_NAME}_VERSION_MINOR 0)

# find the required packages
find_package(GLM REQUIRED)					# GLM
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)					# GLFW3
FIND_PATH(GLFW3_INCLUDE_DIR "GLFW/glfw3.h" PATHS ${GLFW3_DIR}/../../../include )
FIND_LIBRARY(GLFW3_LIBRARY NAMES glfw3 glfw PATHS ${GLFW3_DIR}/../../ )
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)					# ASSIMP
message(STATUS "Found ASSIMP in ${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES}")

# setzten der Include Directories
include_directories( ${OPENGL_INCLUDE_DIR} 
					${GLM_INCLUDE_DIRS}
					${GLFW3_INCLUDE_DIR}
					${ASSIMP_INCLUDE_DIRS}
					$ENV{glad_DIR}/include 
					$ENV{stb_DIR}
					${CMAKE_CURRENT_SOURCE_DIR}/include)

# WINDOWS SYSTEM
if(WIN32)
	set(LIBS ${GLFW3_LIBRARY} opengl32 ${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES})

	# nicht ZERO_CHECK bauen
	set(CMAKE_SUPPRESS_REGENERATION true)
	# cmake Folder ALL_BUILD in FilterUnterordner
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	
	# if Visual Studio
	if(MSVC)
		# ${PROJECT_NAME} als Startup-Projekt setzten
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	endif()
elseif(UNIX AND NOT APPLE)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
	find_package(OpenGL REQUIRED)
	add_definitions(${OPENGL_DEFINITIONS})
	find_package(X11 REQUIRED)
	# note that the order is important for setting the libs
	# use pkg-config --libs $(pkg-config --print-requires --print-requires-private glfw3) in a terminal to confirm
	set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread ${ASSIMP_LIBRARY})
	set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
elseif(APPLE)
	INCLUDE_DIRECTORIES(/System/Library/Frameworks)
	FIND_LIBRARY(COCOA_LIBRARY Cocoa)
	FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
	FIND_LIBRARY(IOKit_LIBRARY IOKit)
	FIND_LIBRARY(CoreVideo_LIBRARY CoreVideo)
	MARK_AS_ADVANCED(COCOA_LIBRARY OpenGL_LIBRARY)
	SET(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
	SET(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY} ${ASSIMP_LIBRARY})
	set(LIBS ${LIBS} ${APPLE_LIBS})
else()
	set(LIBS )
endif(WIN32)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

# hinzufügen aller files im src Folder
file(GLOB src_files
	"src/*.h"
	"src/*.cpp"
)

# hinzufügen aller files im external Folder (rekursive)
file(GLOB_RECURSE SHADERS
	"shader/*"
)

# erstellen des Targets ${PROJECT_NAME} 
add_executable(${PROJECT_NAME} ${src_files} $ENV{glad_DIR}/src/glad.c ${SHADERS})


# erstellen der filter fuer die external-files
foreach(source IN LISTS SHADERS)
    get_filename_component(source_path "${source}" PATH)
    file(RELATIVE_PATH pathR "${CMAKE_CURRENT_SOURCE_DIR}" "${source_path}")
    string(REPLACE "/" "\\" source_path_ide "${pathR}")
    source_group("${source_path_ide}" FILES "${source}")
endforeach()

# linken der Libraries
if(WIN32)
        target_link_libraries(${PROJECT_NAME} ${LIBS})
else()
        target_link_libraries(${PROJECT_NAME} ${LIBS})
endif()

## Shaders
#foreach(SHADER ${SHADERS})
#	if(WIN32)
#		# configure_file(${SHADER} "test")
#                add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} $<TARGET_FILE_DIR:${NAME}>)
#	elseif(UNIX AND NOT APPLE)
#                file(COPY ${SHADER} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/build)
#	elseif(APPLE)
#                # create symbolic link for *.vs *.fs *.gs
#                get_filename_component(SHADERNAME ${SHADER} NAME)
#                makeLink(${SHADER} ${CMAKE_CURRENT_BINARY_DIR}/build/${SHADERNAME} ${NAME})
#	endif(WIN32)
#endforeach(SHADER)



