cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets Svg)

add_library(idian STATIC)
add_library(OBS::idian ALIAS idian)

target_sources(
  idian
  PRIVATE
    components/CheckBox.cpp
    components/ComboBox.cpp
    components/DoubleSpinBox.cpp
    components/ExpandButton.cpp
    components/RowInfo.cpp
    components/SpinBox.cpp
    components/ToggleSwitch.cpp
    include/Idian/CheckBox.hpp
    include/Idian/CollapsibleGroup.hpp
    include/Idian/ComboBox.hpp
    include/Idian/DoubleSpinBox.hpp
    include/Idian/ExpandButton.hpp
    include/Idian/Idian.hpp
    include/Idian/ListHeader.hpp
    include/Idian/Row.hpp
    include/Idian/RowInfo.hpp
    include/Idian/RowList.hpp
    include/Idian/SpinBox.hpp
    include/Idian/StateEventFilter.cpp
    include/Idian/StateEventFilter.hpp
    include/Idian/ToggleSwitch.hpp
    include/Idian/Utils.cpp
    include/Idian/Utils.hpp
    widgets/CollapsibleGroup.cpp
    widgets/ListHeader.cpp
    widgets/Row.cpp
    widgets/RowList.cpp
)
target_sources(idian PUBLIC include/Idian/Idian.hpp)

set_target_properties(idian PROPERTIES AUTOMOC ON FOLDER "frontend")

target_include_directories(idian PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(idian PUBLIC Qt::Core Qt::Widgets Qt::Svg)
