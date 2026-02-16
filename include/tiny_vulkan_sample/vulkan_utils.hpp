#ifndef TINY_VULKAN_SAMPLE_VULKAN_UTILS_HPP
#define TINY_VULKAN_SAMPLE_VULKAN_UTILS_HPP
/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <memory>
#include <vulkan/vulkan.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#include <tiny_vulkan_sample/vk_mem_alloc.h>
#pragma clang diagnostic pop

namespace tiny_vulkan_sample {

struct vulkan_context {
  vk::UniqueHandle<vk::Instance, vk::detail::DispatchLoaderDynamic> instance;
  vk::PhysicalDevice physical_device;
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> device;
  vk::Queue queue;
  std::uint32_t queue_family_index = 0u;
  std::string model;
};

struct vulkan_framebuffer {
  std::shared_ptr< vk::Image > color;
  std::shared_ptr< vk::Image > depth;
  vk::UniqueHandle<vk::ImageView, vk::detail::DispatchLoaderDynamic> color_view;
  vk::UniqueHandle<vk::ImageView, vk::detail::DispatchLoaderDynamic> depth_view;
  vk::RenderingAttachmentInfo color_rai;
  vk::RenderingAttachmentInfo depth_rai;
};

vulkan_context init_vulkan(
  int argc,
  const char *argv[]
);

std::shared_ptr< VmaAllocator > create_allocator(
  vk::UniqueHandle<vk::Instance, vk::detail::DispatchLoaderDynamic> &instance,
  vk::PhysicalDevice &physical_device,
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device
);

std::tuple< std::shared_ptr< vk::Buffer >, std::shared_ptr< VmaAllocation > >  create_buffer(
  const std::shared_ptr< VmaAllocator > &allocator,
  const vk::BufferCreateInfo &ci,
  VmaMemoryUsage usage,
  VmaAllocationCreateFlags flags
);

std::size_t align_to( std::size_t v, std::size_t align );

std::shared_ptr< vk::Buffer >
create_host_buffer(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< void > &head,
  const vk::BufferCreateInfo &ci
);

std::shared_ptr< vk::Buffer >
create_host_buffer(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  void *head,
  const vk::BufferCreateInfo &ci
);

std::shared_ptr< vk::Image > create_image(
  const std::shared_ptr< VmaAllocator > &allocator,
  const vk::ImageCreateInfo &ci,
  VmaMemoryUsage usage,
  VmaAllocationCreateFlags flags
);

std::shared_ptr< vk::Image >
create_host_image(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< void > &head,
  const vk::ImageCreateInfo &ci
);

std::shared_ptr< vk::Image >
create_host_image(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  void *head,
  const vk::ImageCreateInfo &ci
);

vk::UniqueHandle< vk::ShaderModule, vk::detail::DispatchLoaderDynamic> 
create_shader_module(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const std::string &filename
);

vk::UniqueHandle< vk::Pipeline, vk::detail::DispatchLoaderDynamic >
create_pipeline(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const vk::UniqueHandle< vk::PipelineLayout, vk::detail::DispatchLoaderDynamic > &pipeline_layout,
  std::uint32_t width,
  std::uint32_t height,
  const vk::UniqueHandle< vk::ShaderModule, vk::detail::DispatchLoaderDynamic > &vertex_shader,
  const vk::UniqueHandle< vk::ShaderModule, vk::detail::DispatchLoaderDynamic > &fragment_shader,
  bool use_vertex_buffer
);

void convert_image(
  const vk::UniqueHandle< vk::CommandBuffer, vk::detail::DispatchLoaderDynamic> &command_buffer,
  vk::ImageLayout from,
  vk::ImageLayout to,
  vk::ImageAspectFlags aspect,
  std::uint32_t queue_family_index,
  const std::shared_ptr< vk::Image > &image
);

void sync_buffer(
  const vk::UniqueHandle< vk::CommandBuffer, vk::detail::DispatchLoaderDynamic> &command_buffer,
  std::uint32_t queue_family_index,
  const std::shared_ptr< vk::Buffer > &buffer,
  vk::DeviceSize offset,
  vk::DeviceSize size
);

std::shared_ptr< void > map_buffer(
  const std::shared_ptr< VmaAllocator > &allocator,
  const std::shared_ptr< VmaAllocation > &allocation
);

std::tuple< std::shared_ptr< vk::Buffer >, std::shared_ptr< VmaAllocation >, std::size_t >
create_vertex_buffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const std::shared_ptr< VmaAllocator > &allocator,
  const std::string &filename
);

vulkan_framebuffer
create_framebuffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const std::shared_ptr< VmaAllocator > &allocator,
  unsigned int width,
  unsigned int height
);

vulkan_framebuffer
create_host_framebuffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< VmaAllocator > &allocator,
  const std::shared_ptr< void > &head,
  unsigned int width,
  unsigned int height
);

vulkan_framebuffer
create_host_framebuffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< VmaAllocator > &allocator,
  void *head,
  unsigned int width,
  unsigned int height
);

void wait_for_executed(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::Queue &queue,
  vk::UniqueHandle< vk::CommandBuffer, vk::detail::DispatchLoaderDynamic> &command_buffer
);

}

#endif

