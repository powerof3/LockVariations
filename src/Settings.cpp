#include "Settings.h"

bool Settings::Load()
{
	std::vector<std::string> configs;

	auto constexpr folder = R"(Data\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
			if (const auto path = entry.path().string(); path.find("_LID") != std::string::npos) {
				configs.push_back(path);
			}
		}
	}

	if (configs.empty()) {
		logger::warn("	No .ini files with _LID suffix were found within the Data folder, aborting...");
		return false;
	}

	logger::info("	{} matching inis found", configs.size());

	for (auto& path : configs) {
		logger::info("		INI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("	couldn't read INI");
			continue;
		}

		std::list<CSimpleIniA::Entry> list;
		ini.GetAllSections(list);
		for (auto& [section, comment, order] : list) {
			LockType lockType{};
		    LockData lock{};
			SoundData sound{};

			detail::get_value(ini, lock.door.model, section, "Door");
			detail::get_value(ini, lock.door.waterModel, section, "Door [Water]", lock.door.model);
			detail::get_value(ini, lock.door.snowModel, section, "Door [Snow]", lock.door.model);

			detail::get_value(ini, lock.chest.model, section, "Chest");
			detail::get_value(ini, lock.chest.waterModel, section, "Chest [Water]", lock.chest.model);
			detail::get_value(ini, lock.chest.snowModel, section, "Chest [Snow]", lock.chest.model);

			detail::get_value(ini, lock.lockpickModel, section, "Lockpick");

			detail::get_value(ini, sound.UILockpickingCylinderSqueakA, section, "CylinderSqueakA");
			detail::get_value(ini, sound.UILockpickingCylinderSqueakB, section, "CylinderSqueakB");
			detail::get_value(ini, sound.UILockpickingCylinderStop, section, "CylinderStop");
			detail::get_value(ini, sound.UILockpickingCylinderTurn, section, "CylinderTurn");
			detail::get_value(ini, sound.UILockpickingPickMovement, section, "PickMovement");
			detail::get_value(ini, sound.UILockpickingUnlock, section, "LockpickingUnlock");

			if (auto lockTypeStrs = string::split(section, ":"); lockTypeStrs.size() > 1) {
				lockType.modelPath = lockTypeStrs[0];
				lockType.locationID = lockTypeStrs[1];
			} else {
				lockType.modelPath = lockTypeStrs[0];
				lockType.locationID = std::string();
			}

			if (const auto it = lockDataMap.find(lockType); it == lockDataMap.end()) {
				lockDataMap.emplace(lockType, lock);
			} else {
				auto& existingLock = it->second;
				detail::fill_default_lock(existingLock, lock);
			}

			soundDataMap.emplace(lockType, sound);
		}
	}

	return !lockDataMap.empty();
}

std::string Settings::GetLockModel(const char* a_fallbackPath)
{
	//reset
	currentLockType = std::nullopt;
	std::optional<LockData> lockData = std::nullopt;

	const auto ref = RE::LockpickingMenu::GetTargetReference();
	const auto base = ref ? ref->GetBaseObject() : nullptr;
	const auto model = base ? base->As<RE::TESModel>() : nullptr;

	if (base && model) {
		const auto get_matching_lock = [&](const LockType& a_lockType) {
			auto& [lockPath, locationID] = a_lockType;
			if (string::icontains(model->GetModel(), lockPath)) {
				if (!locationID.empty()) {
					const auto loc = RE::TESForm::LookupByEditorID<RE::BGSLocation>(locationID);
				    const auto currentLoc = ref->GetCurrentLocation();

					return loc && currentLoc && (loc == currentLoc || currentLoc->IsParent(loc));
				}
                return true;
			}
			return false;
		};

		auto it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
			return get_matching_lock(data.first);
		});
		if (it != lockDataMap.end()) {
			currentLockType = it->first;
			lockData = it->second;
		}

		if (!lockData) {
			const auto get_matching_lock_by_model = [&](const LockType& a_lockType, std::string_view a_model) {
				auto& [lockPath, locationID] = a_lockType;
				return lockPath == a_model;
			};

			if (detail::is_underwater(ref)) {
				it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
					return get_matching_lock_by_model(data.first, "Underwater");
				});
			} else if (detail::has_snow(model)) {
				it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
					return get_matching_lock_by_model(data.first, "IceCastle");
				});
			}
			if (it != lockDataMap.end()) {
				currentLockType = it->first;
				lockData = it->second;
			}
		}

		if (currentLockType && lockData) {
			const auto isDoor = base->Is(RE::FormType::Door);
			if (detail::is_underwater(ref)) {
				return isDoor ? lockData->door.waterModel : lockData->chest.waterModel;
			}
			if (detail::has_snow(model)) {
				return isDoor ? lockData->door.snowModel : lockData->chest.snowModel;
			}
			return isDoor ? lockData->door.model : lockData->chest.model;
		}
	}

	return a_fallbackPath;
}

std::string Settings::GetLockpickModel(const char* a_fallbackPath)
{
	//reset
	std::optional<LockData> lockData = std::nullopt;

	std::string path(a_fallbackPath);

	if (path == skeletonKey) {
		return path;
	}

	const auto ref = RE::LockpickingMenu::GetTargetReference();
	const auto base = ref ? ref->GetBaseObject() : nullptr;
	const auto model = base ? base->As<RE::TESModel>() : nullptr;

	if (base && model) {
		if (const auto it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
				auto& [lockPath, locationID] = data.first;
				return string::icontains(model->GetModel(), lockPath);
			});
			it != lockDataMap.end()) {
			lockData = it->second;
		}
		if (lockData) {
			path = lockData->lockpickModel;
		}
	}

	return path;
}

std::optional<Settings::SoundData> Settings::GetSoundData()
{
	if (currentLockType) {
		return soundDataMap[*currentLockType];
	}
	return std::nullopt;
}
