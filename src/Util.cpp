#include "Util.h"

namespace util
{
	RE::FormID GetFormID(const std::string& a_str)
	{
		if (const auto splitID = REX::STR::SPLIT(a_str, "~"); splitID.size() == 2) {
			const auto  formID = REX::STR::TO_NUM<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			if (g_mergeMapperInterface) {
				const auto [mergedModName, mergedFormID] = g_mergeMapperInterface->GetNewFormID(modName.c_str(), formID);
				return RE::TESDataHandler::GetSingleton()->LookupFormID(mergedFormID, mergedModName);
			} else {
				return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
			}
		}
		if (REX::STR::IS_ONLY_HEX(a_str, true)) {
			return REX::STR::TO_NUM<RE::FormID>(a_str, true);
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
		static const boost::regex slashExpr("/+|\\\\+");
		static const boost::regex leadingSlashExpr("^\\\\+");
		static const boost::regex texturesExpr(R"(.*?[^\s]textures\\|^textures\\)", boost::regex::icase);

		auto path = REX::STR::TO_LOWER(a_path);

		path = boost::regex_replace(path, slashExpr, "\\");
		path = boost::regex_replace(path, leadingSlashExpr, "");
		path = boost::regex_replace(path, texturesExpr, "");

		return path;
	}

	std::string SanitizeModel(const std::string& a_path)
	{
		static const boost::regex slashExpr("/+|\\\\+");
		static const boost::regex leadingSlashExpr("^\\\\+");
		static const boost::regex meshesExpr(R"(.*?[^\s]meshes\\|^meshes\\)", boost::regex::icase);

		auto path = REX::STR::TO_LOWER(a_path);

		path = boost::regex_replace(path, slashExpr, "\\");
		path = boost::regex_replace(path, leadingSlashExpr, "");
		path = boost::regex_replace(path, meshesExpr, "");

		return path;
	}
}
