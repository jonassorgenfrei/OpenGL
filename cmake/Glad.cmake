# Generate the pinned GLAD 1.x bindings without relying on shell environment
# variables or an embeddable interpreter's installation-specific module path.
set(glad_header "${glad_BINARY_DIR}/include/glad/glad.h")
set(glad_platform_header "${glad_BINARY_DIR}/include/KHR/khrplatform.h")
set(glad_source "${glad_BINARY_DIR}/src/glad.c")
file(GLOB_RECURSE glad_generator_inputs CONFIGURE_DEPENDS
    "${glad_SOURCE_DIR}/glad/*.py"
    "${glad_SOURCE_DIR}/glad/files/*.xml"
    "${glad_SOURCE_DIR}/glad/files/*.h")
add_custom_command(
    OUTPUT "${glad_header}" "${glad_platform_header}" "${glad_source}"
    COMMAND "${Python_EXECUTABLE}" -X utf8
        "${CMAKE_CURRENT_LIST_DIR}/GenerateGlad.py" "${glad_SOURCE_DIR}"
        --profile=core "--out-path=${glad_BINARY_DIR}" --api=gl=4.6
        --generator=c --spec=gl --reproducible
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/GenerateGlad.py" ${glad_generator_inputs}
    COMMENT "Generating GLAD"
    VERBATIM)
add_custom_target(glad-generate-files
    DEPENDS "${glad_header}" "${glad_platform_header}" "${glad_source}")
add_library(glad STATIC "${glad_source}" "${glad_header}" "${glad_platform_header}")
add_dependencies(glad glad-generate-files)
target_include_directories(glad PUBLIC "${glad_BINARY_DIR}/include")
