#pragma once
#include <glm/vec4.hpp>
#include <span>

namespace vf {
///
/// \brief Packed 4x 8-bit channel colour
///
struct Rgba {
	using Channel = std::uint8_t;

	glm::tvec4<Channel> channels{};

	static Rgba make(glm::vec4 const& v) { return {{unnormalize(v.x), unnormalize(v.y), unnormalize(v.z), unnormalize(v.w)}}; }
	static constexpr Rgba make(std::span<Channel const, 4> channels) { return {{channels[0], channels[1], channels[2], channels[3]}}; }
	static constexpr Rgba make(std::uint32_t mask);

	///
	/// \brief Normalize a channel to [0-1]
	///
	static float normalize(Channel ch);
	///
	/// \brief Unnormalize f to [0-255]
	///
	static Channel unnormalize(float f);

	///
	/// \brief Gamma correct linear
	///
	static glm::vec4 srgb(glm::vec4 const& linear);
	///
	/// \brief Gamme uncorrect srgb
	///
	static glm::vec4 linear(glm::vec4 const& srgb);
	///
	/// \brief Interpolate between two Rgba values
	///
	static Rgba lerp(Rgba a, Rgba b, float t);

	constexpr bool operator==(Rgba const& rhs) const = default;

	Rgba srgb() const;
	Rgba linear() const;

	///
	/// \brief Obtain channels as packed 32 bit unsigned int
	///
	constexpr std::uint32_t to_u32() const;
	///
	/// \brief Obtain channels as normalized floats
	///
	glm::vec4 normalize() const { return {normalize(channels[0]), normalize(channels[1]), normalize(channels[2]), normalize(channels[3])}; }
};

constexpr Rgba black_v = {{{}, {}, {}, 0xff}};
constexpr Rgba white_v = {{0xff, 0xff, 0xff, 0xff}};
constexpr Rgba red_v = {{0xff, {}, {}, 0xff}};
constexpr Rgba green_v = {{{}, 0xff, {}, 0xff}};
constexpr Rgba blue_v = {{{}, {}, 0xff, 0xff}};
constexpr Rgba yellow_v = {{0xff, 0xff, {}, 0xff}};
constexpr Rgba cyan_v = {{{}, 0xff, 0xff, 0xff}};
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

constexpr std::uint32_t Rgba::to_u32() const {
	using u32 = std::uint32_t;
	return (u32(channels[0]) << 24) | (u32(channels[1]) << 16) | (u32(channels[2]) << 8) | u32(channels[3]);
}
} // namespace vf
