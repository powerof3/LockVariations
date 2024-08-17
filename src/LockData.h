#pragma once

#include "Util.h"

namespace Lock
{
	inline std::string_view defaultLock{ "Interface/Lockpicking/LockPickShiv01.nif"sv };
	inline std::string_view defaultLockPick{ "Interface/Lockpicking/LockPick01.nif"sv };
	inline std::string_view skeletonKey{ "Interface/Lockpicking/LockPickSkeletonKey01.nif"sv };

	struct ConditionChecker;

	struct Type
	{
		Type() = default;
		Type(const std::string& a_section);

		bool operator<(const Type& a_rhs) const
		{	
			if (modelPath.empty() && !a_rhs.modelPath.empty()) {
				return false;
			}
			if (!modelPath.empty() && a_rhs.modelPath.empty()) {
				return true;
			}
			if (modelPath != a_rhs.modelPath) {
				return modelPath < a_rhs.modelPath;
			}
			return locationStr > a_rhs.locationStr;  //biggest to smallest/empty
		}

		void               InitLocation();
		[[nodiscard]] bool IsValid(const ConditionChecker& a_checker) const;

		// members
		std::string modelPath{};

		RE::FormID  locationID{};
		std::string locationStr{};
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

			void               InitForms();
			[[nodiscard]] bool IsValid(const ConditionChecker& a_checker) const;

			[[nodiscard]] static bool IsValidImpl(const ConditionChecker& a_checker, RE::FormID a_formID);
			[[nodiscard]] static bool IsValidImpl(const ConditionChecker& a_checker, const std::string& a_path);

			// members
			std::vector<FormIDStr> ids{};  // textureset/chest/door
			Flags                  flags{ Flags::kNone };
		};

		void InitForms();

		// members
		std::optional<Condition> condition{};
		std::string              model{ defaultLock };
	};

	struct Variant
	{
		Variant(CSimpleIniA& a_ini, const std::string& a_section);

		void AddModels(CSimpleIniA& a_ini, const std::string& a_section);
		void AddModels(const std::string& a_key, const std::string& a_entry);
		void SortModels();
		void InitForms();

		template <typename Func, typename... Args>
		void ForEachModelType(Func&& func, Args&&... args)
		{
			func(chests, std::forward<Args>(args)...);
			func(doors, std::forward<Args>(args)...);
			func(lockpicks, std::forward<Args>(args)...);
		}

		// members
		Type               type{};
		std::vector<Model> chests{};
		std::vector<Model> doors{};
		std::vector<Model> lockpicks{};
		Sound              sounds{};
	};

	inline bool operator<(const Variant& a_lhs, const Type& a_rhs) { return a_lhs.type < a_rhs; }
	inline bool operator<(const Type& a_lhs, const Variant& a_rhs) { return a_lhs < a_rhs.type; }
	inline bool operator<(const Variant& a_lhs, const Variant& a_rhs) { return a_lhs.type < a_rhs.type; }

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
		std::vector<Texture> textureSet{};
	};
}
