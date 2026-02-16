/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <cstdint>
#include <chrono>
#include <thread>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <tiny_vulkan_sample/drm_utils.hpp>

int main() {
  std::uint32_t width = 320u;
  std::uint32_t height = 240u;
  auto dumb_buffer = tiny_vulkan_sample::dumb_buffer(
    "/dev/dri/card0",
    width,
    height,
    16u,
    DRM_FORMAT_RGB565
  );
  
  std::uint16_t *pixel = reinterpret_cast< std::uint16_t* >( dumb_buffer.get_buffer() );

  const auto pitch = dumb_buffer.get_pitch();

  for( int t = 0; t != 360; ++t ) {
    const auto begin_date = std::chrono::high_resolution_clock::now();
    auto cur = pixel;
    for( std::uint32_t y = 0u; y < height; ++y ) {
      for( std::uint32_t x = 0u; x < width; ++x ) {
        pixel[ y * pitch/sizeof( std::uint16_t ) + x ] =
	  ( ( ( x + t ) & 0x1F ) << 11 ) |
	  ( ( ( y + t ) & 0x3F ) << 5 );
      }
    }
    dumb_buffer.present();
    std::this_thread::sleep_until( begin_date + std::chrono::microseconds( 33333 ) );
  }
}

