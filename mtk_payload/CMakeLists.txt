cmake_minimum_required(VERSION 3.16)
project(mtk_payload VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH} "/usr/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
find_library(GMP_LIBRARY gmp)
find_library(GMPXX_LIBRARY gmpxx)
find_path(GMP_INCLUDE_DIR gmpxx.h)

find_package(OpenSSL REQUIRED)

set(PAYLOADS_DIR "${CMAKE_SOURCE_DIR}/payloads")

add_library(mtk_payload_lib STATIC
    src/logger.cpp
    src/utils.cpp
    src/usb_transport.cpp
    src/preloader.cpp
    src/sla.cpp
    src/chipconfig.cpp
    src/mtk_config.cpp
    src/exploit.cpp
    src/exploit_kamakiri.cpp
    src/exploit_kamakiri2.cpp
    src/exploit_amonet.cpp
    src/exploit_hashimoto.cpp
    src/exploit_carbonara.cpp
    src/pltools.cpp
    src/meta.cpp
    src/payload_command.cpp
)

target_include_directories(mtk_payload_lib PUBLIC
    src
    ${LIBUSB_INCLUDE_DIRS}
    ${GMP_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(mtk_payload_lib PUBLIC
    ${LIBUSB_LIBRARIES}
    ${GMP_LIBRARY}
    ${GMPXX_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_compile_options(mtk_payload_lib PRIVATE
    -Wall -Wextra -Wpedantic
    ${LIBUSB_CFLAGS_OTHER}
)

add_executable(mtk_payload src/main.cpp)
target_link_libraries(mtk_payload PRIVATE mtk_payload_lib)
target_compile_definitions(mtk_payload PRIVATE
    PAYLOADS_DIR="${PAYLOADS_DIR}"
)

enable_testing()
add_executable(mtk_payload_tests tests/test_main.cpp)
target_link_libraries(mtk_payload_tests PRIVATE mtk_payload_lib)
target_compile_definitions(mtk_payload_tests PRIVATE
    PAYLOADS_DIR="${PAYLOADS_DIR}"
)
add_test(NAME MtkPayloadTests COMMAND mtk_payload_tests)
