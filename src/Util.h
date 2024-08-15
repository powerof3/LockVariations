#pragma once

namespace regex
{
	inline srell::regex generic{ R"(\((.*?)\))" };                // pos(0,0,100) -> "0,0,100"
	inline srell::regex transform{ R"(\((.*?),(.*?),(.*?)\))" };  // pos(0,0,100) -> 0, 0, 100
	inline srell::regex string{ R"(,\s*(?![^()]*\)))" };          // pos(0, 0, 100), rot(0, 0, 100) -> "pos(0, 0, 100)","rot(0, 0, 100)"
}

namespace util
{
	RE::FormID  GetFormID(const std::string& a_str);
	FormIDStr   GetFormIDStr(const std::string& a_str, bool a_sanitizePath = false);
	std::string SanitizeModel(const std::string& a_path);
	std::string SanitizeTexture(const std::string& a_path);
}
