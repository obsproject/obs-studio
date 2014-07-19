
if(NOT EXISTS "${INPUT}")
	return()
endif()

set(_do_pass FALSE)
foreach(target ${TARGET_CONFIGS})
	if(target STREQUAL "${CONFIG}" OR target STREQUAL "ALL")
		set(_do_pass TRUE)
	endif()
endforeach()

if(NOT _do_pass)
	return()
endif()

file(COPY "${INPUT}" DESTINATION "${OUTPUT}")
