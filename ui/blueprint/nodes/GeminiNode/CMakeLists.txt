add_library(ui-gemininode STATIC
    GeminiNodeController.cpp
    GeminiNodeController.h
)

target_include_directories(ui-gemininode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui-gemininode
    Qt6::Core
    Qt6::Quick
    ui-basenode
)

install(FILES GeminiNode.qml
    DESTINATION share/neural-studio/qml/blueprint/nodes/GeminiNode
)
