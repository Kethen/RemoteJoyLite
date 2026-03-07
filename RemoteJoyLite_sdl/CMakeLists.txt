cmake_minimum_required(VERSION 3.20)

project(RemoteJoyLite-cross)

find_package(SDL3 REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# ---------------------------------------------------------------------------
# Find libusb-1.0 (pkg-config first, then manual fallback for cross-builds)
# ---------------------------------------------------------------------------
if(NOT CMAKE_CROSSCOMPILING)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0 IMPORTED_TARGET QUIET)
  endif()
endif()

if(NOT LIBUSB_FOUND)
  find_path(LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    PATH_SUFFIXES libusb-1.0
    HINTS "${LIBUSB_ROOT}/include"
  )
  find_library(LIBUSB_LIBRARY
    NAMES usb-1.0 usb libusb-1.0 libusb usb1
    HINTS
      "${LIBUSB_ROOT}/lib"
      "${LIBUSB_ROOT}/MinGW64/static"
      "${LIBUSB_ROOT}/MinGW64/dll"
  )
  if(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARY)
    set(LIBUSB_FOUND TRUE)
    set(LIBUSB_INCLUDE_DIRS "${LIBUSB_INCLUDE_DIR}")
    set(LIBUSB_LIBRARIES "${LIBUSB_LIBRARY}")
  else()
    message(FATAL_ERROR "libusb-1.0 not found. Set LIBUSB_ROOT to its install prefix.")
  endif()
endif()
# ---------------------------------------------------------------------------

add_executable(${PROJECT_NAME}
  src/main.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  ${LIBUSB_INCLUDE_DIRS}
)

if(TARGET PkgConfig::LIBUSB)
  target_link_libraries(${PROJECT_NAME} SDL3::SDL3 PkgConfig::LIBUSB)
else()
  target_link_libraries(${PROJECT_NAME} SDL3::SDL3 ${LIBUSB_LIBRARIES})
endif()
