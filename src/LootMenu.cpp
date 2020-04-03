#include "LootMenu.h"

#include "skse64_common/SafeWrite.h"

#include <array>
#include <cstdlib>  // abort
#include <queue>
#include <string>

#include "ActivatePerkEntryVisitor.h"
#include "Forms.h"
#include "Hooks.h"
#include "ItemData.h"
#include "InventoryList.h"
#include "Registration.h"
#include "Settings.h"
#include "Utility.h"

#include "SKSE/API.h"
#include "RE/Skyrim.h"


namespace
{
	StyleMap::StyleMap()
	{
		std::string key;

		key = "dialogue";
		insert({ key, Style::kDialogue });

		key = "default";
		insert({ key, Style::kDefault });
	}
}


RE::UI_MESSAGE_RESULTS LootMenu::ProcessMessage(RE::UIMessage& a_message)
{

	switch (a_message.type) {
	case RE::UI_MESSAGE_TYPE::kShow:
		OnMenuOpen();
		break;
	case RE::UI_MESSAGE_TYPE::kHide:
		OnMenuClose();
		break;
	}

	return RE::UI_MESSAGE_RESULTS::kPassOn;
}


void LootMenu::PostDisplay()
{
	if (IsOpen()) {
		view->Display();
	}
}


bool LootMenu::CanProcess(RE::InputEvent* a_event)
{
	using DeviceType = RE::INPUT_DEVICE;
	using EventType = RE::INPUT_EVENT_TYPE;
	using Gamepad = RE::BSWin32GamepadDevice::Key;
	using Mouse = RE::BSWin32MouseDevice::Key;

	if (IsOpen() && a_event->eventType == EventType::kButton) {
		auto button = static_cast<RE::ButtonEvent*>(a_event);

		if (button->IsUp()) {
			return false;
		}

		auto& controlID = a_event->QUserEvent();
		auto events = RE::UserEvents::GetSingleton();
		if (controlID == events->sneak) {
			return true;
		}

		switch (a_event->device) {
		case DeviceType::kGamepad:
			{
				auto keyMask = static_cast<Gamepad>(button->idCode);
				return (keyMask == Gamepad::kUp || keyMask == Gamepad::kDown);
			}
			break;
		case DeviceType::kMouse:
			{
				auto keyMask = static_cast<Mouse>(button->idCode);
				return (keyMask == Mouse::kWheelDown || keyMask == Mouse::kWheelUp);
			}
			break;
		case DeviceType::kKeyboard:
			return (controlID == events->zoomIn || controlID == events->zoomOut);
			break;
		}
	}
	return false;
}


bool LootMenu::ProcessButton(RE::ButtonEvent* a_event)
{
	using DeviceType = RE::INPUT_DEVICE;
	using Gamepad = RE::BSWin32GamepadDevice::Key;
	using Mouse = RE::BSWin32MouseDevice::Key;

	auto& controlID = a_event->QUserEvent();
	auto events = RE::UserEvents::GetSingleton();
	ControlMethod inputMethod;

	switch (a_event->device) {
	case DeviceType::kGamepad:
		{
			bool ret;
			if (a_event->heldDownSecs == 0.0) {
				ResetInputTimer();
				ret = false;
			} else {
				if (!MeetsInputThreshold(a_event->heldDownSecs)) {
					return true;
				}
				ret = true;
			}

			inputMethod = ControlMethod::kController;
			switch (static_cast<Gamepad>(a_event->idCode)) {
			case Gamepad::kUp:
				ModSelectedIndex(-1);
				break;
			case Gamepad::kDown:
				ModSelectedIndex(1);
				break;
			}

			if (ret) {
				return true;
			}
		}
		break;
	case DeviceType::kMouse:
		if (!a_event->IsDown()) {
			return true;
		}

		inputMethod = ControlMethod::kPC;
		switch (Mouse(a_event->idCode)) {
		case Mouse::kWheelUp:
			ModSelectedIndex(-1);
			break;
		case Mouse::kWheelDown:
			ModSelectedIndex(1);
			break;
		}
		break;
	case DeviceType::kKeyboard:
		if (!a_event->IsDown()) {
			return true;
		}

		inputMethod = ControlMethod::kPC;
		if (controlID == events->zoomIn) {
			ModSelectedIndex(-1);
		} else if (controlID == events->zoomOut) {
			ModSelectedIndex(1);
		}
		break;
	}

	if (controlID == events->sneak) {
		Close();
		SkipNextInput();
		auto player = RE::PlayerCharacter::GetSingleton();
		if (CanOpen(_containerRef, !player->IsSneaking())) {
			Open();
		}
		return true;
	}

	if (_controlMethod != inputMethod) {
		_controlMethod = inputMethod;
		UpdateButtonIcons(_controlMethod == ControlMethod::kController);
	}

	return true;
}


LootMenu* LootMenu::GetSingleton()
{
	if (!_singleton) {
		_singleton = new LootMenu();
	}
	return _singleton;
}


void LootMenu::Free()
{
	if (_singleton) {
		_singleton->Release();
		_singleton = 0;
	}
}


const char* LootMenu::GetSingleLootMapping()
{
	return _singleLootMapping.c_str();
}


void LootMenu::SetSingleLootMapping(const char* a_singLootMapping)
{
	_singleLootMapping = a_singLootMapping;
}


const char* LootMenu::GetTakeMapping()
{
	return _takeMapping.c_str();
}


void LootMenu::SetTakeMapping(const char* a_takeStr)
{
	_takeMapping = a_takeStr;
}


const char* LootMenu::GetTakeAllMapping()
{
	return _takeAllMapping.c_str();
}


void LootMenu::SetTakeAllMapping(const char* a_takeAllStr)
{
	_takeAllMapping = a_takeAllStr;
}


const char* LootMenu::GetSearchMapping()
{
	return _searchMapping.c_str();
}


void LootMenu::SetSearchMapping(const char* a_searchStr)
{
	_searchMapping = a_searchStr;
}


void LootMenu::QueueMessage(Message a_msg)
{
	auto loot = LootMenu::GetSingleton();

	switch (a_msg) {
	case Message::kNoInputLoaded:
		_messageQueue.push("$QuickLootRE_NoInputLoaded");
		break;
	case Message::kHookShareMissing:
		_messageQueue.push("$QuickLootRE_HookShareMissing");
		break;
	case Message::kHookShareIncompatible:
		_messageQueue.push("$QuickLootRE_HookShareIncompatible");
		break;
	case Message::kMissingDependencies:
		_messageQueue.push("$QuickLootRE_MissingDependencies");
		ProcessMessageQueue();
		break;
	case Message::kLootMenuToggled:
		{
			static const char* enabled = "$QuickLootRE_LootMenuToggled_Enabled";
			static const char* disabled = "$QuickLootRE_LootMenuToggled_Disabled";
			auto state = loot->IsEnabled() ? enabled : disabled;
			_messageQueue.push(state);
			ProcessMessageQueue();
		}
		break;
	default:
		_ERROR("Invalid message (%i)", a_msg);
		break;
	}

	if (loot->IsOpen()) {
		ProcessMessageQueue();
	}
}


void LootMenu::ModSelectedIndex(SInt32 a_indexOffset)
{
	if (IsOpen()) {
		_selectedIndex += a_indexOffset;
		if (_selectedIndex < 0) {
			_selectedIndex = 0;
		} else if (_selectedIndex > _displaySize - 1) {
			_selectedIndex = _displaySize - 1;
		}
		Dispatch<SetSelectedIndexDelegate>(_selectedIndex);
	}
}


void LootMenu::SetDisplaySize(SInt32 a_size)
{
	_displaySize = a_size;
}


bool LootMenu::ShouldSkipNextInput() const
{
	return _skipInputCount;
}


void LootMenu::SkipNextInput()
{
	++_skipInputCount;
}


void LootMenu::NextInputSkipped()
{
	if (_skipInputCount > 0) {
		--_skipInputCount;
	}
}


RE::TESObjectREFR* LootMenu::GetContainerRef() const
{
	return _containerRef;
}


void LootMenu::ClearContainerRef()
{
	PlayAnimationClose();
	_containerRef = 0;
}


bool LootMenu::IsOpen() const
{
	return _isMenuOpen;
}


bool LootMenu::IsVisible() const
{
	return view->GetVisible();
}


bool LootMenu::CanProcessInventoryChanges() const
{
	return _canProcessInvChanges;
}


void LootMenu::SetEnabled(bool a_enable)
{
	_isEnabled = a_enable;
	if (!_isEnabled) {
		Close();
	}
}


void LootMenu::ToggleEnabled()
{
	SetEnabled(!_isEnabled);
}


const char* LootMenu::GetActiText() const
{
	return _actiText.c_str();
}


void LootMenu::SetActiText(const char* a_actiText)
{
	_actiText = a_actiText;
}


void LootMenu::Open() const
{
	if (_isEnabled) {
		auto msgQ = RE::UIMessageQueue::GetSingleton();
		msgQ->AddMessage(Name(), RE::UI_MESSAGE_TYPE::kShow, 0);
	}
}


void LootMenu::Close() const
{
	auto msgQ = RE::UIMessageQueue::GetSingleton();
	msgQ->AddMessage(Name(), RE::UI_MESSAGE_TYPE::kHide, 0);
}


void LootMenu::SetVisible(bool a_visible)
{
	auto mc = RE::MenuControls::GetSingleton();
	view->SetVisible(a_visible);
	if (a_visible && !_isRegistered) {
		mc->RegisterHandler(this);
		_isRegistered = true;
	} else if (!a_visible && _isRegistered) {
		mc->RemoveHandler(this);
		_isRegistered = false;
	}
}


void LootMenu::SetContainerRef(RE::TESObjectREFR* a_ref)
{
	_containerRef = a_ref;
}


RE::TESObjectREFR* LootMenu::CanOpen(RE::TESObjectREFR* a_ref, bool a_isSneaking) const
{
	using EntryPoint = RE::BGSEntryPointPerkEntry::EntryPoint;

	static RE::BSFixedString strAnimationDriven = "bAnimationDriven";

	if (!_isEnabled) {
		return 0;
	}

	if (!a_ref || !a_ref->GetBaseObject()) {
		return 0;
	}

	auto ui = RE::UI::GetSingleton();
	auto intfcStr = RE::InterfaceStrings::GetSingleton();
	if (ui->GameIsPaused() || !ui->IsCursorHiddenWhenTopmost() || ui->GetMenu(intfcStr->dialogueMenu)) {
		return 0;
	}

	auto ctrlMap = RE::ControlMap::GetSingleton();
	if (!ctrlMap->IsMovementControlsEnabled()) {
		return 0;
	}

	auto player = RE::PlayerCharacter::GetSingleton();
	if (player->GetGrabbedRef() || player->GetActorDoingPlayerCommand() || player->IsInKillMove()) {
		return 0;
	}

	bool bAnimationDriven;
	if (player->GetGraphVariableImpl3(strAnimationDriven, bAnimationDriven) && bAnimationDriven) {
		return 0;
	}

	if (*Settings::disableInCombat && player->IsInCombat()) {
		return 0;
	}

	if (*Settings::disableTheft && a_ref->IsOffLimits()) {
		return 0;
	}

	if (a_ref->IsActivationBlocked()) {
		return 0;
	}

	RE::TESObjectREFR* containerRef = 0;
	switch (a_ref->GetBaseObject()->formType) {
	case RE::FormType::Activator:
		{
			RE::ObjectRefHandle refHandle = a_ref->extraList.GetAshPileRefHandle();
			if (refHandle) {
				RE::TESObjectREFRPtr refPtr = refHandle.get();
				if (refPtr) {
					containerRef = refPtr.get();
				}
			}
		}
		break;
	case RE::FormType::Container:
		if (!a_ref->IsLocked()) {
			containerRef = a_ref;
		}
		break;
	case RE::FormType::NPC:
		auto target = static_cast<RE::Actor*>(a_ref);
		if (*Settings::disableForAnimals && target->GetRace()->HasKeyword(ActorTypeAnimal)) {
			return 0;
		} else if (a_ref->IsDead(true) && !target->IsSummoned()) {
			containerRef = a_ref;
		} else if (!*Settings::disablePickPocketing && IsValidPickPocketTarget(a_ref, a_isSneaking)) {
			if (!target->IsInCombat()) {
				containerRef = a_ref;
			}
		}
		break;
	}

	if (!containerRef) {
		return 0;
	}

	auto numItems = containerRef->GetInventoryCount();
	auto droppedList = containerRef->extraList.GetByType<RE::ExtraDroppedItemList>();
	if (*Settings::disableIfEmpty && numItems <= 0 && (!droppedList || droppedList->droppedItemList.empty())) {
		return 0;
	}

	if (*Settings::disableForActiOverride && player->HasPerkEntries(EntryPoint::kActivate)) {
		ActivatePerkEntryVisitor visitor(player, containerRef);
		player->ForEachPerkEntry(EntryPoint::kActivate, visitor);
		if (visitor.GetResult()) {
			return 0;
		}
	}

	return containerRef;
}


void LootMenu::TakeItemStack()
{
	if (!IsOpen() || !_containerRef || _displaySize <= 0) {
		return;
	}

	ItemData itemCopy(_invList[_selectedIndex]);

	auto numItems = itemCopy.GetCount();
	if (numItems > 1 && IsSingleLootEnabled()) {
		numItems = 1;
	}

	if (TakeItem(itemCopy, numItems, true, true)) {
		auto player = RE::PlayerCharacter::GetSingleton();
		auto invChanges = player->GetInventoryChanges();
		auto xLists = itemCopy.GetExtraLists();
		auto xList = (xLists && !xLists->empty()) ? xLists->front() : 0;
		invChanges->SendContainerChangedEvent(xList, _containerRef, itemCopy.GetForm(), itemCopy.GetCount());
	}
	RE::ChestsLooted::SendEvent();
}


void LootMenu::TakeAllItems()
{
	using DObj = RE::DEFAULT_OBJECT;

	if (!IsOpen() || !_containerRef || _displaySize <= 0) {
		return;
	}

	auto player = RE::PlayerCharacter::GetSingleton();
	if (IsValidPickPocketTarget(_containerRef, player->IsSneaking())) {
		return;
	}

	_canProcessInvChanges = true;

	std::array<RE::BGSSoundDescriptorForm*, 5> descriptors = { 0 };
	auto dObjManager = RE::BGSDefaultObjectManager::GetSingleton();
	descriptors[0] = dObjManager->GetObject<RE::BGSSoundDescriptorForm>(DObj::kPickupSoundGeneric);
	descriptors[1] = dObjManager->GetObject<RE::BGSSoundDescriptorForm>(DObj::kPickupSoundWeapon);
	descriptors[2] = dObjManager->GetObject<RE::BGSSoundDescriptorForm>(DObj::kPickupSoundArmor);
	descriptors[3] = dObjManager->GetObject<RE::BGSSoundDescriptorForm>(DObj::kPickupSoundBook);
	descriptors[4] = dObjManager->GetObject<RE::BGSSoundDescriptorForm>(DObj::kPickupSoundIngredient);

	auto soundManager = RE::BSAudioManager::GetSingleton();
	for (auto& descriptor : descriptors) {
		if (descriptor) {
			soundManager->Play(descriptor);
		}
	}

	for (auto& item : _invList) {
		TakeItem(item, item.GetCount(), false, false);
	}

	_invList.clear();
	SkipNextInput();
	_containerRef->InitChildActivates(player);  // Trigger traps
	RE::ChestsLooted::SendEvent();

	_canProcessInvChanges = false;
}


InventoryList& LootMenu::GetInventoryList()
{
	return _invList;
}


void LootMenu::ParseInventory()
{
	_invList.parse(_containerRef);
}


LootMenu::LootMenu() :
	MenuBase(),
	HandlerBase(),
	_invList(),
	_actiText(""),
	_containerRef(0),
	_controlMethod(ControlMethod::kPC),
	_selectedIndex(0),
	_displaySize(0),
	_skipInputCount(0),
	_lastInputTimer(0.0),
	_isContainerOpen(false),
	_isMenuOpen(false),
	_canProcessInvChanges(false),
	_isRegistered(false),
	_isEnabled(true)
{
	using ScaleModeType = RE::GFxMovieView::ScaleModeType;
	using Context = RE::UserEvents::INPUT_CONTEXT_ID;
	using Flag = RE::IMenu::Flag;

	auto loader = RE::BSScaleformManager::GetSingleton();
	if (loader->LoadMovie(this, view, SWF_NAME, ScaleModeType::kShowAll, 0.0)) {
		flags = Flag::kAlwaysOpen | Flag::kAllowSaving;
		context = Context::kInventory;
	}

	if (!view) {
		_FATALERROR("Lootmenu did not have a view due to missing dependencies! Aborting process!\n");
		MessageBoxA(NULL, "Lootmenu did not have a view due to missing dependencies!\r\nAborting process!", NULL, MB_OK);
		std::abort();
	}

	SetVisible(false);
	AddRef();	// Force persistence
	Dispatch<SetupDelegate>();
	Dispatch<SwitchStyleDelegate>(GetStyle());
}


LootMenu::~LootMenu()
{}


void LootMenu::ProcessMessageQueue()
{
	const char* msg = 0;
	while (!_messageQueue.empty()) {
		msg = _messageQueue.front();
		_messageQueue.pop();
		RE::DebugNotification(msg);
	}
}


void LootMenu::OnMenuOpen()
{
	if (!_containerRef) {
		return;
	}

	auto input = RE::BSInputDeviceManager::GetSingleton();
	if (input->IsGamepadEnabled()) {
		_controlMethod = ControlMethod::kController;
		UpdateButtonIcons(true);
	} else {
		_controlMethod = ControlMethod::kPC;
		UpdateButtonIcons(false);
	}

	_selectedIndex = 0;
	_skipInputCount = 0;
	_isMenuOpen = true;
	Dispatch<SetContainerDelegate>(_selectedIndex);

	auto player = RE::PlayerCharacter::GetSingleton();
	if (IsValidPickPocketTarget(_containerRef, player->IsSneaking())) {
		Dispatch<SetVisibleButtonsDelegate>(true, false, true);
	} else {
		Dispatch<SetVisibleButtonsDelegate>(true, true, true);
	}

	Dispatch<OpenContainerDelegate>();
	SetVisible(true);
	ProcessMessageQueue();
}


void LootMenu::OnMenuClose()
{
	if (IsOpen()) {
		SetVisible(false);
		_isMenuOpen = false;
		Dispatch<CloseContainerDelegate>();
		PlayAnimationClose();
	}
}


Style LootMenu::GetStyle() const
{
	static StyleMap styleMap;
	auto it = styleMap.find(*Settings::interfaceStyle);
	if (it != styleMap.end()) {
		return it->second;
	} else {
		_ERROR("Invalid style (%s)!", Settings::interfaceStyle->c_str());
		_ERROR("Using default!\n");
		return Style::kDefault;
	}
}


void LootMenu::UpdateButtonIcons(bool a_controller) const
{
	UInt32 take;
	UInt32 takeAll;
	UInt32 search;
	if (a_controller) {
		GetGamepadButtonID(take, _takeMapping);
		GetGamepadButtonID(takeAll, _takeAllMapping);
		GetGamepadButtonID(search, _searchMapping);
	} else {
		GetPCButtonID(take, _takeMapping);
		GetPCButtonID(takeAll, _takeAllMapping);
		GetPCButtonID(search, _searchMapping);
	}
	Dispatch<UpdateButtonIconsDelegate>(take, takeAll, search);
}


void LootMenu::GetGamepadButtonID(UInt32& a_key, const std::string_view& a_mapping) const
{
	using Key = RE::BSWin32GamepadDevice::Key;

	auto ctrlMap = RE::ControlMap::GetSingleton();
	a_key = ctrlMap->GetMappedKey(a_mapping, RE::INPUT_DEVICE::kGamepad);
	switch (a_key) {
	case Key::kUp:
		a_key = 0;
		break;
	case Key::kDown:
		a_key = 1;
		break;
	case Key::kLeft:
		a_key = 2;
		break;
	case Key::kRight:
		a_key = 3;
		break;
	case Key::kStart:
		a_key = 4;
		break;
	case Key::kBack:
		a_key = 5;
		break;
	case Key::kLeftThumb:
		a_key = 6;
		break;
	case Key::kRightThumb:
		a_key = 7;
		break;
	case Key::kLeftShoulder:
		a_key = 8;
		break;
	case Key::kRightShoulder:
		a_key = 9;
		break;
	case Key::kA:
		a_key = 10;
		break;
	case Key::kB:
		a_key = 11;
		break;
	case Key::kX:
		a_key = 12;
		break;
	case Key::kY:
		a_key = 13;
		break;
	case Key::kLeftTrigger:
		a_key = 14;
		break;
	case Key::kRightTrigger:
		a_key = 15;
		break;
	default:
		a_key = kInvalidButton;
		break;
	}

	if (a_key == kInvalidButton) {
		a_key = kESC;
	} else {
		a_key += kGamepadOffset;
	}
}


void LootMenu::GetPCButtonID(UInt32& a_key, const std::string_view& a_mapping) const
{
	auto ctrlMap = RE::ControlMap::GetSingleton();
	a_key = ctrlMap->GetMappedKey(a_mapping, RE::INPUT_DEVICE::kKeyboard);
	if (a_key == kInvalidButton) {
		a_key = ctrlMap->GetMappedKey(a_mapping, RE::INPUT_DEVICE::kMouse);
		if (a_key == kInvalidButton) {
			a_key = kESC;
		} else {
			a_key += kMouseOffset;
		}
	} else {
		a_key += kKeyboardOffset;
	}
}


bool LootMenu::IsSingleLootEnabled() const
{
	using DeviceType = RE::INPUT_DEVICE;

	if (*Settings::disableSingleLoot) {
		return false;
	}

	auto input = RE::BSInputDeviceManager::GetSingleton();
	auto keyboard = input->GetKeyboard();
	if (keyboard && keyboard->IsEnabled()) {
		auto singleLootKeyboard = GetSingleLootKey(DeviceType::kKeyboard);
		if (singleLootKeyboard != -1 && keyboard->IsPressed(singleLootKeyboard)) {
			return true;
		}
	}

	auto gamepad = input->GetGamepad();
	if (gamepad && gamepad->IsEnabled()) {
		auto singleLootSprint = GetSingleLootKey(DeviceType::kGamepad);
		if (singleLootSprint != kInvalidButton && gamepad->IsPressed(singleLootSprint)) {
			return true;
		}
	}

	return false;
}


void LootMenu::PlayAnimation(std::string_view a_fromName, std::string_view a_toName) const
{
	if (!*Settings::disableAnimations) {
		_containerRef->PlayAnimation(a_fromName, a_toName);
	}
}


void LootMenu::PlayAnimationOpen()
{
	if (_containerRef && !_isContainerOpen) {
		PlayAnimation("Close", "Open");

		if (!*Settings::disableAnimations) {
			OnContainerOpenAnim::GetSingleton()->QueueEvent();
		}

		if (_containerRef->formType != RE::FormType::ActorCharacter) {
			auto player = RE::PlayerCharacter::GetSingleton();
			_containerRef->InitChildActivates(player);  // Triggers traps
		}

		_isContainerOpen = true;
	}
}


void LootMenu::PlayAnimationClose()
{
	if (_containerRef && _isContainerOpen) {
		PlayAnimation("Open", "Close");

		if (!*Settings::disableAnimations) {
			auto dispatcher = OnContainerCloseAnim::GetSingleton();
			dispatcher->QueueEvent();
		}

		_isContainerOpen = false;
	}
}


bool LootMenu::TakeItem(ItemData& a_item, SInt32 a_numItems, bool a_playAnim, bool a_playSound)
{
	using EventType = RE::PlayerCharacter::EventType;
	using RemoveType = RE::ITEM_REMOVE_REASON;
	using Archetype = RE::EffectArchetypes::ArchetypeID;

	bool manualUpdate = false;	// picking up dropped items doesn't disptach a container changed event

	// Locate item's extra list (if any)
	auto extraLists = a_item.GetExtraLists();
	RE::ExtraDataList* xList = 0;
	if (extraLists && !extraLists->empty()) {
		xList = extraLists->front();
	}

	auto player = RE::PlayerCharacter::GetSingleton();

	// Pickup dropped items
	if (xList && xList->HasType(RE::ExtraDataType::kItemDropper)) {
		auto refItem = reinterpret_cast<RE::TESObjectREFR*>((std::uintptr_t)xList - offsetof(RE::TESObjectREFR, extraList));
		player->PickUpObject(refItem, 1, false, a_playSound);
		manualUpdate = true;
	} else {
		auto lootMode = RemoveType::kRemove;
		auto baseObj = _containerRef->GetBaseObject();

		if (baseObj->Is(RE::FormType::NPC)) {
			// Dead body
			if (_containerRef->IsDead(false)) {
				player->PlayPickupEvent(a_item.GetForm(), _containerRef->GetOwner(), _containerRef, EventType::kDeadBody);
				// Pickpocket
			} else {
				if (!TryToPickPocket(a_item, lootMode)) {
					return manualUpdate;
				}
			}
		} else {
			// Container
			player->PlayPickupEvent(a_item.GetForm(), _containerRef->GetOwner(), _containerRef, EventType::kContainer);

			// Stealing
			if (_containerRef->IsOffLimits()) {
				lootMode = RemoveType::kSteal;
			}
		}

		// Remove projectile 3D
		auto bound = static_cast<RE::TESBoundObject*>(a_item.GetForm());
		if (bound) {
			bound->HandleRemoveItemFromContainer(_containerRef);
		}

		if (_containerRef->Is(RE::FormType::ActorCharacter)) {
			DispellWornItemEnchantments();
		} else {
			// Stealing
			if (_containerRef->IsOffLimits()) {
				player->StealAlarm(_containerRef, a_item.GetForm(), a_numItems, a_item.GetValue(), _containerRef->GetOwner(), true);
			}
		}

		if (a_playAnim) {
			PlayAnimationOpen();
		}
		if (a_playSound) {
			player->PlayPickUpSound(a_item.GetForm(), true, false);
		}
		if (!*Settings::disableInvisDispell) {
			player->DispellEffectsWithArchetype(Archetype::kInvisibility, false);
		}
		RE::ObjectRefHandle droppedHandle;
		_containerRef->RemoveItem(a_item.GetForm(), a_numItems, lootMode, xList, player);
	}

	return manualUpdate;
}


bool LootMenu::TryToPickPocket(ItemData& a_item, RE::ITEM_REMOVE_REASON& a_lootMode) const
{
	using EventType = RE::PlayerCharacter::EventType;
	using RemoveType = RE::ITEM_REMOVE_REASON;

	auto target = static_cast<RE::Actor*>(_containerRef);
	auto player = RE::PlayerCharacter::GetSingleton();
	auto pickSuccess = player->AttemptPickpocket(target, a_item.GetEntryData(), a_item.GetCount());
	player->PlayPickupEvent(a_item.GetEntryData()->object, _containerRef->GetActorOwner(), _containerRef, EventType::kThief);
	a_lootMode = RemoveType::kSteal;
	if (!pickSuccess) {
		return false;
	} else {
		RE::ItemsPickpocketed::SendEvent(a_item.GetCount());
		return true;
	}
}


void LootMenu::DispellWornItemEnchantments() const
{
	auto actor = static_cast<RE::Actor*>(_containerRef);
	if (actor->currentProcess) {
		actor->DispelWornItemEnchantments();
		actor->currentProcess->Update3DModel(actor);
	}
}


UInt32 LootMenu::GetSingleLootKey(RE::INPUT_DEVICE a_deviceType) const
{
	auto mm = RE::ControlMap::GetSingleton();
	return mm->GetMappedKey(_singleLootMapping, a_deviceType);
}


bool LootMenu::IsEnabled() const
{
	return _isEnabled;
}


void LootMenu::ResetInputTimer()
{
	_lastInputTimer = 0.5;
}


bool LootMenu::MeetsInputThreshold(float a_timer)
{
	if (a_timer > _lastInputTimer + INPUT_THRESHOLD) {
		_lastInputTimer += INPUT_THRESHOLD;
		return true;
	} else {
		return false;
	}
}


decltype(LootMenu::_singleton)			LootMenu::_singleton = 0;
decltype(LootMenu::_singleLootMapping)	LootMenu::_singleLootMapping = "";
decltype(LootMenu::_takeMapping)		LootMenu::_takeMapping = "";
decltype(LootMenu::_takeAllMapping)		LootMenu::_takeAllMapping = "";
decltype(LootMenu::_searchMapping)		LootMenu::_searchMapping = "";
decltype(LootMenu::_messageQueue)		LootMenu::_messageQueue;
