# Apply the wxWidgets WebView2 extension once, including when an existing
# ExternalProject source tree already contains the change.
if(NOT DEFINED GIT_EXECUTABLE OR NOT DEFINED PATCH_FILE)
    message(FATAL_ERROR "GIT_EXECUTABLE and PATCH_FILE are required")
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --whitespace=nowarn "${PATCH_FILE}"
    RESULT_VARIABLE apply_result
    OUTPUT_VARIABLE apply_output
    ERROR_VARIABLE apply_error
)
if(apply_result EQUAL 0)
    message(STATUS "Applied wxWidgets WebView2 browser-argument patch")
    return()
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${PATCH_FILE}"
    RESULT_VARIABLE reverse_result
    OUTPUT_VARIABLE reverse_output
    ERROR_VARIABLE reverse_error
)
if(reverse_result EQUAL 0)
    message(STATUS "wxWidgets WebView2 browser-argument patch already applied")
    return()
endif()

message(FATAL_ERROR
    "Could not apply wxWidgets WebView2 browser-argument patch.\n"
    "git apply output: ${apply_output}${apply_error}\n"
    "reverse-check output: ${reverse_output}${reverse_error}")
