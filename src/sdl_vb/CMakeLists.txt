if( SDL3_FOUND )

add_executable( render_to_sdl_vb
  render_to_sdl.cpp
)

target_link_libraries(
  render_to_sdl_vb
  tvs_common
  Boost::program_options
  Boost::system
  SDL3::SDL3
  ${pkgcfg_lib_Vulkan_vulkan}
)

target_compile_definitions( render_to_sdl_vb PRIVATE -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" )
target_compile_definitions( render_to_sdl_vb PRIVATE -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )

add_shader( render_to_sdl_vb shader.vert )
add_shader( render_to_sdl_vb shader.frag )

endif()

