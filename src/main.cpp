#include "Hooks.h"
#include "Manager.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			if (Manager::GetSingleton()->LoadLocks()) {
				Model::Install();
				Sound::Install();
			}
		}
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			REX::INFO("{:*^30}", "MERGES");
			MergeMapperPluginAPI::GetMergeMapperInterface001();  // Request interface
			if (g_mergeMapperInterface) {                        // Use Interface
				const auto version = g_mergeMapperInterface->GetBuildNumber();
				REX::INFO("\tGot MergeMapper interface buildnumber {}", version);
			} else {
				REX::INFO("INFO - MergeMapper not detected");
			}
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		Manager::GetSingleton()->InitLockForms();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_SUPPORT_AE
constexpr REL::Version MIN_ADDRESS_LIBRARY_V5_RUNTIME{ 1, 7, 99, 0 };

SKSE_PLUGIN_VERSION = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH });
	v.PluginName("Security Overhaul SKSE");
	v.AuthorName("powerofthree/wskeever/Kreiste");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	if constexpr (SKSE::RUNTIME_SSE_LATEST < MIN_ADDRESS_LIBRARY_V5_RUNTIME) {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 2, 5 });
	} else {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 3, 0 });
	}

	return v;
}();
#else
SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Security Overhaul SKSE";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible");
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver
#	ifndef SKYRIMVR
		< SKSE::RUNTIME_SSE_1_5_39
#	else
		> SKSE::RUNTIME_VR_1_4_15_1
#	endif
	) {
		REX::CRITICAL("Unsupported runtime version {}", ver.string());
		return false;
	}

	return true;
}
#endif

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .log = true,
						   .logName = Version::PROJECT.data(),
						   .trampoline = true,
						   .trampolineSize = 7*14 });

	const auto version = a_skse->RuntimeVersion();

	REX::INFO("Game version : {}", version);

#ifdef SKYRIM_SUPPORT_AE
	if constexpr (SKSE::RUNTIME_SSE_LATEST < MIN_ADDRESS_LIBRARY_V5_RUNTIME) {
		if (version >= MIN_ADDRESS_LIBRARY_V5_RUNTIME) {
			REX::FAIL(
				"You are using a newer version of Skyrim than this version of {0} supports.\n"
				"Install the correct version of {0} for your game version.\n"
				"Runtime: {1}\n"
				"Supported: 1.6.1170 (Steam) / 1.6.1179 (GOG)",
				Version::PROJECT, version);
		}
	}
#endif

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
