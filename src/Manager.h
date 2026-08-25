#pragma once

#include "LockData.h"

class Manager : public REX::TSingleton<Manager>
{
public:
	bool LoadLocks();
	void InitLockForms();

	std::string GetLockModel(const char* a_fallbackPath);
	std::string GetLockpickModel(const char* a_fallbackPath);

	const Lock::Sound* GetSounds();

private:
	void        Sanitize(const std::string& a_path);
	std::string GetModel(const char* a_fallbackPath, bool a_isLockPick);

	// members
	std::set<Lock::Variant, std::less<>> lockVariants{};
	const Lock::Sound*                   currentSound{ nullptr };
};
