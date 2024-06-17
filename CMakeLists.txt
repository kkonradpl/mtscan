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

pkg_check_modules(LIBCURL REQUIRED libcurl)
include_directories(${LIBCURL_INCLUDE_DIRS})
link_directories(${LIBCURL_LIBRARY_DIRS})
add_definitions(${LIBCURL_CFLAGS_OTHER})

INCLUDE (CheckIncludeFile)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wall -pedantic -Wno-deprecated-declarations")

add_subdirectory(src)

if(NOT MAN_INSTALL_DIR)
    set(MAN_INSTALL_DIR share/man/man1)
endif()

find_program(TSHARK "tshark")
set(MANUF_FILE ${CMAKE_CURRENT_BINARY_DIR}/manuf)
if(TSHARK)
    execute_process(COMMAND ${TSHARK} -G manuf
                    OUTPUT_VARIABLE MANUF)
    if(MANUF)
        file(WRITE ${MANUF_FILE} ${MANUF})
        message("Successfully created manuf file for OUI lookup using tshark")
    else()
        message("Unable to create manuf file for OUI lookup (check tshark)")
    endif()
else()
    message("Unable to create manuf file for OUI lookup (install tshark)")
endif()

if(NOT MINGW)
    install(TARGETS mtscan DESTINATION bin)
    install(FILES manpages/mtscan.1 DESTINATION ${MAN_INSTALL_DIR})
    install(FILES applications/mtscan.desktop DESTINATION share/applications)
    install(DIRECTORY sounds/ DESTINATION share/sounds/mtscan
            PATTERN "INFO" EXCLUDE)
    install(DIRECTORY icons/ DESTINATION share/icons/hicolor
            PATTERN "INFO" EXCLUDE
            PATTERN "ico" EXCLUDE)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache /usr/share/icons/hicolor)")

    if(EXISTS ${MANUF_FILE})
        install(FILES ${MANUF_FILE} DESTINATION share/mtscan)
    endif()
endif()
