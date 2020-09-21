#pragma once
#include "sic/core/engine_context.h"
#include "gl_includes.h"

#include <array>

namespace sic
{
	enum struct Key
	{
		unknown = GLFW_KEY_UNKNOWN,

		/* printable keys begin */
		space = GLFW_KEY_SPACE,
		apostrophe = GLFW_KEY_APOSTROPHE,
		comma = GLFW_KEY_COMMA,
		minus = GLFW_KEY_MINUS,
		period = GLFW_KEY_PERIOD,
		slash = GLFW_KEY_SLASH,
		num_0 = GLFW_KEY_0,
		num_1 = GLFW_KEY_1,
		num_2 = GLFW_KEY_2,
		num_3 = GLFW_KEY_3,
		num_4 = GLFW_KEY_4,
		num_5 = GLFW_KEY_5,
		num_6 = GLFW_KEY_6,
		num_7 = GLFW_KEY_7,
		num_8 = GLFW_KEY_8,
		num_9 = GLFW_KEY_9,
		semicolon = GLFW_KEY_SEMICOLON,
		equal = GLFW_KEY_EQUAL,
		a = GLFW_KEY_A,
		b = GLFW_KEY_B,
		c = GLFW_KEY_C,
		d = GLFW_KEY_D,
		e = GLFW_KEY_E,
		f = GLFW_KEY_F,
		g = GLFW_KEY_G,
		h = GLFW_KEY_H,
		i = GLFW_KEY_I,
		j = GLFW_KEY_J,
		k = GLFW_KEY_K,
		l = GLFW_KEY_L,
		m = GLFW_KEY_M,
		n = GLFW_KEY_N,
		o = GLFW_KEY_O,
		p = GLFW_KEY_P,
		q = GLFW_KEY_Q,
		r = GLFW_KEY_R,
		s = GLFW_KEY_S,
		t = GLFW_KEY_T,
		u = GLFW_KEY_U,
		v = GLFW_KEY_V,
		w = GLFW_KEY_W,
		x = GLFW_KEY_X,
		y = GLFW_KEY_Y,
		z = GLFW_KEY_Z,
		left_bracket = GLFW_KEY_LEFT_BRACKET,
		backslash = GLFW_KEY_BACKSLASH,
		right_bracket = GLFW_KEY_RIGHT_BRACKET,
		grave_accent = GLFW_KEY_GRAVE_ACCENT,
		world_1 = GLFW_KEY_WORLD_1,
		world_2 = GLFW_KEY_WORLD_2,
		/* printable keys end */

		/* function keys begin */
		escape = GLFW_KEY_ESCAPE,
		enter = GLFW_KEY_ENTER,
		tab = GLFW_KEY_TAB,
		backspace = GLFW_KEY_BACKSPACE,
		insert = GLFW_KEY_INSERT,
		del = GLFW_KEY_DELETE,
		right = GLFW_KEY_RIGHT,
		left = GLFW_KEY_LEFT,
		down = GLFW_KEY_DOWN,
		up = GLFW_KEY_UP,
		page_up = GLFW_KEY_PAGE_UP,
		page_down = GLFW_KEY_PAGE_DOWN,
		home = GLFW_KEY_HOME,
		end = GLFW_KEY_END,
		caps_lock = GLFW_KEY_CAPS_LOCK,
		scroll_lock = GLFW_KEY_SCROLL_LOCK,
		num_lock = GLFW_KEY_NUM_LOCK,
		print_screen = GLFW_KEY_PRINT_SCREEN,
		pause = GLFW_KEY_PAUSE,
		/* function keys end */

		f1 = GLFW_KEY_F1,
		f2 = GLFW_KEY_F2,
		f3 = GLFW_KEY_F3,
		f4 = GLFW_KEY_F4,
		f5 = GLFW_KEY_F5,
		f6 = GLFW_KEY_F6,
		f7 = GLFW_KEY_F7,
		f8 = GLFW_KEY_F8,
		f9 = GLFW_KEY_F9,
		f10 = GLFW_KEY_F10,
		f11 = GLFW_KEY_F11,
		f12 = GLFW_KEY_F12,
		f13 = GLFW_KEY_F13,
		f14 = GLFW_KEY_F14,
		f15 = GLFW_KEY_F15,
		f16 = GLFW_KEY_F16,
		f17 = GLFW_KEY_F17,
		f18 = GLFW_KEY_F18,
		f19 = GLFW_KEY_F19,
		f20 = GLFW_KEY_F20,
		f21 = GLFW_KEY_F21,
		f22 = GLFW_KEY_F22,
		f23 = GLFW_KEY_F23,
		f24 = GLFW_KEY_F24,
		f25 = GLFW_KEY_F25,
		numpad_0 = GLFW_KEY_KP_0,
		numpad_1 = GLFW_KEY_KP_1,
		numpad_2 = GLFW_KEY_KP_2,
		numpad_3 = GLFW_KEY_KP_3,
		numpad_4 = GLFW_KEY_KP_4,
		numpad_5 = GLFW_KEY_KP_5,
		numpad_6 = GLFW_KEY_KP_6,
		numpad_7 = GLFW_KEY_KP_7,
		numpad_8 = GLFW_KEY_KP_8,
		numpad_9 = GLFW_KEY_KP_9,
		numpad_decimal = GLFW_KEY_KP_DECIMAL,
		numpad_divide = GLFW_KEY_KP_DIVIDE,
		numpad_multiply = GLFW_KEY_KP_MULTIPLY,
		numpad_subtract = GLFW_KEY_KP_SUBTRACT,
		numpad_add = GLFW_KEY_KP_ADD,
		numpad_enter = GLFW_KEY_KP_ENTER,
		numpad_equal = GLFW_KEY_KP_EQUAL,
		left_shift = GLFW_KEY_LEFT_SHIFT,
		left_control = GLFW_KEY_LEFT_CONTROL,
		left_alt = GLFW_KEY_LEFT_ALT,
		left_super = GLFW_KEY_LEFT_SUPER,
		right_shift = GLFW_KEY_RIGHT_SHIFT,
		right_control = GLFW_KEY_RIGHT_CONTROL,
		right_alt = GLFW_KEY_RIGHT_ALT,
		right_super = GLFW_KEY_RIGHT_SUPER,
		menu = GLFW_KEY_MENU,
		count = GLFW_KEY_LAST
	};

	enum struct Mousebutton
	{
		num_1 = GLFW_MOUSE_BUTTON_1,
		num_2 = GLFW_MOUSE_BUTTON_2,
		num_3 = GLFW_MOUSE_BUTTON_3,
		num_4 = GLFW_MOUSE_BUTTON_4,
		num_5 = GLFW_MOUSE_BUTTON_5,
		num_6 = GLFW_MOUSE_BUTTON_6,
		num_7 = GLFW_MOUSE_BUTTON_7,
		num_8 = GLFW_MOUSE_BUTTON_8,
		num_count = GLFW_MOUSE_BUTTON_8,

		left = num_1,
		right = num_2,
		middle = num_3
	};

	struct State_input : public State
	{
		bool is_key_down(Key in_key) const;
		bool is_key_pressed(Key in_key) const;
		bool is_key_released(Key in_key) const;

		bool is_mousebutton_down(Mousebutton in_button) const;
		bool is_mousebutton_pressed(Mousebutton in_button) const;
		bool is_mousebutton_released(Mousebutton in_button) const;

		std::optional<Mousebutton> get_down_mousebutton() const;
		std::optional<Mousebutton> get_pressed_mousebutton() const;
		std::optional<Mousebutton> get_released_mousebutton() const;

		std::array<bool, static_cast<i32>(Mousebutton::num_count)> mouse_last_frame_down;
		std::array<bool, static_cast<i32>(Mousebutton::num_count)> mouse_this_frame_down;

		std::array<bool, static_cast<i32>(Key::count)> key_last_frame_down;
		std::array<bool, static_cast<i32>(Key::count)> key_this_frame_down;

		double m_scroll_offset_x = 0.0;
		double m_scroll_offset_y = 0.0;
	};
}