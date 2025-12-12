include(FindPackageHandleStandardArgs)
set(${CMAKE_FIND_PACKAGE_NAME}_CONFIG ${CMAKE_CURRENT_LIST_FILE})
find_package_handle_standard_args(nlohmann_json CONFIG_MODE)

if(NOT TARGET nlohmann_json::nlohmann_json)
    include("${CMAKE_CURRENT_LIST_DIR}/nlohmann_jsonTargets.cmake")
    if((NOT TARGET nlohmann_json) AND
       (NOT nlohmann_json_FIND_VERSION OR
        nlohmann_json_FIND_VERSION VERSION_LESS 3.2.0))
        add_library(nlohmann_json INTERFACE IMPORTED)
        set_target_properties(nlohmann_json PROPERTIES
            INTERFACE_LINK_LIBRARIES nlohmann_json::nlohmann_json
        )
    endif()
endif()
