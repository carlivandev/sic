#pragma once
#include "impuls/asset.h"
#include "impuls/system_asset.h"
#include "impuls/logger.h"

void impuls::Asset_header::increment_reference_count()
{
	std::scoped_lock lock(m_reference_mutex);

	if (m_reference_count == 0)
		m_assetsystem_state->leave_unload_queue(*this);

	++m_reference_count;

	IMPULS_LOG_W(g_log_asset_verbose, "incremented: \"{0}\", ref count: {1}", m_name.c_str(), m_reference_count);
}

void impuls::Asset_header::decrement_reference_count()
{
	std::scoped_lock lock(m_reference_mutex);

	--m_reference_count;

	IMPULS_LOG_W(g_log_asset_verbose, "decremented: \"{0}\", ref count: {1}", m_name.c_str(), m_reference_count);

	if (m_reference_count == 0)
		m_assetsystem_state->join_unload_queue(*this);
}