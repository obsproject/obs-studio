cmake_minimum_required(VERSION 3.28)
project(aerium-account-tests LANGUAGES CXX)

enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(frontend "${CMAKE_CURRENT_SOURCE_DIR}/../../frontend")
add_executable(
  aerium-account-tests
  account-test.cpp
  "${frontend}/widgets/AeriumAccountWidget.cpp"
  "${frontend}/widgets/AeriumAccountWidget.hpp"
)
target_include_directories(aerium-account-tests PRIVATE "${frontend}")
target_link_libraries(aerium-account-tests PRIVATE Qt6::Widgets Qt6::Network)
target_compile_definitions(aerium-account-tests PRIVATE QT_ENABLE_STRICT_MODE_UP_TO=0xFF0000)
target_compile_options(aerium-account-tests PRIVATE "$<$<CXX_COMPILER_ID:AppleClang,Clang>:-Werror=newline-eof>")
add_test(NAME aerium-account COMMAND aerium-account-tests)
set_tests_properties(aerium-account PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 90)
