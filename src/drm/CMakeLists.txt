if( LIBDRM_FOUND )

add_executable( render_to_drm
  render_to_drm.cpp
)

target_include_directories(
  render_to_drm
  PRIVATE
  ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(
  render_to_drm
  tvs_common
  Boost::program_options
  Boost::system
  ${LIBDRM_LIBRARIES}
  ${pkgcfg_lib_Vulkan_vulkan}
)

target_compile_definitions( render_to_drm PRIVATE -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" )
target_compile_definitions( render_to_drm PRIVATE -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )

add_shader( render_to_drm shader.vert )
add_shader( render_to_drm shader.frag )

endif()

