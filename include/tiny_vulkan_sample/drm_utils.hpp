#ifndef TINY_VULKAN_SAMPLE_DRM_UTILS_HPP
#define TINY_VULKAN_SAMPLE_DRM_UTILS_HPP
/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <cstdint>
#include <cstring>
#include <array>
#include <stdexcept>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace tiny_vulkan_sample {

class dumb_buffer {
public:
  dumb_buffer(
    const std::string &devname,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t bpp,
    std::uint32_t format
  ) {
    fd = open( devname.c_str(), O_RDWR );
    if( fd < 0 ) {
      throw std::runtime_error( "Unable to open device." );
    }
    res = drmModeGetResources( fd );
    if( res == nullptr ) {
      close( fd );
      throw std::runtime_error( "drmModeGetResources failed" );
    }
    std::memset( reinterpret_cast< void* >( &creq ), 0, sizeof( creq ) );
    creq.width = width;
    creq.height = height;
    creq.bpp = bpp;
    if( drmIoctl( fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq ) ) {
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "drmIoctl DRM_IOCTL_MODE_CREATE_DUMB failed." );
    }

    std::array< std::uint32_t, 4u > handles{ creq.handle, 0u, 0u, 0u };
    std::array< std::uint32_t, 4u > pitches{ creq.pitch, 0u, 0u, 0u };
    std::array< std::uint32_t, 4u > offsets{ 0u, 0u, 0u, 0u };
    if( drmModeAddFB2(
      fd,
      creq.width,
      creq.height,
      format,
      handles.data(),
      pitches.data(),
      offsets.data(),
      &fb,
      0u
    ) ) {
      drm_mode_destroy_dumb dreq;
      dreq.handle = creq.handle;
      drmIoctl( fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq );
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "drmModeAddFB failed." );
    }

    for ( int i = 0; i < res->count_connectors; ++i ) {
      drmModeConnectorPtr candidate = drmModeGetConnectorCurrent( fd, res->connectors[ i ] );
      if( !candidate )
          continue;
 
      for( int j = 0; j < candidate->count_modes; ++j ) {
        if(
          candidate->modes[ j ].hdisplay == creq.width && 
          candidate->modes[ j ].vdisplay == creq.height
        ) {
          connector = candidate;
          mode_index = j;
          i = res->count_connectors;
          j = candidate->count_modes;
        }
      }
      if( i != res->count_connectors ) {
        drmModeFreeConnector( candidate );
      }
    }

    if( connector == nullptr ) {
      drm_mode_destroy_dumb dreq;
      dreq.handle = creq.handle;
      drmIoctl( fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq );
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "no suitable mode." );
    }

    encoder = drmModeGetEncoder( fd, connector->encoder_id );
    if ( encoder == nullptr ) {
      drmModeFreeConnector( connector );
      drm_mode_destroy_dumb dreq;
      dreq.handle = creq.handle;
      drmIoctl( fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq );
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "drmModeGetEncoder failed." );
    }

    crtc = drmModeGetCrtc( fd, encoder->crtc_id );
    if ( crtc == nullptr ) {
      drmModeFreeEncoder( encoder );
      drmModeFreeConnector( connector );
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "drmModeGetCrtc failed." );
    }

    std::memset( reinterpret_cast< void* >( &mreq ), 0, sizeof( mreq ) );
    mreq.handle = creq.handle;
    if( drmIoctl( fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq ) ) {
      drmModeFreeCrtc( crtc );
      drmModeFreeEncoder( encoder );
      drmModeFreeConnector( connector );
      drm_mode_destroy_dumb dreq;
      dreq.handle = creq.handle;
      drmIoctl( fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq );
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "drmIoctl DRM_IOCTL_MODE_MAP_DUMB failed." );
    }

    raw_mapped = mmap( 0, creq.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, mreq.offset );
    if( raw_mapped == MAP_FAILED ) {
      drmModeFreeCrtc( crtc );
      drmModeFreeEncoder( encoder );
      drmModeFreeConnector( connector );
      drm_mode_destroy_dumb dreq;
      dreq.handle = creq.handle;
      drmIoctl( fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq );
      drmModeFreeResources( res );
      close( fd );
      throw std::runtime_error( "mmap failed." );
    }

    drmSetMaster( fd );
    drmModeSetCrtc( fd, crtc->crtc_id, 0, 0, 0, nullptr, 0, nullptr );
    drmModeSetCrtc( fd, crtc->crtc_id, fb, 0, 0, &connector->connector_id, 1, &connector->modes[ mode_index ] );
  }
  ~dumb_buffer() {
    drmDropMaster( fd );
    munmap( raw_mapped, creq.size );
    drmModeFreeCrtc( crtc );
    drmModeFreeEncoder( encoder );
    drmModeFreeConnector( connector );
    drm_mode_destroy_dumb dreq;
    dreq.handle = creq.handle;
    drmIoctl( fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq );
    drmModeFreeResources( res );
    close( fd );
  }
  dumb_buffer( const dumb_buffer& ) = delete;
  dumb_buffer( dumb_buffer&& ) = delete;
  dumb_buffer &operator=( const dumb_buffer& ) = delete;
  dumb_buffer &operator=( dumb_buffer&& ) = delete;
  void *get_buffer() {
    return raw_mapped;
  }
  std::uint32_t get_pitch() const {
    return creq.pitch;
  }
  void present() {
    drmModeSetCrtc( fd, crtc->crtc_id, fb, 0, 0, &connector->connector_id, 1, &connector->modes[ mode_index ] );
  }
private:
  int fd = -1;
  drmModeResPtr res = nullptr;
  drm_mode_create_dumb creq;
  std::uint32_t fb = 0;
  drmModeConnectorPtr connector = nullptr;
  int mode_index = 0u;
  drmModeEncoderPtr encoder = nullptr;
  drmModeCrtcPtr crtc = nullptr;
  drm_mode_map_dumb mreq;
  void *raw_mapped = nullptr;
};

}

#endif

