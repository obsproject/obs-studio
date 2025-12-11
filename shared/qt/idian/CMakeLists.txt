cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets Svg)

add_library(idian STATIC)
add_library(OBS::idian ALIAS idian)

target_sources(
  idian
  PRIVATE
    components/AbstractPopup.cpp
    components/CheckBox.cpp
    components/ComboBox.cpp
    components/DoubleSpinBox.cpp
    components/GuidePopup.cpp
    components/InfoPopup.cpp
    components/SpinBox.cpp
    components/ToggleSwitch.cpp
    components/Tooltip.cpp
    include/Idian/AbstractPopup.hpp
    include/Idian/CheckBox.hpp
    include/Idian/ComboBox.hpp
    include/Idian/DoubleSpinBox.hpp
    include/Idian/Group.hpp
    include/Idian/GuidePopup.hpp
    include/Idian/Idian.hpp
    include/Idian/InfoPopup.hpp
    include/Idian/PopupContents.hpp
    include/Idian/PropertiesList.hpp
    include/Idian/Row.hpp
    include/Idian/SpinBox.hpp
    include/Idian/ToggleSwitch.hpp
    include/Idian/Tooltip.hpp
    include/Idian/Utils.hpp
    widgets/Group.cpp
    widgets/PropertiesList.cpp
    widgets/Row.cpp
)
target_sources(idian PUBLIC include/Idian/Idian.hpp)

set_target_properties(idian PROPERTIES AUTOMOC ON FOLDER "frontend")

target_include_directories(idian PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(idian PUBLIC Qt::Core Qt::Widgets Qt::Svg)
