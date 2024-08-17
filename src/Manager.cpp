#include "Manager.h"

bool Manager::LoadLocks()
{
	logger::info("{:*^30}", "INI");

	std::vector<std::string> configs = dist::get_configs(R"(Data\)", "_LID"sv);

	if (configs.empty()) {
		logger::warn("\tNo .ini files with _LID suffix were found within the Data folder, aborting...");
		return false;
	}

	logger::info("{} matching inis found", configs.size());

	std::ranges::sort(configs);

	for (auto& path : configs) {
		logger::info("INI : {}", path);

		Sanitize(path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("\tcouldn't read INI");
			continue;
		}

		CSimpleIniA::TNamesDepend sections;
		ini.GetAllSections(sections);
		sections.sort(CSimpleIniA::Entry::LoadOrder());

		for (auto& [_section, comment, order] : sections) {
			std::string section = _section;
			if (auto it = lockVariants.find(section); it != lockVariants.end()) {
				auto node = lockVariants.extract(it);
				node.value().AddModels(ini, section);
				lockVariants.insert(std::move(node));
			} else {
				lockVariants.emplace(ini, section);
			}
		}
	}

	return !lockVariants.empty();
}

void Manager::InitLockForms()
{
	logger::info("{:*^30}", "DATA LOAD");

	for (auto it = lockVariants.begin(); it != lockVariants.end(); it++) {
		auto node = lockVariants.extract(it);
		node.value().InitForms();
		node.value().SortModels();
		lockVariants.insert(std::move(node));
	}

	logger::info("Loaded {} lock entries", lockVariants.size());
	logger::info("{:*^30}", "INFO");
}

// hack
void Manager::Sanitize(const std::string& a_path)
{
	std::fstream input(a_path);
	if (!input.good()) {
		return;
	}

	std::string             line;
	std::deque<std::string> processedLines;
	bool                    firstLine = true;

	bool                     underwater = false;
	bool                     finishedUnderWater = false;
	std::vector<std::string> underWaterLines;

	constexpr unsigned char boms[]{ 0xef, 0xbb, 0xbf };
	bool                    have_bom{ true };
	for (const auto& c : boms) {
		if ((unsigned char)input.get() != c) {
			have_bom = false;
		}
	}
	if (!have_bom) {
		input.seekg(0);
	}

	while (std::getline(input, line)) {
		if (firstLine) {
			if (line.starts_with(";3.30")) {
				return;
			}
			firstLine = false;
		}
		if (line.contains('[')) {
			string::replace_first_instance(line, ":", "|");
		}
		if (underwater) {
			if (line.contains("Door")) {
				string::replace_first_instance(line, "Door", "Door|NONE|underwater");
				underWaterLines.push_back(line);
			}
			if (line.contains("Chest")) {
				string::replace_first_instance(line, "Chest", "Chest|NONE|underwater");
				underWaterLines.push_back(line);
				finishedUnderWater = true;
			}
		}
		if (line.contains("[Underwater]")) {
			underwater = true;
		} else if (!underwater) {
			processedLines.push_back(line);
		} else {
			if (finishedUnderWater) {
				underwater = false;
			}
		}
	}

	if (!underWaterLines.empty()) {
		processedLines.push_front(underWaterLines[1] + "\n");
		processedLines.push_front(underWaterLines[0]);
	}
	processedLines.push_front(";3.30");

	std::ofstream output(a_path);
	std::ranges::copy(processedLines, std::ostream_iterator<std::string>(output, "\n"));
}

std::string Manager::GetLockModel(const char* a_fallbackPath)
{
	//reset
	currentSound = std::nullopt;

	const auto ref = RE::LockpickingMenu::GetTargetReference();
	const auto base = ref ? ref->GetBaseObject() : nullptr;
	const auto model = base ? base->As<RE::TESModel>() : nullptr;

	if (ref && base && model) {
		Lock::ConditionChecker checker(ref, base, model);
		for (auto& variant : lockVariants) {
			auto [result, modelPath, sounds] = checker.IsValid(variant, false);
			if (result) {
				currentSound = sounds;
				return modelPath;
			}
		}
	}

	return a_fallbackPath;
}

std::string Manager::GetLockpickModel(const char* a_fallbackPath)
{
	std::string path(a_fallbackPath);

	if (path == Lock::skeletonKey) {
		return path;
	}

	const auto ref = RE::LockpickingMenu::GetTargetReference();
	const auto base = ref ? ref->GetBaseObject() : nullptr;
	const auto model = base ? base->As<RE::TESModel>() : nullptr;

	if (ref && base && model) {
		Lock::ConditionChecker checker(ref, base, model);
		for (auto& variant : lockVariants) {
			auto [result, modelPath, sounds] = checker.IsValid(variant, true);
			if (result) {
				return modelPath;
			}
		}
	}

	return path;
}

const std::optional<Lock::Sound>& Manager::GetSounds()
{
	return currentSound;
}
