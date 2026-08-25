#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <ranges>

#include "RE/Skyrim.h"
#include "REX/REX.h"
#include "SKSE/SKSE.h"

#include <MergeMapperPluginAPI.h>

#include <ClibUtil/distribution.hpp>
#include <ClibUtil/editorID.hpp>
#include <boost/regex.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#include <ClibUtil/editorID.hpp>

#include <ClibUtil/SimpleINI.hpp>
#undef ERROR

#define DLLEXPORT __declspec(dllexport)

namespace ini = clib_util::ini;
namespace dist = clib_util::distribution;
namespace edid = clib_util::editorID;

using namespace std::literals;

// for visting variants
template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};

using FormIDStr = std::variant<RE::FormID, std::string>;

namespace stl
{
	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = REL::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
}

namespace Runtime
{
	inline constexpr REL::Version SSE_1_7_99(1, 7, 99, 0);

	inline bool IsAtLeast1_7_99() noexcept
	{
		static const bool value = REX::FModule::GetExecutingModule().GetFileVersion() >= SSE_1_7_99;
		return value;
	}
}

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#	define RELOCATION_ID_VERSIONED(SE, AE, AE1799) \
		REL::ID(Runtime::IsAtLeast1_7_99() ? (AE1799) : (AE))
#elif SKYRIMVR
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#	define RELOCATION_ID_VERSIONED(SE, AE, AE1799) REL::ID(SE)
#else
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#	define RELOCATION_ID_VERSIONED(SE, AE, AE1799) REL::ID(SE)
#endif

#include "Version.h"
