# Copy static files to binary directory
execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
        "${CMAKE_BINARY_DIR}/src/"
)
