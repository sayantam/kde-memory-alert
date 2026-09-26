# This application is installed for the current user, including autostart.
set(_data_home "$ENV{XDG_DATA_HOME}")
if(NOT IS_ABSOLUTE "${_data_home}")
    set(_data_home "$ENV{HOME}/.local/share")
endif()
set(_config_home "$ENV{XDG_CONFIG_HOME}")
if(NOT IS_ABSOLUTE "${_config_home}")
    set(_config_home "$ENV{HOME}/.config")
endif()
set(MEMORY_ALERT_DATA_HOME "${_data_home}" CACHE PATH "User XDG data directory")
set(MEMORY_ALERT_CONFIG_HOME "${_config_home}" CACHE PATH "User XDG config directory")
foreach(directory MEMORY_ALERT_DATA_HOME MEMORY_ALERT_CONFIG_HOME)
    if(NOT IS_ABSOLUTE "${${directory}}")
        message(FATAL_ERROR "${directory} must be an absolute path")
    endif()
endforeach()

set(APP_EXEC "${KDE_INSTALL_FULL_BINDIR}/kde-memory-alert")
# Desktop Entry Exec quoting has two layers: string escapes, then argument escapes.
string(REPLACE "\\" "\\\\\\\\" APP_EXEC "${APP_EXEC}")
string(REPLACE "\"" "\\\\\"" APP_EXEC "${APP_EXEC}")
string(REPLACE "$" "\\\\$" APP_EXEC "${APP_EXEC}")
string(REPLACE "`" "\\\\`" APP_EXEC "${APP_EXEC}")
string(REPLACE "%" "%%" APP_EXEC "${APP_EXEC}")
set(APP_DESKTOP_ID io.github.sayantam.kde-memory-alert.desktop)
configure_file(data/app.desktop.in ${APP_DESKTOP_ID} @ONLY)
configure_file(data/autostart.desktop.in autostart/${APP_DESKTOP_ID} @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_DESKTOP_ID}"
    DESTINATION "${MEMORY_ALERT_DATA_HOME}/applications")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/autostart/${APP_DESKTOP_ID}"
    DESTINATION "${MEMORY_ALERT_CONFIG_HOME}/autostart")

configure_file(cmake/Uninstall.cmake.in uninstall.cmake @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
