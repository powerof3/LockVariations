#include "LockData.h"

namespace Lock
{
	RE::FormID detail::GetFormID(const std::string& a_str)
	{
		if (const auto splitID = string::split(a_str, "~"); splitID.size() == 2) {
			const auto  formID = string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			if (g_mergeMapperInterface) {
				const auto [mergedModName, mergedFormID] = g_mergeMapperInterface->GetNewFormID(modName.c_str(), formID);
				return RE::TESDataHandler::GetSingleton()->LookupFormID(mergedFormID, mergedModName);
			} else {
				return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
			}
		}
		if (string::is_only_hex(a_str, true)) {
			return string::to_num<RE::FormID>(a_str, true);
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str)) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}

	FormIDStr detail::GetFormIDStr(const std::string& a_str)
	{
		auto formID = GetFormID(a_str);
		if (formID != 0) {
			return formID;
		}
		return a_str;
	}

	Type::Type(const std::string& a_section)
	{
		if (a_section.empty()) {
			return;
		}

		if (auto lockTypeStrs = string::split(a_section, "|"); lockTypeStrs.size() > 1) {
			modelPath = lockTypeStrs[0];
			locationID = detail::GetFormID(lockTypeStrs[1]);
		} else {
			modelPath = lockTypeStrs[0];
		}
	}

	bool Type::IsValid(const ConditionChecker& a_checker) const
	{
		if (!modelPath.empty() && !string::icontains(a_checker.modelPath, modelPath)) {
			return false;
		}

		if (locationID != 0 && a_checker.location) {
			const auto loc = RE::TESForm::LookupByID<RE::BGSLocation>(locationID);
			return loc && (loc == a_checker.location || a_checker.location->IsParent(loc));
		}

		return true;
	}

	Sound::Sound(CSimpleIniA& a_ini, const std::string& a_section)
	{
		if (a_section.empty()) {
			return;
		}

		ini::get_value(a_ini, UILockpickingCylinderSqueakA, a_section.c_str(), "CylinderSqueakA");
		ini::get_value(a_ini, UILockpickingCylinderSqueakB, a_section.c_str(), "CylinderSqueakB");
		ini::get_value(a_ini, UILockpickingCylinderStop, a_section.c_str(), "CylinderStop");
		ini::get_value(a_ini, UILockpickingCylinderTurn, a_section.c_str(), "CylinderTurn");
		ini::get_value(a_ini, UILockpickingPickMovement, a_section.c_str(), "PickMovement");
		ini::get_value(a_ini, UILockpickingUnlock, a_section.c_str(), "LockpickingUnlock");
	}

	Model::Condition::Condition(const std::string& a_id, const std::string& a_flags)
	{
		if (dist::is_valid_entry(a_id)) {
			auto vec = string::split(a_id, ",");
			for (auto& id : vec) {
				ids.push_back(detail::GetFormIDStr(id));
			}
		}
		if (dist::is_valid_entry(a_flags)) {
			if (a_flags == "underwater") {
				flags = Flags::kUnderwater;
			}
		}
	}

	bool Model::Condition::IsValid(const ConditionChecker& a_checker) const
	{
		bool result = false;

		result = std::any_of(ids.begin(), ids.end(), [&](auto& id) {
			bool isValid = false;
			std::visit(overload{
						   [&](RE::FormID a_formID) {
							   isValid = IsValidImpl(a_checker, a_formID);
						   },
						   [&](const std::string& a_edid) {
							   isValid = IsValidImpl(a_checker, a_edid);
						   } },
				id);
			return isValid;
		});

		if (result) {
			if (flags == Flags::kUnderwater) {
				result = RE::TESWaterSystem::GetSingleton()->playerUnderwater;
			}
		}

		return result;
	}

	bool Model::Condition::IsValidImpl(const ConditionChecker& a_checker, RE::FormID a_formID)
	{
		if (const auto form = RE::TESForm::LookupByID(a_formID)) {
			switch (form->GetFormType()) {
			case RE::FormType::TextureSet:
				{
					for (auto& [diffuse, textureset] : a_checker.textureSet) {
						if (textureset == form) {
							return true;
						}
					}
					return false;
				}
			case RE::FormType::Door:
			case RE::FormType::Container:
				return a_checker.base == form;
			default:
				break;
			}
		}

		return false;
	}

	bool Model::Condition::IsValidImpl(const ConditionChecker& a_checker, const std::string& a_edid)
	{
		for (auto& [diffuse, textureset] : a_checker.textureSet) {
			if (string::icontains(diffuse, a_edid)) {
				return true;
			}
		}

		return false;
	}

	Model::Model(const std::string& key, const std::string& entry) :
		model(entry)
	{
		auto vec = string::split(key, "|");
		if (vec.size() > 1) {
			condition = Condition(
				vec[1],
				vec.size() > 2 ? vec[2] : std::string());
		}
	}

	ConditionChecker::ConditionChecker(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base, RE::TESModel* a_model) :
		base(a_base),
		location(a_ref->GetCurrentLocation()),
		modelPath(a_model->GetModel())
	{
		if (const auto modelSwap = a_model->GetAsModelTextureSwap(); modelSwap && modelSwap->alternateTextures && modelSwap->numAlternateTextures > 0) {
			std::span span(modelSwap->alternateTextures, modelSwap->numAlternateTextures);
			for (auto& txst : span) {
				textureSet.emplace_back(txst.textureSet->textures[RE::BSTextureSet::Texture::kDiffuse].textureName.c_str(), txst.textureSet);
			}
		}
	}

	std::tuple<bool, std::string, Sound> ConditionChecker::IsValid(const Variant& a_variant, bool a_isLockPick) const
	{
		if (a_variant.type.IsValid(*this)) {
			const auto& models = a_isLockPick ? a_variant.lockpicks : GetModels(a_variant);
			for (auto& model : models) {
				if (!model.condition || model.condition->IsValid(*this)) {
					if (model.model != (a_isLockPick ? defaultLockPick : defaultLock)) {
						return { true, model.model, a_variant.sounds };
					}
				}
			}
		}
		return { false, "", Sound() };
	}

	const std::vector<Model>& ConditionChecker::GetModels(const Variant& a_variant) const
	{
		switch (base->GetFormType()) {
		case RE::FormType::Door:
			return a_variant.doors;
		default:
			return a_variant.chests;
		}
	}

	void Variant::SortModels()
	{
		//shift conditional models to top
		std::ranges::stable_partition(chests, [](const Lock::Model& a_lhs) {
			return a_lhs.condition.has_value();
		});
		std::ranges::stable_partition(doors, [](const Lock::Model& a_lhs) {
			return a_lhs.condition.has_value();
		});
		std::ranges::stable_partition(lockpicks, [](const Lock::Model& a_lhs) {
			return a_lhs.condition.has_value();
		});
	}
}
