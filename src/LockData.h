#pragma once

namespace Lock
{
	inline std::string_view defaultLock{ "Interface/Lockpicking/LockPickShiv01.nif"sv };
	inline std::string_view defaultLockPick{ "Interface/Lockpicking/LockPick01.nif"sv };
	inline std::string_view skeletonKey{ "Interface/Lockpicking/LockPickSkeletonKey01.nif"sv };

	struct detail
	{
		static RE::FormID GetFormID(const std::string& a_str);
		static FormIDStr  GetFormIDStr(const std::string& a_str);
	};

	struct ConditionChecker;

	struct Type
	{
		Type() = default;
		Type(const std::string& a_section);

		[[nodiscard]] bool IsValid(const ConditionChecker& a_checker) const;

		// members
		std::string modelPath{};
		RE::FormID  locationID{ 0 };
	};

	struct Sound
	{
		Sound() = default;
		Sound(CSimpleIniA& a_ini, const std::string& a_section);

		// members
		std::string UILockpickingCylinderSqueakA{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderSqueakB{ "UILockpickingCylinderSqueakA" };
		std::string UILockpickingCylinderStop{ "UILockpickingCylinderStop" };
		std::string UILockpickingCylinderTurn{ "UILockpickingCylinderTurn" };
		std::string UILockpickingPickMovement{ "UILockpickingPickMovement" };
		std::string UILockpickingUnlock{ "UILockpickingUnlock" };
	};

	struct Model
	{
		Model() = default;
		Model(std::string_view a_model) :
			model(a_model){};
		Model(const std::string& key, const std::string& entry);

		struct Condition
		{
			enum class Flags
			{
				kNone = 0,
				kUnderwater = 1
			};

			Condition(const std::string& a_id, const std::string& a_flags);

			[[nodiscard]] bool IsValid(const ConditionChecker& a_checker) const;

			[[nodiscard]] static bool IsValidImpl(const ConditionChecker& a_checker, RE::FormID a_formID);
			[[nodiscard]] static bool IsValidImpl(const ConditionChecker& a_checker, const std::string& a_edid);

			// members
			FormIDStr id{};  // textureset/chest/door
			Flags     flags{ Flags::kNone };
		};

		// members
		std::optional<Condition> condition{};
		std::string              model{ defaultLock };
	};

	struct Variant
	{	
		// members
		Type               type{};
		std::vector<Model> chests{};
		std::vector<Model> doors{};
		std::vector<Model> lockpicks{};
		Sound              sounds{};
	};

	struct ConditionChecker
	{
		struct Texture
		{
			std::string        diffuse{};
			RE::BGSTextureSet* txst{};
		};

		ConditionChecker(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base, RE::TESModel* a_model);

		[[nodiscard]] std::tuple<bool, std::string, Sound> IsValid(const Variant& a_variant, bool a_isLockPick) const;
		[[nodiscard]] const std::vector<Model>&            GetModels(const Variant& a_variant) const;

		// members
		RE::TESBoundObject*  base{};
		RE::BGSLocation*     location{};
		std::string          modelPath{};
		std::vector<Texture> textureSet;
	};
}
