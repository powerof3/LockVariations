#include "Manager.h"

bool Manager::Load()
{
	logger::info("{:*^30}", "INI");

    std::vector<std::string> configs;

	constexpr auto suffix = "_LID"sv;

	auto constexpr folder = R"(Data\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
			if (const auto path = entry.path().string(); path.rfind(suffix) != std::string::npos) {
				configs.push_back(path);
			}
		}
	}

	if (configs.empty()) {
		logger::warn("	No .ini files with {} suffix were found within the Data folder, aborting...", suffix);
		return false;
	}

	logger::info("	{} matching inis found", configs.size());

	std::ranges::sort(configs);

	for (auto& path : configs) {
		logger::info("		INI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("	couldn't read INI");
			continue;
		}

		CSimpleIniA::TNamesDepend sections;
		ini.GetAllSections(sections);
		sections.sort(CSimpleIniA::Entry::LoadOrder());

		for (auto& [section, comment, order] : sections) {
			Data::LockType lockType{};
			Data::LockSet lock{};
			Data::Sound sound{};

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
			}

			if (const auto it = lockDataMap.find(lockType); it == lockDataMap.end()) {
				lockDataMap.emplace(lockType, lock);
			} else {
				it->second = lock;
			}

			soundDataMap.emplace(lockType, sound);
		}
	}

	logger::info("{:*^30}", "RESULTS");

	logger::info("{} lock types found", lockDataMap.size());

	return !lockDataMap.empty();
}

std::string Manager::GetLockModel(const char* a_fallbackPath)
{
	//reset
	currentLockType = std::nullopt;
	std::optional<Data::LockSet> lockData = std::nullopt;

	const auto ref = RE::LockpickingMenu::GetTargetReference();
	const auto base = ref ? ref->GetBaseObject() : nullptr;
	const auto model = base ? base->As<RE::TESModel>() : nullptr;

	if (base && model) {
		const auto get_matching_lock = [&](const Data::LockType& a_lockType) {
			auto& [lockModel, locationID] = a_lockType;
			if (string::icontains(model->GetModel(), lockModel)) {
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
			const auto get_matching_lock_by_model = [&](const Data::LockType& a_lockType, std::string_view a_model) {
				return a_lockType.modelPath == a_model;
			};

			if (detail::is_underwater()) {
				it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
					return get_matching_lock_by_model(data.first, "Underwater"sv);
				});
			} else if (detail::has_snow(model)) {
				it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
					return get_matching_lock_by_model(data.first, "IceCastle"sv);
				});
			}
			if (it != lockDataMap.end()) {
				currentLockType = it->first;
				lockData = it->second;
			}
		}

		if (currentLockType && lockData) {
			const auto isDoor = base->Is(RE::FormType::Door);
			if (detail::is_underwater()) {
			    return isDoor ?
				           lockData->door.waterModel :
                           lockData->chest.waterModel;
			}
			if (detail::has_snow(model)) {
				return isDoor ?
				           lockData->door.snowModel :
                           lockData->chest.snowModel;
			}
			return isDoor ?
			           lockData->door.model :
                       lockData->chest.model;
		}
	}

	return a_fallbackPath;
}

std::string Manager::GetLockpickModel(const char* a_fallbackPath)
{
	//reset
	std::optional<Data::LockSet> lockData = std::nullopt;

	std::string path(a_fallbackPath);

	if (path == Data::skeletonKey) {
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

std::optional<Data::Sound> Manager::GetSoundData()
{
	if (currentLockType) {
		return soundDataMap[*currentLockType];
	}
	return std::nullopt;
}
