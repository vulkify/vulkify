# Vulkify

[![Build Status](https://github.com/vulkify/vulkify/actions/workflows/ci.yml/badge.svg)](https://github.com/vulkify/vulkify/actions/workflows/ci.yml)

## A lightweight 2D graphics framework

![Screenshot](docs/screenshot.png)

Inspired by [SFML](https://github.com/SFML/SFML), powered by Vulkan and C++20.

## Features

- [x] Dedicated fullscreen, borderless, and decorated windows
- [x] sRGB / linear framebuffers
- [x] Stall-less swapchain recreation
- [x] Custom cursors / window icons
- [x] Pipeline state customization: 
  - Polygon mode, primitive topology, line width
- [x] Instanced draws
- [x] Textures, Images, Bitmaps
- [x] Customizable viewport
- [x] Multi-monitor support
- [x] Multi-sampled anti-aliasing (MSAA)
- [x] Custom (fragment) shader support
  - [x] Custom descriptor set
  - [ ] Custom uniform buffer
  - [ ] Custom texture
- [ ] Shape outlines
- [ ] Cursor unprojection
- [ ] Fonts (Freetype)
- [ ] Text

## Requirements

- CMake 3.19+
- C++20 compiler and standard library
- Vulkan 1.1+
- Desktop windowing system/manager
  - Windows 10
  - X11 / Wayland (untested)
  - Raspbian (aarch64)

## Usage

Refer to [quick_start.cpp](examples/quick_start.cpp) for currently working code. Documentation and README are WIPs.

## External Dependencies

- [GLFW](https://github.com/glfw/glfw)
- [glm](https://github.com/g-truc/glm)
- [stb](https://github.com/nothings/stb)
- [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [ktl](https://github.com/karnkaul/ktl)

[Original repository](https://github.com/vulkify/vulkify)

[LICENCE](LICENSE)
