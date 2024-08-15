#pragma once

namespace util
{
	RE::FormID  GetFormID(const std::string& a_str);
	FormIDStr   GetFormIDStr(const std::string& a_str, bool a_sanitizePath = false);
	std::string SanitizeModel(const std::string& a_path);
	std::string SanitizeTexture(const std::string& a_path);
}
