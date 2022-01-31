#pragma once

class Settings
{
public:
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

	bool Load();

	std::string GetLockModel(const char* a_fallbackPath);
	std::string GetLockpickModel(const char* a_fallbackPath);

	std::optional<SoundData> GetSoundData();

private:
	static inline std::string_view defaultLock{ "Interface/Lockpicking/LockPickShiv01.nif"sv };
	static inline std::string_view defaultLockPick{ "Interface/Lockpicking/LockPick01.nif"sv };
	static inline std::string_view skeletonKey{ "Interface/Lockpicking/LockPickSkeletonKey01.nif"sv };

    struct LockType
	{
		std::string modelPath{};
		std::string locationID{};

		bool operator<(const LockType& a_rhs) const
		{
			if (modelPath != a_rhs.modelPath) {
				return modelPath < a_rhs.modelPath;
			}
			return locationID > a_rhs.locationID; //biggest to smallest/empty
		}
	};

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
					if (const auto txst = texture.textureSet; txst && string::icontains(txst->textures[RE::BSTextureSet::Texture::kDiffuse].textureName, "snow"sv)) {
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

	std::map<LockType, LockData> lockDataMap;
	std::map<LockType, SoundData> soundDataMap;

	std::optional<LockType> currentLockType;
};
