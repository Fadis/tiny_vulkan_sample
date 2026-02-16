/* SPDX-FileCopyrightText: 2026 Naomasa Matsubayashi <fadis@quaternion.sakura.ne.jp> */
/* SPDX-License-Identifier: MIT */
#define VMA_IMPLEMENTATION
#include <tiny_vulkan_sample/vma.hpp>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#include <tiny_vulkan_sample/vk_mem_alloc.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#else
#pragma clang diagnostic pop
#endif

