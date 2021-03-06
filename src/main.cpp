#include "Settings.h"

namespace Model
{
	namespace Lock
	{
		struct RequestModel
		{
			static std::uint8_t thunk(const char* a_modelPath, std::uintptr_t a_unk02, std::uintptr_t a_unk03)
			{
				const std::string modelPath = Settings::GetSingleton()->GetLockModel(a_modelPath);

				return func(modelPath.c_str(), a_unk02, a_unk03);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	namespace Lockpick
	{
		struct RequestModel
		{
			static std::uint8_t thunk(const char* a_modelPath, std::uintptr_t a_unk02, std::uintptr_t a_unk03)
			{
				const std::string modelPath = Settings::GetSingleton()->GetLockpickModel(a_modelPath);

				return func(modelPath.c_str(), a_unk02, a_unk03);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(51081) };

		stl::write_thunk_call<Lock::RequestModel>(target.address() + 0xC6);
		stl::write_thunk_call<Lockpick::RequestModel>(target.address() + 0xA1);
	}
}

namespace Sound
{
	struct CylinderSqueak
	{
		static void thunk(const char* a_editorID)
		{
			std::string editorID = a_editorID;

			if (const auto soundData = Settings::GetSingleton()->GetSoundData(); soundData) {
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
			const auto soundData = Settings::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ? soundData->UILockpickingCylinderStop : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct CylinderTurn
	{
		static void thunk(const char* a_editorID)
		{
			const auto soundData = Settings::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ? soundData->UILockpickingCylinderTurn : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PickMovement
	{
		static void thunk(const char* a_editorID)
		{
			const auto soundData = Settings::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ? soundData->UILockpickingPickMovement : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct LockpickingUnlock
	{
		static void thunk(const char* a_editorID)
		{
			const auto soundData = Settings::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ? soundData->UILockpickingUnlock : a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> update_lock_angle{ REL::ID(51096) };
		stl::write_thunk_call<CylinderSqueak>(update_lock_angle.address() + 0x159);
		stl::write_thunk_call<CylinderStop>(update_lock_angle.address() + 0xD9);
		stl::write_thunk_call<LockpickingUnlock>(update_lock_angle.address() + 0xA1);

		REL::Relocation<std::uintptr_t> rotate_lock{ REL::ID(51098) };
		stl::write_thunk_call<CylinderTurn>(rotate_lock.address() + 0x33);

		REL::Relocation<std::uintptr_t> update_pick_angle{ REL::ID(51094) };
		stl::write_thunk_call<PickMovement>(update_pick_angle.address() + 0x13F);
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
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

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Lock Variations";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(98);

	Settings::GetSingleton()->Load();

	Model::Install();
	Sound::Install();

	return true;
}
