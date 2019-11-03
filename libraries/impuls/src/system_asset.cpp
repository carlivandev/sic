#include "impuls/system_asset.h"
#include "impuls/asset_types.h"
#include "impuls/file_management.h"

#include "nlohmann/json.hpp"
#include <filesystem>

namespace impuls_private
{
	using namespace impuls;

	std::unique_ptr<asset_header> parse_header(std::string&& in_header_data, state_assetsystem& in_assetsystem_state)
	{
		auto json_object = nlohmann::json::parse(in_header_data);

		if (json_object.is_null())
			return nullptr;

		std::unique_ptr<asset_header> new_header = std::make_unique<asset_header>(in_assetsystem_state);

		new_header->m_id = xg::Guid(json_object["id"].get<std::string>());
		new_header->m_asset_path = json_object["asset_path"].get<std::string>();
		new_header->m_typename = json_object["type"].get<std::string>();

		return std::move(new_header);
	}
}

void impuls::system_asset::on_created(world_context&& in_context) const
{
	in_context.register_state<state_assetsystem>("assetsystem");

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

	state_assetsystem* assetsystem_state = in_context.get_state<state_assetsystem>();

	assetsystem_state->m_asset_headers.reserve(asset_headers_to_load.size());
	assetsystem_state->m_unload_queue.set_size(50);

	for (const std::filesystem::path& asset_header_path : asset_headers_to_load)
	{
		std::string asset_header_data = std::move(file_management::load_file(asset_header_path.string()));

		if (asset_header_data.empty())
			continue;

		auto new_header = impuls_private::parse_header(std::move(asset_header_data), *assetsystem_state);
		 
		if (!new_header)
			continue;

		assetsystem_state->m_asset_headers.push_back(std::move(new_header));
		assetsystem_state->m_id_to_header[assetsystem_state->m_asset_headers.back()->m_id] = assetsystem_state->m_asset_headers.back().get();
	}
}

void impuls::system_asset::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;
	in_context;
}

impuls::asset_header* impuls::state_assetsystem::create_asset_internal(const std::string& in_asset_name, const std::string& in_asset_directory, const std::string& in_typename)
{
	m_asset_headers.push_back(std::make_unique<asset_header>(*this));
	asset_header* new_header = m_asset_headers.back().get();

	new_header->m_id = xg::newGuid();
	new_header->m_name = in_asset_name;
	new_header->m_asset_path = fmt::format("{0}/{1}.asset", in_asset_directory, in_asset_name);
	new_header->m_load_state = e_asset_load_state::loaded;
	new_header->m_typename = in_typename;

	m_id_to_header[m_asset_headers.back()->m_id] = m_asset_headers.back().get();

	return new_header;
}

void impuls::state_assetsystem::save_asset_header(const asset_header& in_header, const std::string& in_typename)
{
	nlohmann::json header_json;

	header_json["type"] = in_typename;
	header_json["id"] = in_header.m_id.str();
	header_json["name"] = in_header.m_name;
	header_json["asset_path"] = in_header.m_asset_path;

	file_management::save_file(fmt::format("{0}_h", in_header.m_asset_path), header_json.dump(1, '\t'));
}