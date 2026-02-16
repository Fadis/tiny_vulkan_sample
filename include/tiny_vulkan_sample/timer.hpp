#ifndef TINY_VULKAN_SAMPLE_TIMER_HPP
#define TINY_VULKAN_SAMPLE_TIMER_HPP
/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <iostream>
#include <chrono>
#include <thread>

namespace tiny_vulkan_sample {
class timer {
public:
  timer() {
    rendering_time = std::chrono::microseconds( 0 );
    present_time = std::chrono::microseconds( 0 );
    begin_rendering = std::chrono::high_resolution_clock::now();
  }
  std::chrono::microseconds begin() {
    auto old_begin = begin_rendering;
    begin_rendering = std::chrono::high_resolution_clock::now();
    ++accum;
    return std::chrono::duration_cast< std::chrono::microseconds >( begin_rendering - old_begin );
  }
  void end_rendering() {
    const auto end = std::chrono::high_resolution_clock::now();
    rendering_time += std::chrono::duration_cast< std::chrono::microseconds >( end - begin_rendering );
    begin_present = end;
  }
  void end_present() {
    const auto end = std::chrono::high_resolution_clock::now();
    present_time += std::chrono::duration_cast< std::chrono::microseconds >( end - begin_present );
    if( accum == 150u ) {
      std::cout <<
        "rendering : " << float( rendering_time.count() ) / 150.0f << "us " <<
        "present : " << float( present_time.count() ) / 150.0f << "us" << std::endl;
      rendering_time = std::chrono::microseconds( 0 );
      present_time = std::chrono::microseconds( 0 );
      accum = 0u;
    }
  }
  void wait_for_vsync() {
    std::this_thread::sleep_until( begin_rendering + std::chrono::microseconds( 33333 ) );
  }
private:
  std::chrono::high_resolution_clock::time_point begin_rendering;
  std::chrono::high_resolution_clock::time_point begin_present;
  std::chrono::microseconds rendering_time;
  std::chrono::microseconds present_time;
  std::uint32_t accum = 0u;
};

}

#endif

