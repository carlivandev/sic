#pragma once
#include "impuls/asset.h"
#include "impuls/system_asset.h"

void impuls::asset_header::increment_reference_count()
{
	std::scoped_lock lock(m_reference_mutex);

	if (m_reference_count == 0)
		m_assetsystem_state->m_unload_queue.leave_queue(m_unload_ticket);

	++m_reference_count;
}

void impuls::asset_header::decrement_reference_count()
{
	std::scoped_lock lock(m_reference_mutex);
	--m_reference_count;

	if (m_reference_count == 0)
	{
		if (m_assetsystem_state->m_unload_queue.is_queue_full())
		{
			//todo: dequeue and unload asset
			//when dequeing, we move it into ANOTHER list of things to call pre-unload on
			//after pre-unload, we actually unload it, and after THAT we allow it to be loaded once again
			//also move both increment/decrement functinoality into assetsystem state, it makes more sense for that one to actually execute the behaviour
		}

		m_unload_ticket = m_assetsystem_state->m_unload_queue.enqueue(this);
	}
}