#pragma once

namespace Data
{
	inline std::string defaultLock{ "Interface/Lockpicking/LockPickShiv01.nif"sv };
	inline std::string defaultLockPick{ "Interface/Lockpicking/LockPick01.nif"sv };
	inline std::string skeletonKey{ "Interface/Lockpicking/LockPickSkeletonKey01.nif"sv };

	struct Sound
	{
		std::string UILockpickingCylinderSqueakA{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderSqueakB{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderStop{ "UILockpickingCylinderStop" };
		std::string UILockpickingCylinderTurn{ "UILockpickingCylinderTurn" };
		std::string UILockpickingPickMovement{ "UILockpickingPickMovement" };
		std::string UILockpickingUnlock{ "UILockpickingUnlock" };
	};

	struct LockType
	{
		std::string modelPath{};
		std::string locationID{};

		bool operator<(const LockType& a_rhs) const
		{
			if (modelPath != a_rhs.modelPath) {
				return modelPath < a_rhs.modelPath;
			}
			return locationID > a_rhs.locationID;  //biggest to smallest/empty
		}
	};

	struct Lock
	{
		std::string model{ defaultLock };
		std::string waterModel{ defaultLock };
		std::string snowModel{ defaultLock };

		Lock& operator=(const Lock& a_rhs)
		{
			if (model == defaultLock) {
				model = a_rhs.model;
			}
			if (waterModel == defaultLock) {
				waterModel = a_rhs.waterModel;
			}
			if (snowModel == defaultLock) {
				snowModel = a_rhs.snowModel;
			}
			return *this;
		}
	};

	struct LockSet
	{
		std::string lockpickModel{ defaultLockPick };

		Lock chest{};
		Lock door{};

		LockSet& operator=(const LockSet& a_rhs)
		{
			if (lockpickModel == defaultLockPick) {
				lockpickModel = a_rhs.lockpickModel;
			}

			chest = a_rhs.chest;
			door = a_rhs.door;

			return *this;
		}
	};
}

class Manager
{
public:
	[[nodiscard]] static Manager* GetSingleton()
	{
		static Manager singleton;
		return std::addressof(singleton);
	}

	bool Load();

	std::string GetLockModel(const char* a_fallbackPath);
	std::string GetLockpickModel(const char* a_fallbackPath);

	std::optional<Data::Sound> GetSoundData();

private:
    struct detail
	{
		static void get_value(const CSimpleIniA& a_ini, std::string& a_value, const char* a_section, const char* a_key, const std::string& a_default = std::string())
		{
			a_value = a_ini.GetValue(a_section, a_key, a_default.empty() ? a_value.c_str() : a_default.c_str());
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
			return a_ref->IsPointSubmergedMoreThan(a_ref->GetPosition(), a_ref->GetParentCell(), 0.875f);
		}
	};

	std::map<Data::LockType, Data::LockSet> lockDataMap;
	std::map<Data::LockType, Data::Sound> soundDataMap;

	std::optional<Data::LockType> currentLockType;
};
