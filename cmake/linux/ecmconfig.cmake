# OBS CMake Linux Extra CMake Module configuration module

include_guard(GLOBAL)

find_package(ECM REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
