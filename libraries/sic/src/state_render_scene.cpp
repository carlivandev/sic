#include "sic/state_render_scene.h"

void sic::State_render_scene::flush_updates()
{
	for (auto& scene_it : m_level_id_to_scene_lut)
		std::apply([](auto&& ... in_args) {(in_args.flush_updates(), ...); }, scene_it.second);
}
