#pragma once
#include <glm/vec4.hpp>
#include <span>

namespace vf {
struct Rgba {
	using Channel = std::uint8_t;

	glm::tvec4<Channel> channels{};

	static constexpr Rgba make(glm::vec4 const& v) { return {{unnormalize(v.x), unnormalize(v.y), unnormalize(v.z), unnormalize(v.w)}}; }
	static constexpr Rgba make(std::span<Channel const, 4> channels) { return {{channels[0], channels[1], channels[2], channels[3]}}; }
	static constexpr Rgba make(std::uint32_t mask);

	static float normalize(Channel ch);
	static Channel unnormalize(float f);

	static glm::vec4 srgb(glm::vec4 const& linear);
	static glm::vec4 linear(glm::vec4 const& srgb);
	static Rgba lerp(Rgba a, Rgba b, float t);

	Rgba srgb() const;
	Rgba linear() const;

	constexpr glm::vec4 normalize() const { return {normalize(channels[0]), normalize(channels[1]), normalize(channels[2]), normalize(channels[3])}; }
};

constexpr Rgba black_v = {{{}, {}, {}, 0xff}};
constexpr Rgba white_v = {{0xff, 0xff, 0xff, 0xff}};
constexpr Rgba red_v = {{0xff, {}, {}, 0xff}};
constexpr Rgba green_v = {{{}, 0xff, {}, 0xff}};
constexpr Rgba blue_v = {{{}, {}, 0xff, 0xff}};
constexpr Rgba yellow_v = {{0xff, 0xff, {}, 0xff}};
constexpr Rgba cyan_v = {{0xff, 0xff, 0xff, 0xff}};
constexpr Rgba magenta_v = {{0xff, {}, 0xff, 0xff}};

// impl

constexpr Rgba Rgba::make(std::uint32_t mask) {
	auto ret = Rgba{};
	ret.channels[3] = mask & 0xff;
	mask >>= 8;
	ret.channels[2] = mask & 0xff;
	mask >>= 8;
	ret.channels[1] = mask & 0xff;
	mask >>= 8;
	ret.channels[0] = mask & 0xff;
	return ret;
}
} // namespace vf
