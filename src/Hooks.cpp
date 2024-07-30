#include "Hooks.h"
#include "Manager.h"

namespace Model
{
	namespace Lock
	{
		struct Demand
		{
			static RE::BSResource::ErrorCode thunk(const char* a_modelPath, std::uintptr_t a_modelHandle, const RE::BSModelDB::DBTraits::ArgsType& a_traits)
			{
				auto path = Manager::GetSingleton()->GetLockModel(a_modelPath);

				if (path != a_modelPath) {
					if (const auto ref = RE::LockpickingMenu::GetTargetReference()) {
						logger::info("{}", ref->GetName());
						logger::info("\tLock : {} -> {}", a_modelPath, path);
					}
				}

				return func(path.c_str(), a_modelHandle, a_traits);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	namespace Lockpick
	{
		struct Demand
		{
			static RE::BSResource::ErrorCode thunk(const char* a_modelPath, std::uintptr_t a_modelHandle, const RE::BSModelDB::DBTraits::ArgsType& a_traits)
			{
				const auto path = Manager::GetSingleton()->GetLockpickModel(a_modelPath);

				if (path != a_modelPath) {
					logger::info("\tLockpick : {} -> {}", a_modelPath, path);
				}

				return func(path.c_str(), a_modelHandle, a_traits);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(51081, 51960) };

		stl::write_thunk_call<Lock::Demand>(target.address() + OFFSET_3(0xC6, 0xBC, 0xBB));
		stl::write_thunk_call<Lockpick::Demand>(target.address() + OFFSET_3(0xA1, 0x97, 0x96));
	}
}

namespace Sound
{
	struct CylinderSqueak
	{
		static void thunk(const char* a_editorID)
		{
			std::string editorID = a_editorID;

			if (const auto soundData = Manager::GetSingleton()->GetSoundData()) {
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
			const auto        soundData = Manager::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ?
			                                 soundData->UILockpickingCylinderStop :
			                                 a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct CylinderTurn
	{
		static void thunk(const char* a_editorID)
		{
			const auto        soundData = Manager::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ?
			                                 soundData->UILockpickingCylinderTurn :
			                                 a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PickMovement
	{
		static void thunk(const char* a_editorID)
		{
			const auto        soundData = Manager::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ?
			                                 soundData->UILockpickingPickMovement :
			                                 a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct LockpickingUnlock
	{
		static void thunk(const char* a_editorID)
		{
			const auto        soundData = Manager::GetSingleton()->GetSoundData();
			const std::string editorID = soundData ?
			                                 soundData->UILockpickingUnlock :
			                                 a_editorID;

			return func(editorID.c_str());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> update_lock_angle{ RELOCATION_ID(51096, 51978) };

		stl::write_thunk_call<CylinderSqueak>(update_lock_angle.address() + OFFSET_3(0x159, 0x157, 0x19E));
		stl::write_thunk_call<CylinderStop>(update_lock_angle.address() + OFFSET_3(0xD9, 0xD7, 0x11E));
		stl::write_thunk_call<LockpickingUnlock>(update_lock_angle.address() + OFFSET(0xA1, 0x9F));

		REL::Relocation<std::uintptr_t> rotate_lock{ RELOCATION_ID(51098, 51980) };
		stl::write_thunk_call<CylinderTurn>(rotate_lock.address() + OFFSET(0x33, 0xBC));

		REL::Relocation<std::uintptr_t> update_pick_angle{ RELOCATION_ID(51094, 51976) };
		stl::write_thunk_call<PickMovement>(update_pick_angle.address() + OFFSET_3(0x13F, 0x145, 0x15E));
	}
}
