cmake_minimum_required(VERSION 3.22)
project(kde-memory-alert VERSION 0.1.0 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Per-user installation prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(ECM 6.14 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs6)
include(CTest)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets DBus)
find_package(KF6 6.14 REQUIRED COMPONENTS StatusNotifierItem DBusAddons I18n)
find_package(LayerShellQt 6.6 REQUIRED)

add_library(memory_core STATIC src/memoryreader.cpp src/monitor.cpp)
target_include_directories(memory_core PUBLIC src)
target_link_libraries(memory_core PUBLIC Qt6::Core)

add_library(memory_ui STATIC src/alertwindow.cpp src/desktopui.cpp src/trayplacement.cpp)
target_link_libraries(memory_ui PUBLIC memory_core Qt6::Widgets Qt6::DBus KF6::StatusNotifierItem KF6::I18n LayerShellQt::Interface)

add_executable(kde-memory-alert src/main.cpp)
target_link_libraries(kde-memory-alert PRIVATE memory_ui KF6::DBusAddons)
target_compile_definitions(kde-memory-alert PRIVATE APP_VERSION="${PROJECT_VERSION}")

foreach(target memory_core memory_ui kde-memory-alert)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

install(TARGETS kde-memory-alert RUNTIME DESTINATION ${KDE_INSTALL_BINDIR})
include(cmake/DesktopEntries.cmake)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
