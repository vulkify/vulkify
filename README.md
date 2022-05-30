# Vulkify

[![Build Status](https://github.com/vulkify/vulkify/actions/workflows/ci.yml/badge.svg)](https://github.com/vulkify/vulkify/actions/workflows/ci.yml)

## A lightweight 2D graphics framework

https://user-images.githubusercontent.com/16272243/168480491-a55fdc7d-c9ee-4dba-81ec-d1e085930f9f.mp4

Inspired by [SFML](https://github.com/SFML/SFML), powered by Vulkan and C++20.

## Features

- [x] No binary dependencies: builds everything from source
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
- [x] Shape outlines
- [x] Cursor unprojection
- [x] TrueType Fonts
- [x] Bulk command buffer recording
- [x] Text

## Requirements

### Runtime

- Vulkan 1.1 loader, conformant / portability-compatible ICD
- C++ runtime (linked dynamically)
- Desktop OS
  - Linux
  - Windows
  - MacOS [experimental]

### Build-time

- CMake 3.17+
- C++20 compiler and standard library
- Vulkan 1.1+ SDK
- Desktop windowing system/manager
  - Primary development environments:
    - Plasma KDE / X11
    - Windows 10
    - Raspbian (aarch64)
  - Other supported environments:
    - GNOME, Xfce, Wayland, ... [untested]
    - Windows 11/8/7/... [untested]
    - MacOS (through MoltenVK / Vulkan SDK) [experimental]

## Usage

Refer to [quick_start.cpp](examples/quick_start.cpp) for currently working code. Documentation and README are WIPs.


## [Wiki](https://github.com/vulkify/vulkify/wiki)

Documentation and sample code is available on [the wiki](https://github.com/vulkify/vulkify/wiki) (WIP).

## External Dependencies

### Public

- [glm](https://github.com/g-truc/glm)
- [ktl](https://github.com/karnkaul/ktl)

### Private

- [GLFW](https://github.com/glfw/glfw)
- [stb](https://github.com/nothings/stb)
- [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [freetype](https://github.com/freetype/freetype)

[Original repository](https://github.com/vulkify/vulkify)

[LICENCE](LICENSE)
