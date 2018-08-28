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

pkg_check_modules(LIBCRYPTO REQUIRED libcrypto)
include_directories(${LIBCRYPTO_INCLUDE_DIRS})
link_directories(${LIBCRYPTO_LIBRARY_DIRS})
add_definitions(${LIBCRYPTO_CFLAGS_OTHER})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")

set(SOURCE_FILES
        callbacks.c
        callbacks.h
        conf-profile.c
        conf-profile.h
        conf-scanlist.c
        conf-scanlist.h
        conf.c
        conf.h
        export.c
        export.h
        gpsd.c
        gpsd.h
        gps.c
        gps.h
        log.c
        log.h
        main.c
        misc.c
        misc.h
        model.c
        model.h
        mt-ssh.c
        mt-ssh.h
        mtscan.h
        network.c
        network.h
        signals.c
        signals.h
        ui-callbacks.c
        ui-callbacks.h
        ui-connection.c
        ui-connection.h
        ui-dialogs.c
        ui-dialogs.h
        ui-dialog-pcap.c
        ui-dialog-pcap.h
        ui-icons.c
        ui-icons.h
        ui-preferences.c
        ui-preferences.h
        ui-scanlist.c
        ui-scanlist.h
        ui-scanlist-manager.c
        ui-scanlist-manager.h
        ui-toolbar.c
        ui-toolbar.h
        ui-view-menu.c
        ui-view-menu.h
        ui-view.c
        ui-view.h
        ui.c
        ui.h
        tzsp-receiver.c
        tzsp-receiver.h
        tzsp/tzsp-decap.c
        tzsp/tzsp-decap.h
        tzsp/nv2.c
        tzsp/nv2.h
        tzsp/mac80211.h
        tzsp/mac80211.c
        tzsp/ie-airmax.h
        tzsp/ie-airmax.c
        tzsp/ie-airmax-ac.h
        tzsp/ie-airmax-ac.c
        tzsp/ie-mikrotik.c
        tzsp/ie-mikrotik.h
        tzsp/ie-mikrotik-utils.c
        tzsp/ie-mikrotik-utils.h
        tzsp/tzsp-sniffer.c
        tzsp/tzsp-sniffer.h
        tzsp/tzsp-socket.c
        tzsp/tzsp-socket.h
        tzsp/utils.c
        tzsp/utils.h)

set(SOURCE_FILES_MINGW
        win32.c
        win32.h
        icon.rc
        tzsp/win32.h)

set(LIBRARIES
        ${GTK_LIBRARIES}
        ${LIBSSH_LIBRARIES}
        ${YAJL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${LIBCRYPTO_LIBRARIES}
        m)

set(LIBRARIES_UNIX
        pcap)

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

    SET(NPCAP_DIR /build/npcap-sdk-0.1)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}${NPCAP_DIR}/Include)
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBRARIES_MINGW_NPCAP ${CMAKE_CURRENT_SOURCE_DIR}${NPCAP_DIR}/Lib/x64/wpcap.lib)
    ELSE()
        set(LIBRARIES_MINGW_NPCAP ${CMAKE_CURRENT_SOURCE_DIR}${NPCAP_DIR}/Lib/wpcap.lib)
    ENDIF()
    add_executable(mtscan ${SOURCE_FILES} ${SOURCE_FILES_MINGW})
    target_link_libraries(mtscan ${LIBRARIES} ${LIBRARIES_MINGW} ${LIBRARIES_MINGW_NPCAP})
ELSE()
    add_executable(mtscan ${SOURCE_FILES})
    target_link_libraries(mtscan ${LIBRARIES} ${LIBRARIES_UNIX})
ENDIF()
