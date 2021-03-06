cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(xaml DESCRIPTION "Xaml" VERSION 0.1.0 LANGUAGES CXX)

# Version
set(PROJECT_VENDOR "Company")
configure_file(res/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.h LF)

# Sources
file(GLOB_RECURSE sources src/*.[hc]pp src/*.xaml)

# Executable
add_executable(${PROJECT_NAME} WIN32 ${sources} src/main.manifest src/main.rc)
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.hpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi windowsapp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Dependencies
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  # Install
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

  if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    install(FILES $<TARGET_FILE_DIR:${PROJECT_NAME}>/fmt.dll DESTINATION .)
  endif()

  # Package
  string(UUID PROJECT_GUID TYPE SHA1
    NAMESPACE 6BA7B814-9DAD-11D1-80B4-00C04FD430C8
    NAME "CN=${PROJECT_NAME}, O=${PROJECT_VENDOR}")

  set(CPACK_PACKAGE_VENDOR "${PROJECT_VENDOR}")
  set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
  set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "${PROJECT_DESCRIPTION}")
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
  set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")

  set(CPACK_GENERATOR "WIX")
  set(CPACK_WIX_UPGRADE_GUID "${PROJECT_GUID}")
  set(CPACK_PACKAGE_NAME "${PROJECT_DESCRIPTION}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_DESCRIPTION}")
  set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/main.ico")
  set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_SOURCE_DIR}/res/license.rtf")
  set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/res/bitmap/banner.bmp")
  set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/res/bitmap/dialog.bmp")
  set(CPACK_WIX_PROGRAM_MENU_FOLDER ".")

  include(CPack)
endif()
