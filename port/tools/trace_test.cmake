# One per-frame trace test: write the input's trace from the asm, then check
# the native routines against it (see src/trace.h).
#   cmake -DWL3=<wl3> -DINPUT=<x.input> -DTRACE=<x.trace> -P trace_test.cmake
execute_process(COMMAND ${WL3} --headless --input ${INPUT} --trace-write ${TRACE}
	RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out)
message("${out}")
if(result)
	message(FATAL_ERROR "writing the trace failed")
endif()
execute_process(COMMAND ${WL3} --headless --input ${INPUT} --trace-check ${TRACE}
	RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out)
message("${out}")
if(result)
	message(FATAL_ERROR "the trace check failed")
endif()
