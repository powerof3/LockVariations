#pragma once

class Settings
{
public:
	static inline std::string_view defaultLock{ "Interface/Lockpicking/LockPickShiv01.nif"sv };
	static inline std::string_view defaultLockPick{ "Interface/Lockpicking/LockPick01.nif"sv };
	static inline std::string_view skeletonKey{ "Interface/Lockpicking/LockPickSkeletonKey01.nif"sv };

	struct Lock
	{
		std::string model{ defaultLock };
		std::string waterModel{ defaultLock };
		std::string snowModel{ defaultLock };
	};

	struct LockData
	{
		std::string lockpickModel{ defaultLockPick };

		Lock chest;
		Lock door;
	};

	struct SoundData
	{
		std::string UILockpickingCylinderSqueakA{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderSqueakB{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderStop{ "UILockpickingCylinderStop" };
		std::string UILockpickingCylinderTurn{ "UILockpickingCylinderTurn" };
		std::string UILockpickingPickMovement{ "UILockpickingPickMovement" };
		std::string UILockpickingUnlock{ "UILockpickingUnlock" };
	};

	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	bool Load()
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

				if (const auto it = lockDataMap.find(section); it == lockDataMap.end()) {
					lockDataMap.emplace(section, lock);
				} else {
					auto& existingLock = it->second;
					detail::fill_default_lock(existingLock, lock);
				}

				soundDataMap.emplace(section, sound);
			}
		}

		return !lockDataMap.empty();
	}

	std::string GetLockModel(const char* a_fallbackPath)
	{
		//reset
		lockType = std::nullopt;
		std::optional<LockData> lockData = std::nullopt;

		const auto ref = RE::LockpickingMenu::GetTargetReference();
		const auto base = ref ? ref->GetBaseObject() : nullptr;
		const auto model = base ? base->As<RE::TESModel>() : nullptr;

		if (base && model) {
			auto it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
				return string::icontains(model->GetModel(), data.first);
			});
			if (it != lockDataMap.end()) {
				lockType = it->first;
				lockData = it->second;
			}
			if (!lockData) {
				if (detail::is_underwater(ref)) {
					if (it = lockDataMap.find("Underwater"); it != lockDataMap.end()) {
						lockType = it->first;
						lockData = it->second;
					}
				} else if (detail::has_snow(model)) {
					if (it = lockDataMap.find("IceCastle"); it != lockDataMap.end()) {
						lockType = it->first;
						lockData = it->second;
					}
				}
			}
			if (lockType && lockData) {
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

	std::string GetLockpickModel(const char* a_fallbackPath)
	{
		//reset
		lockType = std::nullopt;
		std::optional<LockData> lockData = std::nullopt;

		std::string path(a_fallbackPath);

		if (path == skeletonKey) {
			return path;
		}

		const auto ref = RE::LockpickingMenu::GetTargetReference();
		const auto base = ref ? ref->GetBaseObject() : nullptr;
		const auto model = base ? base->As<RE::TESModel>() : nullptr;

		if (base && model) {
			const auto it = std::ranges::find_if(lockDataMap, [&](const auto& data) {
				return string::icontains(model->GetModel(), data.first);
			});
			if (it != lockDataMap.end()) {
				lockType = it->first;
				lockData = it->second;
			}
			if (lockType && lockData) {
				path = lockData->lockpickModel;
			}
		}

		return path;
	}

	std::optional<SoundData> GetSoundData()
	{
		if (lockType) {
			return soundDataMap[*lockType];
		}
		return std::nullopt;
	}

private:
	struct detail
	{
		static void get_value(const CSimpleIniA& a_ini, std::string& a_value, const char* a_section, const char* a_key, const std::string& a_default = std::string())
		{
			a_value = a_ini.GetValue(a_section, a_key, a_default.empty() ? a_value.c_str() : a_default.c_str());
		}

		static void fill_default_lock(LockData& oldLock, const LockData& newLock)
		{
			if (oldLock.chest.model == defaultLock) {
				oldLock.chest.model = newLock.chest.model;
			}
			if (oldLock.chest.waterModel == defaultLock) {
				oldLock.chest.waterModel = newLock.chest.waterModel;
			}
			if (oldLock.chest.snowModel == defaultLock) {
				oldLock.chest.snowModel = newLock.chest.snowModel;
			}

			if (oldLock.door.model == defaultLock) {
				oldLock.door.model = newLock.door.model;
			}
			if (oldLock.door.waterModel == defaultLock) {
				oldLock.door.waterModel = newLock.door.waterModel;
			}
			if (oldLock.door.snowModel == defaultLock) {
				oldLock.door.snowModel = newLock.door.snowModel;
			}

			if (oldLock.lockpickModel == defaultLockPick) {
				oldLock.lockpickModel = newLock.lockpickModel;
			}
		}

		static bool has_snow(RE::TESModel* a_model)
		{
			const auto modelSwap = a_model->GetAsModelTextureSwap();
			if (modelSwap && modelSwap->alternateTextures) {
				std::span span(modelSwap->alternateTextures, modelSwap->numAlternateTextures);
				for (const auto& texture : span) {
					const auto txst = texture.textureSet;
					if (txst && string::icontains(txst->textures[RE::BSTextureSet::Texture::kDiffuse].textureName, "snow"sv)) {
						return true;
					}
				}
			}
			return false;
		}

		static bool is_underwater(const RE::TESObjectREFR* a_ref)
		{
			const auto waterLevel = a_ref->GetSubmergedWaterLevel(a_ref->GetPositionZ(), a_ref->GetParentCell());
			return waterLevel >= 0.875f;
		}
	};

	std::map<std::string, LockData> lockDataMap;
	std::map<std::string, SoundData> soundDataMap;

	std::optional<std::string> lockType;
};
