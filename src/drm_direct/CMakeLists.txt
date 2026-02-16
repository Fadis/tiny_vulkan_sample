if( LIBDRM_FOUND )

add_executable( render_to_drm_direct
  render_to_drm.cpp
)

target_include_directories(
  render_to_drm_direct
  PRIVATE
  ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(
  render_to_drm_direct
  tvs_common
  Boost::program_options
  Boost::system
  ${LIBDRM_LIBRARIES}
  ${pkgcfg_lib_Vulkan_vulkan}
)

target_compile_definitions( render_to_drm_direct PRIVATE -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" )
target_compile_definitions( render_to_drm_direct PRIVATE -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )

add_shader( render_to_drm_direct shader.vert )
add_shader( render_to_drm_direct shader.frag )

endif()

