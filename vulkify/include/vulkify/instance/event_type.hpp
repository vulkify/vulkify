#pragma once

namespace vf {
enum class EventType {
	// void
	eClose,

	// bool
	eIconify,
	eFocus,
	eMaximize,
	eCursorEnter,

	// ivec2
	eMove,
	eWindowResize,
	eFramebufferResize,

	// vec2
	eCursorPos,
	eScroll,

	// KeyEvent
	eKey,
	eMouseButton,
};
}
