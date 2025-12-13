# ThreeDAssetsManager
add_library(ThreeDAssetsManagerController STATIC
    ThreeDAssetsManagerController.cpp
)

target_link_libraries(ThreeDAssetsManagerController
    PRIVATE
        Qt6::Core
        Qt6::Quick
)

# Install QML
install(FILES ThreeDAssetsManager.qml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/neural-studio/qml/managers/ThreeDAssetsManager
)
