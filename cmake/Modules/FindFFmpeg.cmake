# - Try to find FFmpeg
# Once done this will define
#   FFmpeg_FOUND
#   FFmpeg_INCLUDE_DIRS
#   FFmpeg_LIBRARIES
#   FFmpeg_INCLUDE_FILES
# Author: Matt Coffin <mcoffin13@gmail.com>

if (NOT FFmpeg_FIND_COMPONENTS)
	set(FFmpeg_FIND_COMPONENTS avcodec avdevice avfilter avformat avutil swscale)
endif(NOT FFmpeg_FIND_COMPONENTS)

# Generate component include files and requirements
foreach(comp ${FFmpeg_FIND_COMPONENTS})
	list(APPEND required "FFmpeg_${comp}_LIBRARY")
	list(APPEND FFmpeg_INCLUDE_FILES "lib${comp}/${comp}.h")
endforeach(comp)

# Find root include directory
find_path(FFmpeg_INCLUDE_DIR
	NAMES ${FFmpeg_INCLUDE_FILES}
	DOC "FFmpeg include directory"
)

# Find libraries
foreach(comp ${FFmpeg_FIND_COMPONENTS})
	find_library(FFmpeg_${comp}_LIBRARY
		NAMES ${comp}
		PATH_SUFFIXES ${comp} lib${comp}
		DOC "FFmpeg ${comp} library"
	)
	list(APPEND FFmpeg_LIBRARIES ${FFmpeg_${comp}_LIBRARY})
endforeach(comp)

# Set exported variables
set(FFmpeg_INCLUDE_DIRS ${FFmpeg_INCLUDE_DIR})

# Run checks via find_package_handle_standard_args
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
	FOUND_VAR FFmpeg_FOUND
	REQUIRED_VARS ${required} FFmpeg_INCLUDE_DIRS
)
