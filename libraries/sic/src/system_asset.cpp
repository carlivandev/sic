#include "sic/system_asset.h"
#include "sic/asset_types.h"
#include "sic/file_management.h"

#include "nlohmann/json.hpp"
#include <filesystem>

namespace sic_private
{
	using namespace sic;

	std::unique_ptr<Asset_header> parse_header(std::string&& in_header_data, State_assetsystem& in_assetsystem_state)
	{
		auto json_object = nlohmann::json::parse(in_header_data);

		if (json_object.is_null())
			return nullptr;

		std::unique_ptr<Asset_header> new_header = std::make_unique<Asset_header>(in_assetsystem_state);

		new_header->m_id = xg::Guid(json_object["id"].get<std::string>());
		new_header->m_asset_path = json_object["asset_path"].get<std::string>();
		new_header->m_typename = json_object["type"].get<std::string>();

		return std::move(new_header);
	}
}

void sic::System_asset::on_created(Engine_context in_context)
{
	in_context.register_state<State_assetsystem>("assetsystem");

	const char* root = "content";

	if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root))
		return;

	std::filesystem::recursive_directory_iterator it(root);
	std::filesystem::recursive_directory_iterator endit;

	std::vector<std::filesystem::path> asset_headers_to_load;

	while (it != endit)
	{
		if (std::filesystem::is_regular_file(*it) && it->path().extension() == ".asset_h")
			asset_headers_to_load.push_back(std::filesystem::absolute(it->path()));

		++it;
	}

	State_assetsystem* assetsystem_state = in_context.get_state<State_assetsystem>();

	assetsystem_state->m_asset_headers.reserve(asset_headers_to_load.size());
	assetsystem_state->m_unload_queue.set_size(50);

	for (const std::filesystem::path& asset_header_path : asset_headers_to_load)
	{
		std::string asset_header_data = std::move(File_management::load_file(asset_header_path.string()));

		if (asset_header_data.empty())
			continue;

		auto new_header = sic_private::parse_header(std::move(asset_header_data), *assetsystem_state);
		 
		if (!new_header)
			continue;

		assetsystem_state->m_asset_headers.push_back(std::move(new_header));
		assetsystem_state->m_id_to_header[assetsystem_state->m_asset_headers.back()->m_id] = assetsystem_state->m_asset_headers.back().get();
	}
}

void sic::State_assetsystem::leave_unload_queue(const Asset_header& in_header)
{
	m_unload_queue.leave_queue(in_header.m_unload_ticket);
}

void sic::State_assetsystem::join_unload_queue(Asset_header& in_out_header)
{
	if (m_unload_queue.is_queue_full())
		unload_next_asset();

	in_out_header.m_unload_ticket = m_unload_queue.enqueue(&in_out_header);
}

void sic::State_assetsystem::force_unload_unreferenced_assets()
{
	while (!m_unload_queue.is_queue_empty())
		unload_next_asset();
}

sic::Asset_header* sic::State_assetsystem::create_asset_internal(const std::string& in_asset_name, const std::string& in_asset_directory, const std::string& in_typename)
{
	m_asset_headers.push_back(std::make_unique<Asset_header>(*this));
	Asset_header* new_header = m_asset_headers.back().get();

	new_header->m_id = xg::newGuid();
	new_header->m_name = in_asset_name;
	new_header->m_asset_path = fmt::format("{0}/{1}.asset", in_asset_directory, in_asset_name);
	new_header->m_load_state = Asset_load_state::loaded;
	new_header->m_typename = in_typename;

	m_id_to_header[m_asset_headers.back()->m_id] = m_asset_headers.back().get();

	return new_header;
}

void sic::State_assetsystem::save_asset_header(const Asset_header& in_header, const std::string& in_typename)
{
	nlohmann::json header_json;

	header_json["type"] = in_typename;
	header_json["id"] = in_header.m_id.str();
	header_json["name"] = in_header.m_name;
	header_json["asset_path"] = in_header.m_asset_path;

	File_management::save_file(fmt::format("{0}_h", in_header.m_asset_path), header_json.dump(1, '\t'));
}

void sic::State_assetsystem::unload_next_asset()
{
	Asset_header* header = m_unload_queue.dequeue();

	if (header->m_loaded_asset.get()->has_pre_unload())
	{
		std::scoped_lock post_load_lock(m_pre_unload_mutex);
		header->m_load_state = Asset_load_state::unloading;

		std::unique_ptr<std::vector<Asset_header*>>& pre_unload_assets = m_typename_to_pre_unload_headers[header->m_typename];

		if (!pre_unload_assets)
			pre_unload_assets = std::make_unique<std::vector<Asset_header*>>();

		pre_unload_assets->push_back(header);
	}
	else
	{
		SIC_LOG(g_log_asset, "unloaded asset: \"{0}\"", header->m_name.c_str());

		header->m_load_state = Asset_load_state::not_loaded;
		header->m_loaded_asset.reset();
	}
}
