#include "sic/state_render_scene.h"

void sic::State_render_scene::flush_updates()
{
	m_windows.flush_updates();
	for (auto&& ui_elements : m_window_id_to_ui_elements)
		ui_elements.second.flush_updates();

	for (auto& scene_it : m_level_id_to_scene_lut)
		std::apply([](auto&& ... in_args) {(in_args.flush_updates(), ...); }, scene_it.second);
}
