#include "impuls/input.h"

bool impuls::state_input::is_key_down(e_key in_key) const
{
	if (in_key == e_key::unknown)
		return false;

	const i32 key_idx = static_cast<i32>(in_key);

	return key_this_frame_down[key_idx];
}

bool impuls::state_input::is_key_pressed(e_key in_key) const
{
	if (in_key == e_key::unknown)
		return false;

	const i32 key_idx = static_cast<i32>(in_key);

	return key_this_frame_down[key_idx] && !key_last_frame_down[key_idx];
}

bool impuls::state_input::is_key_released(e_key in_key) const
{
	if (in_key == e_key::unknown)
		return false;

	const i32 key_idx = static_cast<i32>(in_key);

	return !key_this_frame_down[key_idx] && key_last_frame_down[key_idx];
}

bool impuls::state_input::is_mousebutton_down(e_mousebutton in_button) const
{
	const i32 button_idx = static_cast<i32>(in_button);

	return mouse_this_frame_down[button_idx];
}

bool impuls::state_input::is_mousebutton_pressed(e_mousebutton in_button) const
{
	const i32 button_idx = static_cast<i32>(in_button);

	return mouse_this_frame_down[button_idx] && !mouse_last_frame_down[button_idx];
}

bool impuls::state_input::is_mousebutton_released(e_mousebutton in_button) const
{
	const i32 button_idx = static_cast<i32>(in_button);

	return !mouse_this_frame_down[button_idx] && mouse_last_frame_down[button_idx];
}
