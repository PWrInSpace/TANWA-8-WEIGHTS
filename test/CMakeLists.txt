cmake_minimum_required(VERSION 3.16)
project(ads1256_unit_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(MOCKS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
set(UNITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unity)

include_directories(
    ${UNITY_DIR}
    ${MOCKS_DIR}
    ${MOCKS_DIR}/freertos
    ${PROJECT_ROOT}/components/hardware
)

add_executable(test_ads1256
    ${UNITY_DIR}/unity.c
    ${MOCKS_DIR}/mock_spi.c
    ${PROJECT_ROOT}/components/hardware/ads1256.c
    test_ads1256.c
)

enable_testing()
add_test(NAME ads1256_tests COMMAND test_ads1256)
