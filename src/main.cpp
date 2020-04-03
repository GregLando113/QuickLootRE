﻿#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/skse_version.h"

#include <string>

#include "CrosshairHook.h"
#include "Events.h"
#include "Hooks.h"
#include "ItemData.h"
#include "LootMenu.h"
#include "Registration.h"
#include "Settings.h"
#include "version.h"

#include "HookShare.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace
{
	enum : UInt32
	{
		kSerializationVersion = 1,

		kQuickLoot = 'QKLT',
		kOnOpenAnimStart = 'OOAS',
		kOnCloseAnimStart = 'OCAS'
	};


	std::string DecodeTypeCode(UInt32 a_typeCode)
	{
		constexpr std::size_t SIZE = sizeof(UInt32);

		std::string sig;
		sig.resize(SIZE);
		char* iter = reinterpret_cast<char*>(&a_typeCode);
		for (std::size_t i = 0, j = SIZE - 2; i < SIZE - 1; ++i, --j) {
			sig[j] = iter[i];
		}
		return sig;
	}


	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		if (!OnContainerOpenAnim::GetSingleton()->Save(a_intfc, kOnOpenAnimStart, kSerializationVersion)) {
			_ERROR("Failed to save OnContainerOpenAnim regs!\n");
		}

		if (!OnContainerCloseAnim::GetSingleton()->Save(a_intfc, kOnCloseAnimStart, kSerializationVersion)) {
			_ERROR("Failed to save OnContainerCloseAnim regs!\n");
		}

		_MESSAGE("Finished saving data");
	}


	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		UInt32 type;
		UInt32 version;
		UInt32 length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (version != kSerializationVersion) {
				_ERROR("Loaded data is out of date! Read (%u), expected (%u) for type code (%s)", version, kSerializationVersion, DecodeTypeCode(type).c_str());
				continue;
			}

			switch (type) {
			case kOnOpenAnimStart:
				{
					auto regs = OnContainerOpenAnim::GetSingleton();
					regs->Clear();
					if (!regs->Load(a_intfc)) {
						_ERROR("Failed to load OnContainerOpenAnim regs!\n");
					}
				}
				break;
			case kOnCloseAnimStart:
				{
					auto regs = OnContainerCloseAnim::GetSingleton();
					regs->Clear();
					if (!regs->Load(a_intfc)) {
						_ERROR("Failed to load OnContainerCloseAnim regs!\n");
					}
				}
				break;
			default:
				_ERROR("Unrecognized record type (%s)!", DecodeTypeCode(type).c_str());
				break;
			}
		}

		_MESSAGE("Finished loading data");
	}


	void HooksReady(SKSE::MessagingInterface::Message* a_msg)
	{
		using HookShare::RegisterForCanProcess_t;

		switch (a_msg->type) {
		case HookShare::kType_CanProcess:
			if (a_msg->dataLen == HookShare::kAPIVersionMajor) {
				auto _RegisterForCanProcess = static_cast<RegisterForCanProcess_t*>(a_msg->data);
				Hooks::InstallHooks(_RegisterForCanProcess);
				_MESSAGE("Hooks registered");
			} else {
				_FATALERROR("An incompatible version of Hook Share SSE was loaded! Expected (%i), found (%i)!\n", HookShare::kAPIVersionMajor, a_msg->type);
				LootMenu::QueueMessage(LootMenu::Message::kHookShareIncompatible);
			}
			break;
		}
	}


	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostPostLoad:
			{
				auto messaging = SKSE::GetMessagingInterface();
				if (messaging->RegisterListener("HookShareSSE", HooksReady)) {
					_MESSAGE("Registered HookShareSSE listener");
				} else {
					_FATALERROR("Failed to register HookShareSSE listener!\n");
					LootMenu::QueueMessage(LootMenu::Message::kHookShareMissing);
				}
			}
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			{
				auto dataHandler = RE::TESDataHandler::GetSingleton();
				if (dataHandler->LookupModByName("SkyUI_SE.esp")) {
					_MESSAGE("SkyUI is installed");
				} else {
					_FATALERROR("SkyUI is not installed!\n");
				}

				auto ui = RE::UI::GetSingleton();
				ui->Register("LootMenu", []() -> RE::IMenu*
				{
					return LootMenu::GetSingleton();
				});
				_MESSAGE("LootMenu registered");

				LootMenu::GetSingleton();	// instantiate menu
				_MESSAGE("LootMenu initialized");

				ItemData::SetCompareOrder();
				_MESSAGE("Settings applied");

				auto crosshairRefDispatcher = SKSE::GetCrosshairRefEventSource();
				crosshairRefDispatcher->AddEventSink(Events::CrosshairRefEventHandler::GetSingleton());
				_MESSAGE("Crosshair ref event handler sinked");

				auto input = RE::BSInputDeviceManager::GetSingleton();
				input->AddEventSink(Events::InputEventHandler::GetSingleton());
				_MESSAGE("Input event handler sinked");

				ui->AddEventSink(Events::MenuOpenCloseEventHandler::GetSingleton());
				_MESSAGE("Menu open/close event handler sinked");

				auto sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
				sourceHolder->AddEventSink(Events::TESCombatEventHandler::GetSingleton());
				_MESSAGE("Combat event handler sinked");

				sourceHolder->AddEventSink(Events::TESContainerChangedEventHandler::GetSingleton());
				_MESSAGE("Container changed event handler sinked");
			}
			break;
		}
	}
}


extern "C" {
	bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		SKSE::Logger::OpenRelative(FOLDERID_Documents, L"\\My Games\\Skyrim Special Edition\\SKSE\\QuickLootRE.log");
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::UseLogStamp(true);

		_MESSAGE("QuickLootRE v%s", QKLT_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "QuickLootRE";
		a_info->version = QKLT_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			_FATALERROR("Loaded in editor, marking as incompatible!\n");
			return false;
		}

		//switch (a_skse->RuntimeVersion()) {
		//case RUNTIME_VERSION_1_5_73:
		//case RUNTIME_VERSION_1_5_80:
		//	break;
		//default:
		//	_FATALERROR("Unsupported runtime version %08X!\n", a_skse->RuntimeVersion());
		//	return false;
		//}

		return true;
	}


	bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		_MESSAGE("QuickLootRE loaded");

		if (!SKSE::Init(a_skse)) {
			return false;
		}

		if (Settings::LoadSettings()) {
			_MESSAGE("Settings successfully loaded");
		} else {
			_FATALERROR("Settings failed to load!\n");
			return false;
		}

		if (!SKSE::AllocTrampoline(1 << 10)) {
			return false;
		}

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", MessageHandler)) {
			return false;
		}

		auto serialization = SKSE::GetSerializationInterface();
		serialization->SetUniqueID(kQuickLoot);
		serialization->SetSaveCallback(SaveCallback);
		serialization->SetLoadCallback(LoadCallback);

		return true;
	}
};
