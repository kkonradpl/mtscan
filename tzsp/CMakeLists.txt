cmake_minimum_required(VERSION 3.6)
project(mtscan-tzsp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -DHAVE_PCAP")

set(SOURCE_FILES
        ie-airmax.h
        ie-airmax.c
        ie-airmax-ac.h
        ie-airmax-ac.c
        ie-mikrotik.c
        ie-mikrotik.h
        ie-mikrotik-utils.c
        ie-mikrotik-utils.h
        mac80211.h
        mac80211.c
        mtscan-tzsp.c
        nv2.c
        nv2.h
        tzsp-decap.c
        tzsp-decap.h
        tzsp-sniffer.c
        tzsp-sniffer.h
        tzsp-socket.c
        tzsp-socket.h
        utils.c
        utils.h)

set(LIBRARIES
        crypto
        m
        pcap)

add_executable(mtscan-tzsp ${SOURCE_FILES})
target_link_libraries(mtscan-tzsp ${LIBRARIES})
