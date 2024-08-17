#pragma once

#include "LockData.h"

class Manager : public ISingleton<Manager>
{
public:
	bool LoadLocks();
	void InitLockForms();

	std::string GetLockModel(const char* a_fallbackPath);
	std::string GetLockpickModel(const char* a_fallbackPath);

	const std::optional<Lock::Sound>& GetSounds();

private:
	void Sanitize(const std::string& a_path);
	
	// members
	std::set<Lock::Variant, std::less<>> lockVariants{};
	std::optional<Lock::Sound>           currentSound{};
};
