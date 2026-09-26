add_executable(memory-tests memory-tests.cpp)
target_link_libraries(memory-tests PRIVATE memory_ui Qt6::Test)
find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)
add_test(NAME memory-tests COMMAND ${DBUS_RUN_SESSION} -- $<TARGET_FILE:memory-tests> -platform offscreen)
set_tests_properties(memory-tests PROPERTIES TIMEOUT 30 ENVIRONMENT "LC_ALL=C.UTF-8;QT_QPA_PLATFORM=offscreen")

# This tool is deliberately not installed or exposed by the production binary.
add_executable(memory-alert-harness desktop-harness.cpp)
target_link_libraries(memory-alert-harness PRIVATE memory_ui KF6::DBusAddons)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME session-smoke COMMAND ${DBUS_RUN_SESSION} -- ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/session-smoke.py
    $<TARGET_FILE:kde-memory-alert> $<TARGET_FILE:memory-alert-harness>)
set_tests_properties(session-smoke PROPERTIES TIMEOUT 40 ENVIRONMENT "LC_ALL=C.UTF-8;QT_QPA_PLATFORM=offscreen")
