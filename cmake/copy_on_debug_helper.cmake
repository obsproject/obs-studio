string(REGEX REPLACE "\\.(dll|exe)$" ".pdb" FNAME "${FNAME}")

if(CONFIG STREQUAL Debug OR CONFIG STREQUAL RelWithDebInfo)
	file(COPY "${INPUT}/${FNAME}" DESTINATION "${OUTPUT}")
elseif(EXISTS "${OUTPUT}/${FNAME}")
	file(REMOVE "${OUTPUT}/${FNAME}")
endif()
