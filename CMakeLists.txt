cmake_minimum_required(VERSION 3.20)
project(csem C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-g -O0 -DDEBUG -Wpointer-to-int-cast")
add_definitions(-DLEFTTORIGHT)
add_definitions(-DSPARC)

#if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(BISON_EXECUTABLE /Users/jasonketterer/Downloads/bison-3.8.2/src/bison)
#endif()

find_package(BISON)

BISON_TARGET(MyParser cgram.y ${CMAKE_SOURCE_DIR}/cgram.tab.c
        DEFINES_FILE ${CMAKE_SOURCE_DIR}/cgram.tab.h)

set (PROJECT_NAME_SOURCES
        ${PROJECT_NAME_SOURCES}
        cgram.tab.c
        semdum.c
        sym.c
        scan.c
        semutil.c
        )

set (PROJECT_NAME_HEADERS
        ${PROJECT_NAME_HEADERS}
        cc.h
        scan.h
        semutil.h
        sym.h
        sem.h
        cgram.tab.h
        )

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
add_executable(csem ${PROJECT_NAME_SOURCES} ${PROJECT_NAME_HEADERS} ${BISON_MyParser_OUTPUTS})

target_link_libraries(csem ${BISON_LIBRARIES})
