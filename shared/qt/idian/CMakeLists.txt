cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets Svg)

add_library(idian STATIC)
add_library(OBS::idian ALIAS idian)

target_sources(
  idian
  PRIVATE
    OBSActionRow.cpp
    OBSActionRow.hpp
    OBSCheckBox.cpp
    OBSCheckBox.hpp
    OBSComboBox.cpp
    OBSComboBox.hpp
    OBSDoubleSpinBox.cpp
    OBSDoubleSpinBox.hpp
    OBSIdianWidget.hpp
    OBSGroupBox.cpp
    OBSGroupBox.hpp
    OBSPropertiesList.cpp
    OBSPropertiesList.hpp
    OBSSpinBox.cpp
    OBSSpinBox.hpp
    OBSToggleSwitch.cpp
    OBSToggleSwitch.hpp
)
target_sources(idian PUBLIC include/Idian/Idian.hpp)

set_target_properties(idian PROPERTIES AUTOMOC ON FOLDER "frontend")

target_include_directories(idian PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(idian PUBLIC Qt::Core Qt::Widgets Qt::Svg)
