if( LIBDRM_FOUND )

add_executable( test_drm_32bit_1024_768
  test_drm_32bit_1024_768.cpp
)

target_include_directories(
  test_drm_32bit_1024_768
  PRIVATE
  ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(
  test_drm_32bit_1024_768
  ${LIBDRM_LIBRARIES}
)

add_executable( test_drm_16bit_1024_768
  test_drm_16bit_1024_768.cpp
)

target_include_directories(
  test_drm_16bit_1024_768
  PRIVATE
  ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(
  test_drm_16bit_1024_768
  ${LIBDRM_LIBRARIES}
)

add_executable( test_drm_32bit_320_240
  test_drm_32bit_320_240.cpp
)

target_include_directories(
  test_drm_32bit_320_240
  PRIVATE
  ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(
  test_drm_32bit_320_240
  ${LIBDRM_LIBRARIES}
)

add_executable( test_drm_16bit_320_240
  test_drm_16bit_320_240.cpp
)

target_include_directories(
  test_drm_16bit_320_240
  PRIVATE
  ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(
  test_drm_16bit_320_240
  ${LIBDRM_LIBRARIES}
)

endif()

