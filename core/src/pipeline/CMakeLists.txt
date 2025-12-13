# Pipeline Module (Stitching, Routing)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(PIPELINE_SOURCES)

set(PIPELINE_HEADERS
    FrameRouter.h
    IStitcher.h
)

add_library(nstudio-pipeline STATIC
    ${PIPELINE_SOURCES}
    ${PIPELINE_HEADERS}
)

target_link_libraries(nstudio-pipeline
    PRIVATE
        Qt6::Core
)

target_include_directories(nstudio-pipeline PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
