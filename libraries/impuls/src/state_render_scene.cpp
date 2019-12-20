#include "impuls/state_render_scene.h"

void impuls::state_render_scene::flush_updates()
{
	m_views.flush_updates();
	m_models.flush_updates();
}
