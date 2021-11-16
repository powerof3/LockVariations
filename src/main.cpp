#include "Settings.h"

namespace Model
{
	struct RequestModel
	{
		static std::uint8_t thunk(const char* a_modelPath, std::uintptr_t a_unk02, std::uintptr_t a_unk03)
		{
			std::string modelPath = Settings::GetSingleton()->GetLockModel(a_modelPath);

			return func(modelPath.c_str(), a_unk02, a_unk03);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> target{ REL::Offset(0x8C7450) };
		stl::write_thunk_call<RequestModel>(target.address() + 0xBC);
	}
}

namespace Sound
{
	struct CylinderSqueak
	{
		static void thunk(const char* a_editorID)
		{
			std::string editorID = a_editorID;

			auto soundData = Settings::GetSingleton()->GetSoundData();
			if (soundData) {
				if (editorID == "UILockpickingCylinderSqueakA") {
					editorID = soundData->UILockpickingCylinderSqueakA;
				} else {
					editorID = soundData->UILockpickingCylinderSqueakB;
				}
			}

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct CylinderStop
	{
		static void thunk(const char* a_editorID)
		{
			auto soundData = Settings::GetSingleton()->GetSoundData();
			std::string editorID = soundData ? soundData->UILockpickingCylinderStop : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct CylinderTurn
	{
		static void thunk(const char* a_editorID)
		{
			auto soundData = Settings::GetSingleton()->GetSoundData();
			std::string editorID = soundData ? soundData->UILockpickingCylinderTurn : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PickMovement
	{
		static void thunk(const char* a_editorID)
		{
			auto soundData = Settings::GetSingleton()->GetSoundData();
			std::string editorID = soundData ? soundData->UILockpickingPickMovement : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct LockpickingUnlock
	{
		static void thunk(const char* a_editorID)
		{
			auto soundData = Settings::GetSingleton()->GetSoundData();
			std::string editorID = soundData ? soundData->UILockpickingUnlock : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> update_lock_angle{ REL::Offset(0x8C9620) };
		stl::write_thunk_call<CylinderSqueak>(update_lock_angle.address() + 0x157);
		stl::write_thunk_call<CylinderStop>(update_lock_angle.address() + 0xD7);
		stl::write_thunk_call<LockpickingUnlock>(update_lock_angle.address() + 0x9F);

		REL::Relocation<std::uintptr_t> rotate_lock{ REL::Offset(0x8C9940) };
		stl::write_thunk_call<CylinderTurn>(rotate_lock.address() + 0xBC);

		REL::Relocation<std::uintptr_t> update_pick_angle{ REL::Offset(0x8C9120) };
		stl::write_thunk_call<PickMovement>(update_pick_angle.address() + 0x145);
	}
}

extern "C" __declspec(dllexport) constexpr auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v{};
	v.pluginVersion = 2;
	v.PluginName("Lock Variations"sv);
	v.AuthorName("powerofthree"sv);
	v.CompatibleVersions({ SKSE::RUNTIME_1_6_318 });
	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
	
	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(84);

	Settings::GetSingleton()->Load();

	Model::Install();
	Sound::Install();

	return true;
}
