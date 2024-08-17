#include "LockData.h"

namespace Lock
{
	Type::Type(const std::string& a_section)
	{
		if (a_section.empty()) {
			return;
		}

		auto lockTypeStrs = string::split(a_section, "|");

		modelPath = util::SanitizeModel(lockTypeStrs[0]);
		if (lockTypeStrs.size() > 1) {
			locationStr = lockTypeStrs[1];
		}
	}

	void Type::InitLocation()
	{
		if (!locationStr.empty()) {
			locationID = util::GetFormID(locationStr);
		}
	}

	bool Type::IsValid(const ConditionChecker& a_checker) const
	{
		if (!modelPath.empty() && !a_checker.modelPath.contains(modelPath)) {
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
				ids.push_back(id);
			}
		}
		if (dist::is_valid_entry(a_flags)) {
			if (a_flags == "underwater") {
				flags = Flags::kUnderwater;
			}
		}
	}

	void Model::Condition::InitForms()
	{
		for (auto& id : ids) {
			id = util::GetFormIDStr(std::get<std::string>(id), true);
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

		if (flags == Flags::kUnderwater) {
			result = RE::TESWaterSystem::GetSingleton()->playerUnderwater;
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

	bool Model::Condition::IsValidImpl(const ConditionChecker& a_checker, const std::string& a_path)
	{
		for (auto& [diffuse, textureset] : a_checker.textureSet) {
			if (diffuse.contains(a_path)) {
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

	void Model::InitForms()
	{
		if (condition) {
			condition->InitForms();
		}
	}

	ConditionChecker::ConditionChecker(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base, RE::TESModel* a_model) :
		base(a_base),
		location(a_ref->GetCurrentLocation()),
		modelPath(util::SanitizeModel(a_model->GetModel()))
	{
		if (const auto modelSwap = a_model->GetAsModelTextureSwap(); modelSwap && modelSwap->alternateTextures && modelSwap->numAlternateTextures > 0) {
			std::span span(modelSwap->alternateTextures, modelSwap->numAlternateTextures);
			for (auto& txst : span) {
				auto diffuse = util::SanitizeTexture(txst.textureSet->textures[RE::BSTextureSet::Texture::kDiffuse].textureName.c_str());
				textureSet.emplace_back(std::move(diffuse), txst.textureSet);
			}
		}
	}

	std::tuple<bool, std::string, Sound> ConditionChecker::IsValid(const Variant& a_variant, bool a_isLockPick) const
	{
		if (a_variant.type.IsValid(*this)) {
			const auto& models = a_isLockPick ? a_variant.lockpicks : GetModels(a_variant);
			for (const auto& model : models) {
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

	Variant::Variant(CSimpleIniA& a_ini, const std::string& a_section) :
		type(a_section),
		sounds(a_ini, a_section)
	{
		AddModels(a_ini, a_section);
	}

	void Variant::AddModels(CSimpleIniA& a_ini, const std::string& a_section)
	{
		if (auto values = a_ini.GetSection(a_section.c_str()); values && !values->empty()) {
			for (auto& [key, entry] : *values) {
				AddModels(key.pItem, entry);
			}
		}
	}

	void Variant::AddModels(const std::string& a_key, const std::string& a_entry)
	{
		if (a_key.starts_with("Chest")) {
			chests.emplace_back(a_key, a_entry);
		} else if (a_key.starts_with("Door")) {
			doors.emplace_back(a_key, a_entry);
		} else if (a_key.starts_with("Lockpick") && a_key != "LockpickingUnlock") {
			lockpicks.emplace_back(a_key, a_entry);
		}
	}

	void Variant::SortModels()
	{
		//shift conditional models to top
		ForEachModelType([&](std::vector<Lock::Model>& models) {
			std::ranges::stable_partition(models, [](const Lock::Model& model) {
				return model.condition.has_value();
			});
		});
		// give conditional models + flags highest priority
		ForEachModelType([&](std::vector<Lock::Model>& models) {
			std::ranges::stable_partition(models, [](const Lock::Model& model) {
				return model.condition.has_value() && model.condition->flags != Model::Condition::Flags::kNone;
			});
		});
	}

	void Variant::InitForms()
	{
		type.InitLocation();

		ForEachModelType([&](std::vector<Lock::Model>& models) {
			for (auto& model : models) {
				model.InitForms();
			}
		});
	}
}
