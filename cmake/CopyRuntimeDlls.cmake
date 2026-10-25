# An empty list is valid when all manually supplied dependencies are static.
foreach(dll IN LISTS RUNTIME_DLLS)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dll}" "${DESTINATION}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()
