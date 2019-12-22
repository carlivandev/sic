#include "sic/input.h"

bool sic::State_input::is_key_down(Key in_key) const
{
	if (in_key == Key::unknown)
		return false;

	const i32 key_idx = static_cast<i32>(in_key);

	return key_this_frame_down[key_idx];
}

bool sic::State_input::is_key_pressed(Key in_key) const
{
	if (in_key == Key::unknown)
		return false;

	const i32 key_idx = static_cast<i32>(in_key);

	return key_this_frame_down[key_idx] && !key_last_frame_down[key_idx];
}

bool sic::State_input::is_key_released(Key in_key) const
{
	if (in_key == Key::unknown)
		return false;

	const i32 key_idx = static_cast<i32>(in_key);

	return !key_this_frame_down[key_idx] && key_last_frame_down[key_idx];
}

bool sic::State_input::is_mousebutton_down(Mousebutton in_button) const
{
	const i32 button_idx = static_cast<i32>(in_button);

	return mouse_this_frame_down[button_idx];
}

bool sic::State_input::is_mousebutton_pressed(Mousebutton in_button) const
{
	const i32 button_idx = static_cast<i32>(in_button);

	return mouse_this_frame_down[button_idx] && !mouse_last_frame_down[button_idx];
}

bool sic::State_input::is_mousebutton_released(Mousebutton in_button) const
{
	const i32 button_idx = static_cast<i32>(in_button);

	return !mouse_this_frame_down[button_idx] && mouse_last_frame_down[button_idx];
}
