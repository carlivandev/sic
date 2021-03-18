#include "sic/core/engine_context.h"
#include "sic/core/object_base.h"
#include "sic/core/scene_context.h"

void sic::Engine_context::destroy_scene(Scene_context in_context)
{
	m_engine->m_scenes_to_remove.push_back(in_context.get_scene_id());
}

std::optional<sic::Scene_context> sic::Engine_context::find_scene(sic::i32 in_scene_index)
{
	if (in_scene_index >= 0 && in_scene_index < m_engine->m_scenes.size())
		return sic::Scene_context(*m_engine, *m_engine->m_scenes[in_scene_index].get());

	return {};
}
