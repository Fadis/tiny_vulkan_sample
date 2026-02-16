/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <array>
#include <memory>
#include <chrono>
#include <thread>
#include <vulkan/vulkan.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <SDL3/SDL.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tiny_vulkan_sample/vma.hpp>
#include <tiny_vulkan_sample/vulkan_utils.hpp>
#include <tiny_vulkan_sample/drm_utils.hpp>
#include <tiny_vulkan_sample/timer.hpp>

namespace tiny_vulkan_sample {

struct uniform_t {
  glm::mat4 projection_camera_matrix;
  glm::mat4 world_matrix;
  glm::vec4 eye_pos = glm::vec4{ 0.f, -3.f, 6.0f, 1.0f };
  glm::vec4 light_pos = glm::vec4{ 2.0f, -2.0f, 2.0f, 1.0f };
  glm::vec4 base_color = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f );
  float light_energy = 2.0f;
  float ambient = 0.01f;
};

}

int main( int argc, const char *argv[] ) {
  using namespace tiny_vulkan_sample;
  const std::uint32_t width = 320u;
  const std::uint32_t height = 240u;
  auto dumb_buffer = tiny_vulkan_sample::dumb_buffer(
    "/dev/dri/card0",
    width,
    height,
    32u,
    DRM_FORMAT_XRGB8888
  );

  auto ctx = init_vulkan( argc, argv );
  auto instance = std::move( ctx.instance );
  auto physical_device = ctx.physical_device;
  auto device = std::move( ctx.device );
  auto queue = ctx.queue;
  const auto queue_family_index = ctx.queue_family_index;

  auto allocator = create_allocator(
    instance,
    physical_device,
    device
  );

  const std::array< vk::DescriptorPoolSize, 2u > descriptor_pool_size{
    vk::DescriptorPoolSize()
      .setType( vk::DescriptorType::eUniformBuffer )
      .setDescriptorCount( 1 ),
    vk::DescriptorPoolSize()
      .setType( vk::DescriptorType::eStorageBuffer )
      .setDescriptorCount( 1 )
  };

  const auto descriptor_pool = device->createDescriptorPoolUnique(
    vk::DescriptorPoolCreateInfo()
      .setFlags( vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet )
      .setMaxSets( 1 )
      .setPoolSizes( descriptor_pool_size )
  );

  const std::array< vk::DescriptorSetLayoutBinding, 2u > descriptor_set_layout_binding{
    vk::DescriptorSetLayoutBinding()
      .setBinding( 0 )
      .setDescriptorType( vk::DescriptorType::eUniformBuffer )
      .setDescriptorCount( 1 )
      .setStageFlags( vk::ShaderStageFlagBits::eVertex ),
    vk::DescriptorSetLayoutBinding()
      .setBinding( 1 )
      .setDescriptorType( vk::DescriptorType::eStorageBuffer )
      .setDescriptorCount( 1 )
      .setStageFlags( vk::ShaderStageFlagBits::eVertex )
  };

  const auto descriptor_set_layout = device->createDescriptorSetLayoutUnique(
    vk::DescriptorSetLayoutCreateInfo()
      .setBindings( descriptor_set_layout_binding )
  );

  const auto descriptor_set = std::move( device->allocateDescriptorSetsUnique(
    vk::DescriptorSetAllocateInfo()
      .setDescriptorPool( *descriptor_pool )
      .setSetLayouts( { *descriptor_set_layout } )
  )[ 0 ] );

  const auto vertex_shader = create_shader_module(
    device,
    CMAKE_CURRENT_BINARY_DIR "/shader.vert.spv"
  );
  const auto fragment_shader = create_shader_module(
    device,
    CMAKE_CURRENT_BINARY_DIR "/shader.frag.spv"
  );

  auto pipeline_layout = device->createPipelineLayoutUnique(
    vk::PipelineLayoutCreateInfo()
      .setSetLayouts( { *descriptor_set_layout } )
  );

  const auto pipeline = create_pipeline(
    device,
    pipeline_layout,
    width,
    height,
    vertex_shader,
    fragment_shader,
    false
  );

  auto [uniform_buffer,uniform_buffer_allocation] = create_buffer(
    allocator,
    vk::BufferCreateInfo()
      .setSize( sizeof( uniform_t ) )
      .setUsage(
        vk::BufferUsageFlagBits::eUniformBuffer
      )
    ,
    VMA_MEMORY_USAGE_CPU_TO_GPU,
    0
  );

  auto [vertex_buffer,vertex_buffer_allocation,vertex_count] = create_vertex_buffer(
    device,
    allocator,
    ctx.model
  );

  auto framebuffer = create_host_framebuffer(
    device,
    physical_device,
    allocator,
    dumb_buffer.get_buffer(),
    width,
    height
  );
  const auto color = framebuffer.color;
  const auto depth = framebuffer.depth;
  auto color_view = std::move( framebuffer.color_view );
  auto depth_view = std::move( framebuffer.depth_view );
  const auto color_rai = framebuffer.color_rai;
  const auto depth_rai = framebuffer.depth_rai;

  const auto descriptor_buffer_info =
    vk::DescriptorBufferInfo()
      .setBuffer( *uniform_buffer )
      .setOffset( 0 )
      .setRange( sizeof( uniform_t ) );

  const auto descriptor_vertex_buffer_info =
    vk::DescriptorBufferInfo()
      .setBuffer( *vertex_buffer )
      .setOffset( 0 )
      .setRange( vertex_count * 6u * sizeof( float ) );

  device->updateDescriptorSets(
    {
      vk::WriteDescriptorSet()
        .setDstSet( *descriptor_set )
        .setDstBinding( 0 )
        .setDstArrayElement( 0 )
        .setDescriptorCount( 1u )
        .setDescriptorType( vk::DescriptorType::eUniformBuffer )
        .setPBufferInfo( &descriptor_buffer_info ),
      vk::WriteDescriptorSet()
        .setDstSet( *descriptor_set )
        .setDstBinding( 1 )
        .setDstArrayElement( 0 )
        .setDescriptorCount( 1u )
        .setDescriptorType( vk::DescriptorType::eStorageBuffer )
        .setPBufferInfo( &descriptor_vertex_buffer_info )
    },
    {}
  );

  const auto command_pool = device->createCommandPoolUnique(
    vk::CommandPoolCreateInfo()
      .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
      .setQueueFamilyIndex( queue_family_index )
  );
  auto command_buffers = device->allocateCommandBuffersUnique(
    vk::CommandBufferAllocateInfo()
      .setCommandPool( *command_pool )
      .setLevel( vk::CommandBufferLevel::ePrimary )
      .setCommandBufferCount( 1u )
  );
  auto command_buffer = std::move( command_buffers[ 0 ] );
  {
    command_buffer->begin(
      vk::CommandBufferBeginInfo()
        .setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    );
    convert_image(
      command_buffer,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eGeneral,
      vk::ImageAspectFlagBits::eColor,
      queue_family_index,
      color
    );
    
    convert_image(
      command_buffer,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eGeneral,
      vk::ImageAspectFlagBits::eDepth,
      queue_family_index,
      depth
    );
  
    command_buffer->end();
  }
  wait_for_executed( device, queue, command_buffer );

  {
    command_buffer->begin(
      vk::CommandBufferBeginInfo()
    );

    command_buffer->bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      *pipeline_layout,
      0u,
      { *descriptor_set },
      {}
    );
   
    command_buffer->bindPipeline(
      vk::PipelineBindPoint::eGraphics,
      *pipeline
    );

    command_buffer->beginRendering(
      vk::RenderingInfo()
        .setRenderArea(
          vk::Rect2D()
            .setExtent(
              vk::Extent2D()
                .setWidth( width )
                .setHeight( height )
            )
        )
        .setLayerCount( 1u ) 
        .setColorAttachments( { color_rai } )
        .setPDepthAttachment( &depth_rai )
        .setPStencilAttachment( nullptr )
    );

    command_buffer->draw( vertex_count, 1u, 0u, 0u );

    command_buffer->endRendering();
  
    command_buffer->end();
  }

  float angle = 0.0f;
  uniform_t uniform;
  uniform.projection_camera_matrix =
    glm::perspective( 0.39959648408210363f, (float(width)/float(height)), 0.1f, 10.f ) *
    glm::lookAt(
      glm::vec3( uniform.eye_pos ),
      glm::vec3( 0.f, -0.8f, 0.f ),
      glm::vec3{ 0.f, uniform.eye_pos[ 1 ] + 100.f, 0.f }
    );

  timer t;  
  bool exit_flag = false;
  while( !exit_flag ) {
    const auto elapsed = t.begin();
    angle += 1.0f * float( elapsed.count() )/1000000.f;
    uniform.world_matrix =
      glm::mat4(
        std::cos( angle ), 0.f, -std::sin( angle ), 0.f,
        0.f, -1.f, 0.f, 0.f,
        std::sin( angle ), 0.f, std::cos( angle ), 0.f,
        0.f, 0.f, 0.f, 1.f
      );
    {
      void *mapped_memory;
      const auto result = vmaMapMemory( *allocator, *uniform_buffer_allocation, &mapped_memory );
      if( result != VK_SUCCESS ) {
        throw std::runtime_error( "vmaMapMemory failed." );
      }
      std::memcpy(
        mapped_memory,
        reinterpret_cast< void* >( &uniform ),
        sizeof( uniform_t )
      );
      vmaUnmapMemory( *allocator, *uniform_buffer_allocation );
    }
    wait_for_executed( device, queue, command_buffer );
    t.end_rendering();
    dumb_buffer.present();
    t.end_present();
    t.wait_for_vsync();
  }
}

