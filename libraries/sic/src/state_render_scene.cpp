#include "sic/state_render_scene.h"

void sic::State_render_scene::flush_updates()
{
	m_views.flush_updates();
	m_models.flush_updates();
}
