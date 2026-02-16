/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <cstddef>
#include <array>
#include <memory>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <tuple>
#include <filesystem>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#include <tiny_vulkan_sample/vk_mem_alloc.h>
#pragma clang diagnostic pop
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tiny_vulkan_sample/vulkan_utils.hpp>

namespace tiny_vulkan_sample {

vulkan_context init_vulkan(
  int argc,
  const char *argv[]
) {
  namespace po = boost::program_options;
  po::options_description desc( "Options" );
  desc.add_options()
    ( "help,h", "show this message" )
    ( "validation,v", po::bool_switch(), "enable validation layer" )
    ( "model,m", po::value< std::string >()->default_value( "vertex.bin" ), "scene data" )
    ( "device,d", po::value< unsigned int >()->default_value( 0 ), "device index" );
  po::variables_map vm;
  po::store( po::parse_command_line( argc, argv, desc ), vm );
  po::notify( vm );
  if( vm.count( "help" ) ) {
    std::cout << desc << std::endl;
    std::exit( 0 );
  }
#ifdef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
  // Vulkan-HppからvkCreateInstanceを呼べるようにする
#if VK_HEADER_VERSION >= 301
  vk::detail::DynamicLoader dl;
#else
  vk::DynamicLoader dl;
#endif
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
  dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
  VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );
#endif
  const auto app_info = vk::ApplicationInfo()
    // アプリケーションの名前
    .setPApplicationName( argc ? argv[ 0 ] : "my_application" )
    // アプリケーションのバージョン
    .setApplicationVersion( VK_MAKE_VERSION(1, 0, 0) )
    // エンジンの名前
    .setPEngineName( "my_engine" )
    // エンジンのバージョン
    .setEngineVersion( VK_MAKE_VERSION(1, 0, 0) )
    // 使用するVulkanのバージョンをVulkan 1.2にする
    .setApiVersion( VK_API_VERSION_1_4 );
  // バリデーションレイヤーを使う
  std::vector< const char * > layers;
  if( vm[ "validation" ].as< bool >() ) {
    layers.push_back( "VK_LAYER_KHRONOS_validation" );
  }
  auto instance = vk::createInstanceUnique(
    vk::InstanceCreateInfo()
      // アプリケーションの情報を指定
      .setPApplicationInfo( &app_info )
      // 使用するレイヤーを指定
      .setPEnabledLayerNames( layers )
  );
#ifdef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
  // Vulkan-Hppからこのインスタンスで利用可能な全ての関数を呼べるようにする
  VULKAN_HPP_DEFAULT_DISPATCHER.init( *instance );
#endif
  // インスタンスがサポートするVulkanのバージョンを取得
  auto version = vk::enumerateInstanceVersion();
  std::cout <<
    "Vulkan instance API version : " <<
    VK_VERSION_MAJOR( version ) << "." <<
    VK_VERSION_MINOR( version ) << "." <<
    VK_VERSION_PATCH( version ) << std::endl;


  auto physical_devices = instance->enumeratePhysicalDevices();

  // デバイスが1つも見つからなかったらabort
  if( physical_devices.empty() ) throw std::runtime_error( "No devices are available." );

  // 1つ目のGPUを使う
  const auto physical_device = physical_devices[ vm[ "device" ].as< unsigned int >() ];

  auto external_memory_host_properties =
    vk::PhysicalDeviceExternalMemoryHostPropertiesEXT();

  auto physical_device_properties =
    vk::PhysicalDeviceProperties2();
  physical_device_properties.pNext = &external_memory_host_properties;

  physical_device.getProperties2(
    &physical_device_properties
  );

  std::cout << "Vulkan device name : " << physical_device_properties.properties.deviceName << std::endl;
  std::cout <<
    "Vulkan device API version : " <<
    VK_VERSION_MAJOR( physical_device_properties.properties.apiVersion ) << "." <<
    VK_VERSION_MINOR( physical_device_properties.properties.apiVersion ) << "." <<
    VK_VERSION_PATCH( physical_device_properties.properties.apiVersion ) << std::endl;

  std::cout << "external_memory_host_properties.minImportedHostPointerAlignment : " << external_memory_host_properties.minImportedHostPointerAlignment << std::endl;

  // デバイスに備わっているキューを取得
  const auto queue_props = physical_device.getQueueFamilyProperties();

  uint32_t queue_family_index = 0u;
  // 描画要求を受け付けるキューを探す
  for( uint32_t i = 0; i < queue_props.size(); ++i ) {
    if( queue_props[ i ].queueFlags & vk::QueueFlagBits::eGraphics ) {
      queue_family_index = i;
      break;
    }
  }

  const float priority = 0.0f;
  // 描画要求を受け付けるキューを1つください
  std::vector< vk::DeviceQueueCreateInfo > queues{
    vk::DeviceQueueCreateInfo()
      .setQueueFamilyIndex( queue_family_index )
      .setQueueCount( 1 )
      .setPQueuePriorities( &priority )
  };

  const auto dynamic_rendering_feature =
    vk::PhysicalDeviceDynamicRenderingFeatures()
      .setDynamicRendering( true );

  const std::array< const char*, 1u > device_extension{
    VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME
  };

  auto device = physical_device.createDeviceUnique(
    vk::DeviceCreateInfo()
      .setPNext( &dynamic_rendering_feature )
      .setQueueCreateInfos( queues )
      .setPEnabledExtensionNames( device_extension )
  );

  auto queue = device->getQueue( queue_family_index, 0u );
  return vulkan_context {
    std::move( instance ),
    physical_device,
    std::move( device ),
    queue,
    queue_family_index,
    vm[ "model" ].as< std::string >()
  };
}

std::shared_ptr< VmaAllocator > create_allocator(
  vk::UniqueHandle<vk::Instance, vk::detail::DispatchLoaderDynamic> &instance,
  vk::PhysicalDevice &physical_device,
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device
) {
  std::shared_ptr< VmaAllocator > allocator;
  VmaAllocatorCreateInfo allocator_create_info;
  allocator_create_info.instance = *instance;
  allocator_create_info.physicalDevice = &*physical_device;
  allocator_create_info.device = *device;
  allocator_create_info.flags = 0u;
  allocator_create_info.preferredLargeHeapBlockSize = 0u;
  allocator_create_info.pAllocationCallbacks = nullptr;
  allocator_create_info.pDeviceMemoryCallbacks = nullptr;
  allocator_create_info.pHeapSizeLimit = nullptr;
  allocator_create_info.pVulkanFunctions = nullptr;
  allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_4;
  allocator_create_info.pTypeExternalMemoryHandleTypes = nullptr;
  VmaAllocator raw_allocator;
  if( vmaCreateAllocator(
    &allocator_create_info, &raw_allocator
  ) != VK_SUCCESS ) {
    throw std::runtime_error( "vmaCreateAllocator failed." );
  }
  allocator.reset(
    new VmaAllocator( raw_allocator ),
    []( VmaAllocator *p ) {
      if( p ) {
        vmaDestroyAllocator( *p );
        delete p;
      }
    }
  );
  return allocator;
}  

std::tuple< std::shared_ptr< vk::Buffer >, std::shared_ptr< VmaAllocation > >  create_buffer(
  const std::shared_ptr< VmaAllocator > &allocator,
  const vk::BufferCreateInfo &ci,
  VmaMemoryUsage usage,
  VmaAllocationCreateFlags flags
) {
  VmaAllocationCreateInfo buffer_alloc_info = {};
  VkBufferCreateInfo raw_buffer_create_info = static_cast< VkBufferCreateInfo >( ci );
  buffer_alloc_info.flags = flags;
  buffer_alloc_info.usage = usage;
  std::shared_ptr< VmaAllocation > allocation( new VmaAllocation() );
  VkBuffer buffer_;
  const auto result = vmaCreateBuffer( *allocator, &raw_buffer_create_info, &buffer_alloc_info, &buffer_, allocation.get(), nullptr );
  if( result != VK_SUCCESS ) {
#if VK_HEADER_VERSION >= 256
    vk::detail::throwResultException( vk::Result( result ), "vmaCreateBuffer failed." );
#else
    vk::throwResultException( vk::Result( result ), "vmaCreateBuffer failed." );
#endif
  }
  std::shared_ptr< vk::Buffer > handle;
  handle.reset(
    new vk::Buffer( buffer_ ),
    [allocator=allocator,allocation=allocation]( vk::Buffer *p ) {
      if( p ) {
        vmaDestroyBuffer( *allocator, VkBuffer( *p ), *allocation );
        delete p;
      }
    }
  );
  return std::make_tuple( handle, allocation );
}

std::size_t align_to( std::size_t v, std::size_t align ) {
  return ( v / align ) * align + ( ( v % align ) ? align : 0u );
}

std::shared_ptr< vk::Buffer >
create_host_buffer(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< void > &head,
  const vk::BufferCreateInfo &ci
) {
  auto props = ci;

  const auto external_memory_buffer_create_info =
    vk::ExternalMemoryBufferCreateInfo()
      .setHandleTypes( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT );

  props.setPNext( &external_memory_buffer_create_info );

  auto buffer = device->createBuffer(
    props,
    nullptr
  );

  const auto buffer_memory_requirement = device->getBufferMemoryRequirements(
    buffer
  );

  std::cout << "create_host_buffer size : " << buffer_memory_requirement.size << " alignment : " << buffer_memory_requirement.alignment << std::endl;

  const auto host_pointer_properties = device->getMemoryHostPointerPropertiesEXT(
    vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
    head.get()
  );

  const auto host_pointer_info =
    vk::ImportMemoryHostPointerInfoEXT()
      .setHandleType( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT )
      .setPHostPointer( head.get() );

  auto memory = device->allocateMemory(
    vk::MemoryAllocateInfo()
      .setPNext( &host_pointer_info )
      .setAllocationSize( buffer_memory_requirement.size )
      .setMemoryTypeIndex( std::countr_zero( host_pointer_properties.memoryTypeBits & buffer_memory_requirement.memoryTypeBits ) ),
    nullptr
  );

  device->bindBufferMemory(
    buffer,
    memory,
    0u
  );

  std::shared_ptr< vk::Buffer > handle;
  handle.reset(
    new vk::Buffer( buffer ),
    [device=*device,head=head,memory=memory]( vk::Buffer *p ) {
      if( p ) {
        device.destroyBuffer( *p );
        device.freeMemory( memory );
        delete p;
      }
    }
  );
  return handle;
}

std::shared_ptr< vk::Buffer >
create_host_buffer(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  void *head,
  const vk::BufferCreateInfo &ci
) {
  auto props = ci;

  const auto external_memory_buffer_create_info =
    vk::ExternalMemoryBufferCreateInfo()
      .setHandleTypes( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT );

  props.setPNext( &external_memory_buffer_create_info );

  auto buffer = device->createBuffer(
    props,
    nullptr
  );

  const auto buffer_memory_requirement = device->getBufferMemoryRequirements(
    buffer
  );

  std::cout << "create_host_buffer size : " << buffer_memory_requirement.size << " alignment : " << buffer_memory_requirement.alignment << std::endl;

  const auto host_pointer_properties = device->getMemoryHostPointerPropertiesEXT(
    vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
    head
  );

  const auto host_pointer_info =
    vk::ImportMemoryHostPointerInfoEXT()
      .setHandleType( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT )
      .setPHostPointer( head );

  auto memory = device->allocateMemory(
    vk::MemoryAllocateInfo()
      .setPNext( &host_pointer_info )
      .setAllocationSize( buffer_memory_requirement.size )
      .setMemoryTypeIndex( std::countr_zero( host_pointer_properties.memoryTypeBits & buffer_memory_requirement.memoryTypeBits ) ),
    nullptr
  );

  device->bindBufferMemory(
    buffer,
    memory,
    0u
  );

  std::shared_ptr< vk::Buffer > handle;
  handle.reset(
    new vk::Buffer( buffer ),
    [device=*device,memory=memory]( vk::Buffer *p ) {
      if( p ) {
        device.destroyBuffer( *p );
        device.freeMemory( memory );
        delete p;
      }
    }
  );
  return handle;
}

std::shared_ptr< vk::Image > create_image(
  const std::shared_ptr< VmaAllocator > &allocator,
  const vk::ImageCreateInfo &ci,
  VmaMemoryUsage usage,
  VmaAllocationCreateFlags flags
) {
  VmaAllocationCreateInfo image_alloc_info = {};
  image_alloc_info.flags = flags;
  image_alloc_info.usage = usage;
  VkImageCreateInfo image_create_info = static_cast< VkImageCreateInfo >( ci );
  VkImage image;
  std::shared_ptr< VmaAllocation > allocation( new VmaAllocation() );
  const auto result = vmaCreateImage(
    *allocator,
    &image_create_info,
    &image_alloc_info,
    &image,
    allocation.get(),
    nullptr
  );
  if( result != VK_SUCCESS ) {
#if VK_HEADER_VERSION >= 256
    vk::detail::throwResultException( vk::Result( result ), "vmaCreateImage failed." );
#else
    vk::throwResultException( vk::Result( result ), "vmaCreateImage failed." );
#endif
  }
  std::shared_ptr< vk::Image > handle;
  handle.reset(
    new vk::Image( image ),
    [allocator=allocator,allocation=allocation]( vk::Image *p ) {
      if( p ) {
        vmaDestroyImage( *allocator, VkImage( *p ), *allocation );
        delete p;
      }
    }
  );
  return handle;
}

std::shared_ptr< vk::Image >
create_host_image(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< void > &head,
  const vk::ImageCreateInfo &ci
) {
  auto props = ci;

  const auto external_memory_buffer_create_info =
    vk::ExternalMemoryImageCreateInfo()
      .setHandleTypes( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT );

  props.setPNext( &external_memory_buffer_create_info );

  auto image = device->createImage(
    props,
    nullptr
  );

  const auto image_memory_requirement = device->getImageMemoryRequirements(
    image
  );

  std::cout << "create_host_image size : " << image_memory_requirement.size << " alignment : " << image_memory_requirement.alignment << std::endl;

  const auto host_pointer_properties = device->getMemoryHostPointerPropertiesEXT(
    vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
    head.get()
  );

  const auto host_pointer_info =
    vk::ImportMemoryHostPointerInfoEXT()
      .setHandleType( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT )
      .setPHostPointer( head.get() );

  auto memory = device->allocateMemory(
    vk::MemoryAllocateInfo()
      .setPNext( &host_pointer_info )
      .setAllocationSize( image_memory_requirement.size )
      .setMemoryTypeIndex( std::countr_zero( host_pointer_properties.memoryTypeBits & image_memory_requirement.memoryTypeBits ) ),
    nullptr
  );

  device->bindImageMemory(
    image,
    memory,
    0u
  );

  std::shared_ptr< vk::Image > handle;
  handle.reset(
    new vk::Image( image ),
    [device=*device,head=head,memory=memory]( vk::Image *p ) {
      if( p ) {
        device.destroyImage( *p );
        device.freeMemory( memory );
        delete p;
      }
    }
  );
  return handle;
}

std::shared_ptr< vk::Image >
create_host_image(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  void *head,
  const vk::ImageCreateInfo &ci
) {
  auto props = ci;

  const auto external_memory_buffer_create_info =
    vk::ExternalMemoryImageCreateInfo()
      .setHandleTypes( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT );

  props.setPNext( &external_memory_buffer_create_info );

  auto image = device->createImage(
    props,
    nullptr
  );

  const auto image_memory_requirement = device->getImageMemoryRequirements(
    image
  );

  std::cout << "create_host_image size : " << image_memory_requirement.size << " alignment : " << image_memory_requirement.alignment << std::endl;

  const auto host_pointer_properties = device->getMemoryHostPointerPropertiesEXT(
    vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
    head
  );

  const auto host_pointer_info =
    vk::ImportMemoryHostPointerInfoEXT()
      .setHandleType( vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT )
      .setPHostPointer( head );

  auto memory = device->allocateMemory(
    vk::MemoryAllocateInfo()
      .setPNext( &host_pointer_info )
      .setAllocationSize( image_memory_requirement.size )
      .setMemoryTypeIndex( std::countr_zero( host_pointer_properties.memoryTypeBits & image_memory_requirement.memoryTypeBits ) ),
    nullptr
  );

  device->bindImageMemory(
    image,
    memory,
    0u
  );

  std::shared_ptr< vk::Image > handle;
  handle.reset(
    new vk::Image( image ),
    [device=*device,memory=memory]( vk::Image *p ) {
      if( p ) {
        device.destroyImage( *p );
        device.freeMemory( memory );
        delete p;
      }
    }
  );
  return handle;
}


vk::UniqueHandle< vk::ShaderModule, vk::detail::DispatchLoaderDynamic> 
create_shader_module(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const std::string &filename
) {
  // ファイルからSPIR-Vを読む
  std::fstream file( filename, std::ios::in|std::ios::binary );
  if( !file.good() ) abort();
  std::vector< std::uint8_t > code;
  code.assign(
    std::istreambuf_iterator< char >( file ),
    std::istreambuf_iterator< char >()
  );

  // シェーダモジュールを作る
  return device->createShaderModuleUnique(
    vk::ShaderModuleCreateInfo()
      .setCodeSize( code.size() )
      .setPCode( reinterpret_cast< const uint32_t* >( code.data() ) )
  );
}

vk::UniqueHandle< vk::Pipeline, vk::detail::DispatchLoaderDynamic >
create_pipeline(
  vk::UniqueHandle< vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const vk::UniqueHandle< vk::PipelineLayout, vk::detail::DispatchLoaderDynamic > &pipeline_layout,
  std::uint32_t width,
  std::uint32_t height,
  const vk::UniqueHandle< vk::ShaderModule, vk::detail::DispatchLoaderDynamic > &vertex_shader,
  const vk::UniqueHandle< vk::ShaderModule, vk::detail::DispatchLoaderDynamic > &fragment_shader,
  bool use_vertex_buffer
) {
  const std::array< vk::PipelineShaderStageCreateInfo, 2u > shader{
    vk::PipelineShaderStageCreateInfo()
      .setStage( vk::ShaderStageFlagBits::eVertex )
      .setModule( *vertex_shader )
      .setPName( "main" ),
    vk::PipelineShaderStageCreateInfo()
      .setStage( vk::ShaderStageFlagBits::eFragment )
      .setModule( *fragment_shader )
      .setPName( "main" )
  };
  
  // 頂点配列の読み方
  const std::array< vk::VertexInputBindingDescription, 1u > vib{
    vk::VertexInputBindingDescription()
      // 頂点配列binding 0番は
      .setBinding( 0 )
      // 頂点1個毎に
      .setInputRate( vk::VertexInputRate::eVertex )
      // 12バイト移動しながら読む
      .setStride( sizeof( float ) * 6 )
  };

  const std::array< vk::VertexInputAttributeDescription, 2u > via{
    vk::VertexInputAttributeDescription()
      .setLocation( 0 )
      .setFormat( vk::Format::eR32G32B32Sfloat )
      .setBinding( 0 )
      .setOffset( 0 ),
    vk::VertexInputAttributeDescription()
      .setLocation( 1 )
      .setFormat( vk::Format::eR32G32B32Sfloat )
      .setBinding( 0 )
      .setOffset( sizeof( float ) * 3 )
  };

  auto vistat =
    vk::PipelineVertexInputStateCreateInfo();
  if( use_vertex_buffer ) {
    vistat
      .setVertexBindingDescriptions( vib )
      .setVertexAttributeDescriptions( via );
  }

  // プリミティブの組み立て方
  const auto input_assembly =
    vk::PipelineInputAssemblyStateCreateInfo()
      // 頂点配列の要素3個毎に1つの三角形
      .setTopology( vk::PrimitiveTopology::eTriangleList );

  const std::array< vk::Viewport, 1u > viewport_{
    vk::Viewport()
      .setWidth( width )
      .setHeight( height )
      .setMinDepth( 0.0f )
      .setMaxDepth( 1.0f )
  };

  const std::array< vk::Rect2D, 1u > scissor{
    vk::Rect2D()
      .setOffset( { 0, 0 } )
      .setExtent( { width, height } )
  };

  // ビューポートとシザーの設定
  const auto viewport =
    vk::PipelineViewportStateCreateInfo()
      // 1個のビューポートと
      .setViewports( viewport_ )
      // 1個のシザーを使う
      .setScissors( scissor );
      // 具体的な値は後でDynamicStateを使って設定する

  // ラスタライズの設定
  const auto rasterization =
    vk::PipelineRasterizationStateCreateInfo()
      // 範囲外の深度を丸めない
      .setDepthClampEnable( false )
      // ラスタライズを行う
      .setRasterizerDiscardEnable( false )
      // 三角形の中を塗る
      .setPolygonMode( vk::PolygonMode::eFill )
      // 背面カリングを行わない
      .setCullMode( vk::CullModeFlagBits::eFront )
      // 表面は時計回り
      .setFrontFace( vk::FrontFace::eClockwise )
      // 深度バイアスを使わない
      .setDepthBiasEnable( false )
      // 線を描く時は太さ1.0で
      .setLineWidth( 1.0f );

  // マルチサンプルの設定
  const auto multisample =
    // 全部デフォルト(マルチサンプルを使わない)
    vk::PipelineMultisampleStateCreateInfo();

  // 深度とステンシルの設定
  const auto depth_stencil =
    vk::PipelineDepthStencilStateCreateInfo()
      // 深度テストをする
      .setDepthTestEnable( true )
      // 深度値を深度バッファに書く
      .setDepthWriteEnable( true )
      // 深度値がより小さい場合手前と見做す
      .setDepthCompareOp( vk::CompareOp::eLessOrEqual )
      // 深度の範囲を制限しない
      .setDepthBoundsTestEnable( false )
      // ステンシルテストをしない
      .setStencilTestEnable( false );

  // カラーブレンドの設定
  const auto color_blend_attachment =
    vk::PipelineColorBlendAttachmentState()
      // フレームバッファに既にある色と新しい色を混ぜない
      // (新しい色で上書きする)
      .setBlendEnable( false )
      // RGBA全ての要素を書く
      .setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
      );
  const auto color_blend =
    vk::PipelineColorBlendStateCreateInfo()
      // 論理演算をしない
      .setLogicOpEnable( false )
      // 論理演算をする場合clearを使う
      .setLogicOp( vk::LogicOp::eClear )
      // カラーアタッチメント1つ分の設定がある
      .setAttachmentCount( 1 )
      // このブレンドの設定を0番目のカラーアタッチメントで使う
      .setPAttachments( &color_blend_attachment )
      // カラーブレンドに使う定数
      .setBlendConstants( { 0.f, 0.f, 0.f, 0.f } );

  // 後から変更できるパラメータの設定
  const auto dynamic =
    vk::PipelineDynamicStateCreateInfo();

  // テッセレーションの設定
  const auto tessellation =
    // 全部デフォルト(テッセレーションを使わない)
    vk::PipelineTessellationStateCreateInfo();

  const std::array< vk::Format, 1u > color_attachment_format{
    //vk::Format::eR5G6B5UnormPack16
    vk::Format::eB8G8R8A8Unorm
  };

  const auto rendering =
    vk::PipelineRenderingCreateInfo()
      .setColorAttachmentFormats( color_attachment_format )
      .setDepthAttachmentFormat( vk::Format::eD16Unorm )
      .setStencilAttachmentFormat( vk::Format::eUndefined );

  const auto create_info = 
    vk::GraphicsPipelineCreateInfo()
      .setPNext( &rendering )
      .setStages( shader )
      .setPVertexInputState( &vistat )
      .setPInputAssemblyState( &input_assembly )
      .setPTessellationState( &tessellation )
      .setPViewportState( &viewport )
      .setPRasterizationState( &rasterization )
      .setPMultisampleState( &multisample )
      .setPDepthStencilState( &depth_stencil )
      .setPColorBlendState( &color_blend )
      .setPDynamicState( &dynamic )
      .setLayout( *pipeline_layout )
      // このパイプラインレイアウトで
      .setRenderPass( nullptr )
      // このレンダーパスの0番目のサブパスとして使う
      .setSubpass( 0 );

  // グラフィクスパイプラインを作る
  auto wrapped = device->createGraphicsPipelineUnique(
    nullptr,
    create_info,
    nullptr
  );
  if( wrapped.result != vk::Result::eSuccess ) {
#if VK_HEADER_VERSION >= 256
    vk::detail::throwResultException( wrapped.result, "createGraphicsPipeline failed." );
#else
    vk::throwResultException( wrapped.result, "createGraphicsPipeline failed." );
#endif
  }
  return std::move( wrapped.value );
}

void convert_image(
  const vk::UniqueHandle< vk::CommandBuffer, vk::detail::DispatchLoaderDynamic> &command_buffer,
  vk::ImageLayout from,
  vk::ImageLayout to,
  vk::ImageAspectFlags aspect,
  std::uint32_t queue_family_index,
  const std::shared_ptr< vk::Image > &image
) {
  command_buffer->pipelineBarrier(
    vk::PipelineStageFlagBits::eAllGraphics|
    vk::PipelineStageFlagBits::eTransfer,
    vk::PipelineStageFlagBits::eAllGraphics|
    vk::PipelineStageFlagBits::eTransfer,
    vk::DependencyFlagBits( 0 ),
    {},
    {},
    {
      vk::ImageMemoryBarrier()
        .setSrcAccessMask(
          vk::AccessFlagBits::eMemoryRead |
          vk::AccessFlagBits::eMemoryWrite
        )
        .setDstAccessMask(
          vk::AccessFlagBits::eMemoryRead |
          vk::AccessFlagBits::eMemoryWrite
        )
        .setOldLayout( from )
        .setNewLayout( to )
        .setSrcQueueFamilyIndex( queue_family_index )
        .setDstQueueFamilyIndex( queue_family_index )
        .setImage( *image )
        .setSubresourceRange(
          vk::ImageSubresourceRange()
            .setAspectMask( aspect )
            .setBaseMipLevel( 0u )
            .setLevelCount( 1u )
            .setBaseArrayLayer( 0u )
            .setLayerCount( 1u )
        )
    }
  );
}

void sync_buffer(
  const vk::UniqueHandle< vk::CommandBuffer, vk::detail::DispatchLoaderDynamic> &command_buffer,
  std::uint32_t queue_family_index,
  const std::shared_ptr< vk::Buffer > &buffer,
  vk::DeviceSize offset,
  vk::DeviceSize size
) {
  command_buffer->pipelineBarrier(
    vk::PipelineStageFlagBits::eAllGraphics|
    vk::PipelineStageFlagBits::eTransfer,
    vk::PipelineStageFlagBits::eAllGraphics|
    vk::PipelineStageFlagBits::eTransfer,
    vk::DependencyFlagBits( 0 ),
    {},
    {
      vk::BufferMemoryBarrier()
        .setSrcAccessMask(
          vk::AccessFlagBits::eMemoryRead |
          vk::AccessFlagBits::eMemoryWrite
        )
        .setDstAccessMask(
          vk::AccessFlagBits::eMemoryRead |
          vk::AccessFlagBits::eMemoryWrite
        )
        .setSrcQueueFamilyIndex( queue_family_index )
        .setDstQueueFamilyIndex( queue_family_index )
        .setBuffer( *buffer )
        .setOffset( offset )
        .setSize( size )
    },
    {}
  );
}

std::shared_ptr< void > map_buffer(
  const std::shared_ptr< VmaAllocator > &allocator,
  const std::shared_ptr< VmaAllocation > &allocation
) {
  void* mapped_memory;
  const auto result = vmaMapMemory( *allocator, *allocation, &mapped_memory );
  if( result != VK_SUCCESS ) {
#if VK_HEADER_VERSION >= 256
    vk::detail::throwResultException( vk::Result( result ), "vmaMapMemory failed." );
#else
    vk::throwResultException( vk::Result( result ), "vmaMapMemory failed." );
#endif
  }
  std::shared_ptr< void > handle{
    mapped_memory,
    [allocator=allocator,allocation=allocation]( void *p ) {
      if( p ) {
        vmaUnmapMemory( *allocator, *allocation );
      }
    }
  };
  return handle;
}

std::tuple< std::shared_ptr< vk::Buffer >, std::shared_ptr< VmaAllocation >, std::size_t >
create_vertex_buffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const std::shared_ptr< VmaAllocator > &allocator,
  const std::string &filename
) {
  std::shared_ptr< void > raw_vertex_buffer;
  const std::size_t vertex_buffer_size = std::filesystem::file_size( filename );
  const std::size_t vertex_count = vertex_buffer_size / ( sizeof( float ) * 6u );
  const std::size_t aligned_vertex_buffer_size = align_to( vertex_buffer_size, 4096 );
  auto [vertex_buffer,allocation] = create_buffer(
    allocator,
    vk::BufferCreateInfo()
      .setSize( aligned_vertex_buffer_size )
      .setUsage(
        vk::BufferUsageFlagBits::eVertexBuffer|
        vk::BufferUsageFlagBits::eStorageBuffer
      ),
    VMA_MEMORY_USAGE_CPU_TO_GPU,
    0u
  );
  std::fstream fd( filename, std::ios::binary|std::ios::in );
  {
    const auto mapped = map_buffer( allocator, allocation );
    std::copy(
      std::istreambuf_iterator< char >( fd ),
      std::istreambuf_iterator< char >(),
      reinterpret_cast< char* >( mapped.get() )
    );
  }
  return std::make_tuple( vertex_buffer, allocation, vertex_count );
}

vulkan_framebuffer
create_framebuffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  const std::shared_ptr< VmaAllocator > &allocator,
  unsigned int width,
  unsigned int height
) {
  auto color = create_image(
    allocator,
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      //.setFormat( vk::Format::eR5G6B5UnormPack16 )
      .setFormat( vk::Format::eB8G8R8A8Unorm )
      .setExtent( { width, height, 1u } )
      .setMipLevels( 1 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eOptimal )
      .setUsage(
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eColorAttachment
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined ),
    VMA_MEMORY_USAGE_GPU_ONLY,
    0
  );

  auto color_view = device->createImageViewUnique(
    vk::ImageViewCreateInfo()
      .setImage( *color )
      .setViewType( vk::ImageViewType::e2D )
      //.setFormat( vk::Format::eR5G6B5UnormPack16 )
      .setFormat( vk::Format::eB8G8R8A8Unorm )
      .setSubresourceRange(
        vk::ImageSubresourceRange()
          .setAspectMask( vk::ImageAspectFlagBits::eColor )
          .setBaseMipLevel( 0u )
          .setLevelCount( 1u )
          .setBaseArrayLayer( 0u )
          .setLayerCount( 1u )
      )
  );

  const auto color_rai = vk::RenderingAttachmentInfo()
    .setImageView( *color_view )
    .setImageLayout( vk::ImageLayout::eGeneral )
    .setResolveMode( vk::ResolveModeFlagBits::eNone )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue(
      vk::ClearValue()
        .setColor(
          vk::ClearColorValue()
            .setFloat32( { 0.0f, 0.0f, 1.0f, 1.0f } )
        )
    );

  auto depth = create_image(
    allocator,
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      .setFormat( vk::Format::eD16Unorm )
      .setExtent( { width, height, 1u } )
      .setMipLevels( 1 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eOptimal )
      .setUsage(
        vk::ImageUsageFlagBits::eDepthStencilAttachment
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined ),
    VMA_MEMORY_USAGE_GPU_ONLY,
    0
  );
  
  auto depth_view = device->createImageViewUnique(
    vk::ImageViewCreateInfo()
      .setImage( *depth )
      .setViewType( vk::ImageViewType::e2D )
      .setFormat( vk::Format::eD16Unorm )
      .setSubresourceRange(
        vk::ImageSubresourceRange()
          .setAspectMask( vk::ImageAspectFlagBits::eDepth )
          .setBaseMipLevel( 0u )
          .setLevelCount( 1u )
          .setBaseArrayLayer( 0u )
          .setLayerCount( 1u )
      )
  );

  const auto depth_rai = vk::RenderingAttachmentInfo()
    .setImageView( *depth_view )
    .setImageLayout( vk::ImageLayout::eGeneral )
    .setResolveMode( vk::ResolveModeFlagBits::eNone )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue(
      vk::ClearValue()
        .setDepthStencil(
          vk::ClearDepthStencilValue()
            .setDepth( 1.0f )
            .setStencil( 0u )
        )
    );
  return vulkan_framebuffer{
    color,
    depth,
    std::move( color_view ),
    std::move( depth_view ),
    color_rai,
    depth_rai
  };
}

vulkan_framebuffer
create_host_framebuffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< VmaAllocator > &allocator,
  const std::shared_ptr< void > &head,
  unsigned int width,
  unsigned int height
) {
  auto color = create_host_image(
    device,
    physical_device,
    head,
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      //.setFormat( vk::Format::eR5G6B5UnormPack16 )
      .setFormat( vk::Format::eB8G8R8A8Unorm )
      .setExtent( { width, height, 1u } )
      .setMipLevels( 1 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eLinear )
      .setUsage(
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eColorAttachment
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined )
  );

  auto color_view = device->createImageViewUnique(
    vk::ImageViewCreateInfo()
      .setImage( *color )
      .setViewType( vk::ImageViewType::e2D )
      //.setFormat( vk::Format::eR5G6B5UnormPack16 )
      .setFormat( vk::Format::eB8G8R8A8Unorm )
      .setSubresourceRange(
        vk::ImageSubresourceRange()
          .setAspectMask( vk::ImageAspectFlagBits::eColor )
          .setBaseMipLevel( 0u )
          .setLevelCount( 1u )
          .setBaseArrayLayer( 0u )
          .setLayerCount( 1u )
      )
  );

  const auto color_rai = vk::RenderingAttachmentInfo()
    .setImageView( *color_view )
    .setImageLayout( vk::ImageLayout::eGeneral )
    .setResolveMode( vk::ResolveModeFlagBits::eNone )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue(
      vk::ClearValue()
        .setColor(
          vk::ClearColorValue()
            .setFloat32( { 0.0f, 0.0f, 1.0f, 1.0f } )
        )
    );

  auto depth = create_image(
    allocator,
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      .setFormat( vk::Format::eD16Unorm )
      .setExtent( { width, height, 1u } )
      .setMipLevels( 1 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eOptimal )
      .setUsage(
        vk::ImageUsageFlagBits::eDepthStencilAttachment
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined ),
    VMA_MEMORY_USAGE_GPU_ONLY,
    0
  );
  
  auto depth_view = device->createImageViewUnique(
    vk::ImageViewCreateInfo()
      .setImage( *depth )
      .setViewType( vk::ImageViewType::e2D )
      .setFormat( vk::Format::eD16Unorm )
      .setSubresourceRange(
        vk::ImageSubresourceRange()
          .setAspectMask( vk::ImageAspectFlagBits::eDepth )
          .setBaseMipLevel( 0u )
          .setLevelCount( 1u )
          .setBaseArrayLayer( 0u )
          .setLayerCount( 1u )
      )
  );

  const auto depth_rai = vk::RenderingAttachmentInfo()
    .setImageView( *depth_view )
    .setImageLayout( vk::ImageLayout::eGeneral )
    .setResolveMode( vk::ResolveModeFlagBits::eNone )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue(
      vk::ClearValue()
        .setDepthStencil(
          vk::ClearDepthStencilValue()
            .setDepth( 1.0f )
            .setStencil( 0u )
        )
    );
  return vulkan_framebuffer{
    color,
    depth,
    std::move( color_view ),
    std::move( depth_view ),
    color_rai,
    depth_rai
  };
}

vulkan_framebuffer
create_host_framebuffer(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::PhysicalDevice &physical_device,
  const std::shared_ptr< VmaAllocator > &allocator,
  void *head,
  unsigned int width,
  unsigned int height
) {
  auto color = create_host_image(
    device,
    physical_device,
    head,
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      //.setFormat( vk::Format::eR5G6B5UnormPack16 )
      .setFormat( vk::Format::eB8G8R8A8Unorm )
      .setExtent( { width, height, 1u } )
      .setMipLevels( 1 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eLinear )
      .setUsage(
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eColorAttachment
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined )
  );

  auto color_view = device->createImageViewUnique(
    vk::ImageViewCreateInfo()
      .setImage( *color )
      .setViewType( vk::ImageViewType::e2D )
      //.setFormat( vk::Format::eR5G6B5UnormPack16 )
      .setFormat( vk::Format::eB8G8R8A8Unorm )
      .setSubresourceRange(
        vk::ImageSubresourceRange()
          .setAspectMask( vk::ImageAspectFlagBits::eColor )
          .setBaseMipLevel( 0u )
          .setLevelCount( 1u )
          .setBaseArrayLayer( 0u )
          .setLayerCount( 1u )
      )
  );

  const auto color_rai = vk::RenderingAttachmentInfo()
    .setImageView( *color_view )
    .setImageLayout( vk::ImageLayout::eGeneral )
    .setResolveMode( vk::ResolveModeFlagBits::eNone )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue(
      vk::ClearValue()
        .setColor(
          vk::ClearColorValue()
            .setFloat32( { 0.0f, 0.0f, 1.0f, 1.0f } )
        )
    );

  auto depth = create_image(
    allocator,
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      .setFormat( vk::Format::eD16Unorm )
      .setExtent( { width, height, 1u } )
      .setMipLevels( 1 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eOptimal )
      .setUsage(
        vk::ImageUsageFlagBits::eDepthStencilAttachment
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined ),
    VMA_MEMORY_USAGE_GPU_ONLY,
    0
  );
  
  auto depth_view = device->createImageViewUnique(
    vk::ImageViewCreateInfo()
      .setImage( *depth )
      .setViewType( vk::ImageViewType::e2D )
      .setFormat( vk::Format::eD16Unorm )
      .setSubresourceRange(
        vk::ImageSubresourceRange()
          .setAspectMask( vk::ImageAspectFlagBits::eDepth )
          .setBaseMipLevel( 0u )
          .setLevelCount( 1u )
          .setBaseArrayLayer( 0u )
          .setLayerCount( 1u )
      )
  );

  const auto depth_rai = vk::RenderingAttachmentInfo()
    .setImageView( *depth_view )
    .setImageLayout( vk::ImageLayout::eGeneral )
    .setResolveMode( vk::ResolveModeFlagBits::eNone )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue(
      vk::ClearValue()
        .setDepthStencil(
          vk::ClearDepthStencilValue()
            .setDepth( 1.0f )
            .setStencil( 0u )
        )
    );
  return vulkan_framebuffer{
    color,
    depth,
    std::move( color_view ),
    std::move( depth_view ),
    color_rai,
    depth_rai
  };
}

void wait_for_executed(
  vk::UniqueHandle<vk::Device, vk::detail::DispatchLoaderDynamic> &device,
  vk::Queue &queue,
  vk::UniqueHandle< vk::CommandBuffer, vk::detail::DispatchLoaderDynamic> &command_buffer
) {
  queue.submit(
    {
      vk::SubmitInfo()
        .setCommandBuffers( { *command_buffer } )
    },
    VK_NULL_HANDLE
  );
  queue.waitIdle();
}

}

