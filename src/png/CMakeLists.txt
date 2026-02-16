if( OpenImageIO_FOUND )

add_executable( render_to_png
  render_to_png.cpp
)

target_link_libraries(
  render_to_png
  tvs_common
  Boost::program_options
  Boost::system
  ${pkgcfg_lib_Vulkan_vulkan}
  ${OIIO_LIBRARIES}
)

target_compile_definitions( render_to_png PRIVATE -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" )
target_compile_definitions( render_to_png PRIVATE -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )

add_shader( render_to_png shader.vert )
add_shader( render_to_png shader.frag )

endif()

