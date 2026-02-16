add_library(
  tvs_common
  STATIC
  vulkan_utils.cpp
  dispatch_loader.cpp
  vma.cpp
)

target_link_libraries(
  tvs_common
  Boost::program_options
  Boost::system
  ${pkgcfg_lib_Vulkan_vulkan}
)

subdirs( png sdl null drm png_vb sdl_vb null_vb sdl_direct drm_direct test_drm )

