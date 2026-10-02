cmake_minimum_required(VERSION 3.16)
project(PS5Streamer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Network)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/AppController.cpp
    src/NetworkService.cpp
    src/ConfigGenerator.cpp
    src/ProcessManager.cpp
    src/StreamKeyServer.cpp
    src/DnsInterceptor.cpp
    src/Paths.cpp
    src/Privilege.cpp
)

set(HEADERS
    src/MainWindow.h
    src/AppController.h
    src/NetworkService.h
    src/ConfigGenerator.h
    src/ProcessManager.h
    src/StreamKeyServer.h
    src/DnsInterceptor.h
    src/Paths.h
    src/Privilege.h
)

add_executable(PS5Streamer WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(PS5Streamer PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)

if(MSVC)
    # Source files are UTF-8; without this, QStringLiteral mojibake on Windows
    target_compile_options(PS5Streamer PRIVATE /utf-8)
endif()

if(WIN32)
    target_compile_definitions(PS5Streamer PRIVATE PS5STREAMER_WINDOWS)
    target_link_libraries(PS5Streamer PRIVATE shell32 advapi32)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(PS5Streamer PRIVATE PS5STREAMER_LINUX)
endif()

install(TARGETS PS5Streamer RUNTIME DESTINATION bin)

# Optional: install a desktop entry on Linux
if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/ps5streamer.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/ps5streamer.desktop
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ps5streamer.desktop
            DESTINATION share/applications)
endif()
