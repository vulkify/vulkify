#pragma once
#include <detail/gfx_allocation.hpp>
#include <detail/handle_map.hpp>
#include <ktl/kunique_ptr.hpp>

namespace vf::refactor {
using GfxAllocationMap = HandleMap<ktl::kunique_ptr<GfxAllocation>>;
} // namespace vf::refactor
