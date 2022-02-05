cmake_minimum_required(VERSION 3.0)

# set vitasdk path variable
if (NOT DEFINED ENV{VITASDK})
    set(ENV{VITASDK} "/usr/local/vitasdk")
endif ()
set(VITASDK $ENV{VITASDK})
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
include("$ENV{VITASDK}/share/vita.toolchain.cmake" REQUIRED)

project(knet)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

add_executable(${PROJECT_NAME}.elf main.c)

target_link_libraries(${PROJECT_NAME}.elf
        taihenForKernel_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceNetPsForDriver_stub
        SceDebugForDriver_stub
        k
        gcc
        -nostdlib
        )

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}.elf
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/knet.yml
        UNSAFE)