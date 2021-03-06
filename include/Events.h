#pragma once

#include "RE/Skyrim.h"
#include "SKSE/Events.h"


namespace Events
{
	class CrosshairRefEventHandler : public RE::BSTEventSink<SKSE::CrosshairRefEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;


		virtual	EventResult ProcessEvent(const SKSE::CrosshairRefEvent* a_event, RE::BSTEventSource<SKSE::CrosshairRefEvent>* a_dispatcher) override;

		static CrosshairRefEventHandler* GetSingleton();

	protected:
		CrosshairRefEventHandler() = default;
		CrosshairRefEventHandler(const CrosshairRefEventHandler&) = delete;
		CrosshairRefEventHandler(CrosshairRefEventHandler&&) = delete;
		virtual ~CrosshairRefEventHandler() = default;

		CrosshairRefEventHandler& operator=(const CrosshairRefEventHandler&) = delete;
		CrosshairRefEventHandler& operator=(CrosshairRefEventHandler&&) = delete;
	};


	class InputEventHandler : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;


		static InputEventHandler* GetSingleton();

		virtual EventResult ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

	protected:
		InputEventHandler() = default;
		InputEventHandler(const InputEventHandler&) = delete;
		InputEventHandler(InputEventHandler&&) = delete;
		virtual ~InputEventHandler() = default;

		InputEventHandler& operator=(const InputEventHandler&) = delete;
		InputEventHandler& operator=(InputEventHandler&&) = delete;
	};


	class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;


		static MenuOpenCloseEventHandler* GetSingleton();

		virtual EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;

	protected:
		MenuOpenCloseEventHandler() = default;
		MenuOpenCloseEventHandler(const MenuOpenCloseEventHandler&) = delete;
		MenuOpenCloseEventHandler(MenuOpenCloseEventHandler&&) = delete;
		virtual ~MenuOpenCloseEventHandler() = default;

		MenuOpenCloseEventHandler& operator=(const MenuOpenCloseEventHandler&) = delete;
		MenuOpenCloseEventHandler& operator=(MenuOpenCloseEventHandler&&) = delete;
	};


	class TESCombatEventHandler : public RE::BSTEventSink<RE::TESCombatEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;


		static TESCombatEventHandler* GetSingleton();

		virtual EventResult ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>* a_eventSource) override;

	protected:
		TESCombatEventHandler() = default;
		TESCombatEventHandler(const TESCombatEventHandler&) = delete;
		TESCombatEventHandler(TESCombatEventHandler&&) = delete;
		virtual ~TESCombatEventHandler() = default;

		TESCombatEventHandler& operator=(const TESCombatEventHandler&) = delete;
		TESCombatEventHandler& operator=(TESCombatEventHandler&&) = delete;
	};


	class TESContainerChangedEventHandler : public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;


		static TESContainerChangedEventHandler* GetSingleton();

		virtual EventResult ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>* a_eventSource) override;

	protected:
		TESContainerChangedEventHandler() = default;
		TESContainerChangedEventHandler(const TESContainerChangedEventHandler&) = delete;
		TESContainerChangedEventHandler(TESContainerChangedEventHandler&&) = delete;
		virtual ~TESContainerChangedEventHandler() = default;

		TESContainerChangedEventHandler& operator=(const TESContainerChangedEventHandler&) = delete;
		TESContainerChangedEventHandler& operator=(TESContainerChangedEventHandler&&) = delete;
	};
}
