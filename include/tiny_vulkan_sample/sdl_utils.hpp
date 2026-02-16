#ifndef TINY_VULKAN_SAMPLE_SDL_UTILS_HPP
#define TINY_VULKAN_SAMPLE_SDL_UTILS_HPP
/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#include <memory>
#include <stdexcept>
#include <SDL3/SDL.h>
#include <unistd.h>

namespace tiny_vulkan_sample::sdl {

class context {
public:
  static const context &get() {
    static const context context;
    return context;
  }
private:
  context() {
    if( !SDL_Init( SDL_INIT_VIDEO ) ) {
      throw std::runtime_error( "SDL_Init failed" );
      std::abort();
    }
  }
  ~context() {
    SDL_Quit();
  }
};

class texture {
public:
  texture( SDL_Texture *t ) : texture_( t ) {}
  ~texture() {
    if( texture_ ) {
      SDL_DestroyTexture( texture_ );
    }
  }
  texture( const texture& ) = delete;
  texture( texture &&src ) : texture_( src.texture_ ) {
    src.texture_ = nullptr;
  }
  texture &operator=( const texture& ) = delete;
  texture &operator=( texture &&src ) {
    texture_ = src.texture_;
    src.texture_ = nullptr;
    return *this;
  }
  template< typename T >
  std::shared_ptr< T > lock() {
    void *raw = nullptr;
    int pitch = 0;
    if( !SDL_LockTexture( texture_, nullptr, &raw, &pitch ) ) {
      throw std::runtime_error( "SDL_LockTexture failed." );
    }
    std::shared_ptr< T > wrapped(
      reinterpret_cast< T* >( raw ),
      [t=texture_]( T* ) {
        if( t ) {
          SDL_UnlockTexture( t );
        }
      }
    );
    return wrapped;
  }
  SDL_Texture *get_raw() const {
    return texture_;
  }
private:
  SDL_Texture *texture_;
};

class renderer {
public:
  renderer( SDL_Renderer *r ) : renderer_( r ) {}
  ~renderer() {
    if( renderer_ ) {
      SDL_DestroyRenderer( renderer_ );
    }
  }
  renderer( const renderer& ) = delete;
  renderer( renderer &&src ) : renderer_( src.renderer_ ) {
    src.renderer_ = nullptr;
  }
  renderer &operator=( const renderer& ) = delete;
  renderer &operator=( renderer &&src ) {
    renderer_ = src.renderer_;
    src.renderer_ = nullptr;
    return *this;
  }
  texture create_texture(
    SDL_PixelFormat format,
    SDL_TextureAccess	access,
    int width,
    int height
  ) {
    SDL_Texture *t = SDL_CreateTexture(
      renderer_,
      format,
      access,
      width,
      height
    );
    if( !t ) {
      throw std::runtime_error( "SDL_CreateTexture failed" );
    }
    return texture( t );
  }
  void clear() {
    if( !SDL_RenderClear( renderer_ ) ) {
      throw std::runtime_error( "SDL_RenderClear failed" );
    }
  }
  void draw( const texture &t ) {
    if( !SDL_RenderTexture( renderer_, t.get_raw(), nullptr, nullptr ) ) {
      throw std::runtime_error( "SDL_RenderTexture failed" );
    }
  }
  void present() {
    if( !SDL_RenderPresent( renderer_ ) ) {
      throw std::runtime_error( "SDL_RenderPresent failed" );
    }
  }
private:
  SDL_Renderer *renderer_;
};

class window {
public:
  window(
    int width,
    int height
  ) {
    window_ = SDL_CreateWindow(
      "window name",
      width,
      height,
      0
    );
    if( !window_ ) {
      throw std::runtime_error( "SDL_CreateWindow failed" );
    }
  }
  window( const window& ) = delete;
  window( window &&src ) : window_( src.window_ ) {
    src.window_ = nullptr;
  }
  window &operator=( const window& ) = delete;
  window &operator=( window &&src ) {
    window_ = src.window_;
    src.window_ = nullptr;
    return *this;
  }
  renderer create_renderer() {
    SDL_Renderer *r = SDL_CreateRenderer(
      window_,
      nullptr
    );
    if( !r ) {
      throw std::runtime_error( "SDL_CreateRenderer failed" );
    }
    return renderer( r );
  }
  ~window() {
    if( window_ ) {
      SDL_DestroyWindow( window_ );
    }
  }
private:
  SDL_Window *window_;
};
}

#endif

