cmake_minimum_required(VERSION 3.6)
project(mtscan)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK REQUIRED gtk+-2.0)
include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})
add_definitions(${GTK_CFLAGS_OTHER})

pkg_check_modules(LIBSSH REQUIRED libssh)
include_directories(${LIBSSH_INCLUDE_DIRS})
link_directories(${LIBSSH_LIBRARY_DIRS})
add_definitions(${LIBSSH_CFLAGS_OTHER})

pkg_check_modules(YAJL REQUIRED yajl)
include_directories(${YAJL_INCLUDE_DIRS})
link_directories(${YAJL_LIBRARY_DIRS})
add_definitions(${YAJL_CFLAGS_OTHER})

pkg_check_modules(ZLIB REQUIRED zlib)
include_directories(${ZLIB_INCLUDE_DIRS})
link_directories(${ZLIB_LIBRARY_DIRS})
add_definitions(${ZLIB_CFLAGS_OTHER})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")

set(SOURCE_FILES
        conf.c
        conf.h
        conn.c
        conn.h
        export.c
        export.h
        gps.c
        gps.h
        log.c
        log.h
        main.c
        model.c
        model.h
        mt-ssh.c
        mt-ssh.h
        mtscan.h
        network.c
        network.h
        scanlist.c
        scanlist.h
        signals.c
        signals.h
        ui-connect.c
        ui-connect.h
        ui-dialogs.c
        ui-dialogs.h
        ui-icons.c
        ui-icons.h
        ui-preferences.c
        ui-preferences.h
        ui-toolbar.c
        ui-toolbar.h
        ui-update.c
        ui-update.h
        ui-view-menu.c
        ui-view-menu.h
        ui-view.c
        ui-view.h
        ui.c
        ui.h)

set(SOURCE_FILES_MINGW
        win32.c
        win32.h
        icon.rc)

set(LIBRARIES
        ${GTK_LIBRARIES}
        ${LIBSSH_LIBRARIES}
        ${YAJL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        m)

set(LIBRARIES_UNIX
        gps)

set(LIBRARIES_MINGW
        ws2_32
        winmm)

if(MINGW)
    IF(NOT (CMAKE_BUILD_TYPE MATCHES Debug))
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows")
    ENDIF()
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
    add_executable(mtscan ${SOURCE_FILES} ${SOURCE_FILES_MINGW})
    target_link_libraries(mtscan ${LIBRARIES} ${LIBRARIES_MINGW})
ELSE()
    add_executable(mtscan ${SOURCE_FILES})
    target_link_libraries(mtscan ${LIBRARIES} ${LIBRARIES_UNIX})
ENDIF()
