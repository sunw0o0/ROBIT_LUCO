cmake_minimum_required(VERSION 3.16)
project(luco_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt: .ui 파일과 Q_OBJECT 자동 처리
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(luco_monitor
  src/main.cpp
  src/mainwindow.cpp
  include/luco_monitor/mainwindow.hpp
  include/luco_monitor/theme.hpp
  ui/mainwindow.ui
)
target_include_directories(luco_monitor PRIVATE include)
target_link_libraries(luco_monitor
  Qt${QT_VERSION_MAJOR}::Widgets
  rclcpp::rclcpp
  ${geometry_msgs_TARGETS}
)

install(TARGETS luco_monitor DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
