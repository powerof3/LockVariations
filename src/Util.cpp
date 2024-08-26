#include "Util.h"

namespace util
{
	RE::FormID GetFormID(const std::string& a_str)
	{
		if (const auto splitID = string::split(a_str, "~"); splitID.size() == 2) {
			const auto  formID = string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			if (g_mergeMapperInterface) {
				const auto [mergedModName, mergedFormID] = g_mergeMapperInterface->GetNewFormID(modName.c_str(), formID);
				return RE::TESDataHandler::GetSingleton()->LookupFormID(mergedFormID, mergedModName);
			} else {
				return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
			}
		}
		if (string::is_only_hex(a_str, true)) {
			return string::to_num<RE::FormID>(a_str, true);
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str)) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}

	FormIDStr GetFormIDStr(const std::string& a_str, bool a_sanitizePath)
	{
		auto formID = GetFormID(a_str);
		if (formID != 0) {
			return formID;
		}
		return a_sanitizePath ? SanitizeTexture(a_str) : a_str;
	}

	std::string SanitizeTexture(const std::string& a_path)
	{
		auto path = string::tolower(a_path);

		path = srell::regex_replace(path, srell::regex("/+|\\\\+"), "\\");
		path = srell::regex_replace(path, srell::regex("^\\\\+"), "");
		path = srell::regex_replace(path, srell::regex(R"(.*?[^\s]textures\\|^textures\\)", srell::regex::icase), "");

		return path;
	}

	std::string SanitizeModel(const std::string& a_path)
	{
		auto path = string::tolower(a_path);

		path = srell::regex_replace(path, srell::regex("/+|\\\\+"), "\\");
		path = srell::regex_replace(path, srell::regex("^\\\\+"), "");
		path = srell::regex_replace(path, srell::regex(R"(.*?[^\s]meshes\\|^meshes\\)", srell::regex::icase), "");

		return path;
	}
}
