# QuickLootRE

## Build Dependencies
* [SKSE64](https://skse.silverlock.org/)
* [Json2Settings](https://github.com/Ryan-rsm-McKenzie/Json2Settings)
* [HookShareSSE](https://github.com/Ryan-rsm-McKenzie/HookShareSSE)
* [CommonLibSSE](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE)

## End User Dependencies
* [SKSE64](https://skse.silverlock.org/)
* [HookShareSSE](https://github.com/Ryan-rsm-McKenzie/HookShareSSE)
* [ScaleformTranslation++](https://github.com/Ryan-rsm-McKenzie/ScaleformTranslationPP)

## Settings
Setting | Description
--- | ---
`disableInCombat` | Disables the loot menu from displaying in combat.
`disableTheft` | Disables the loot menu from displaying when thieving from containers.
`disablePickpocketing` | Disables the loot menu from displaying when pickpocketing.
`disableIfEmpty` | Disables the loot menu from displaying when empty.
`disableSingleLoot` | Disables single loot mode.
`disableForAnimals` | Disables the loot menu from displaying when looting animals.
`disableActiTextHook` | Disables the loot menu from hooking activation text, and instead uses localized search strings. Set to `True` when search text appears garbled, such as when using iActivate.
`disableAnimations` | Disables playing animations when opening/closing containers.
`disableInvisDispell` | Disables invisibility from being dispelled when looting.
`disableForActiOverride` | Prevents the menu from opening when the activation options are overridden from default. Set to `True` for compatibility with mods like Hunterborn.
`disableOnActiDispatch` | Prevents the mod from dispatching `OnActivate` events to scripts attached to the targeted object reference. Set to `True` if the vanilla loot menu constantly pops up when looking at an object.
`itemLimit` | The maximum number of items to display in the loot menu at a given time.
`scale` | The scale of the loot menu GUI. A negative value will use the default of `25`.
`positionX` | The X position of the loot menu GUI. A negative value will use the default of `384`.
`positionY` | The Y position of the loot menu GUI. A negative value will use the default of `0`.
`opacity` | The opacity of the loot menu GUI. A negative value will use the default of `100`.
`singleLootModifier` | Determines the key used for the single loot feature. Valid inputs are: `"activate"`, `"readyWeapon"`, `"togglePOV"`, `"jump"`, `"sprint"`, `"sneak"`, `"shout"`, `"toggleRun"`, and `"autoMove"`.
`takeMethod` | Determines the key used for the take feature. Valid inputs are: `"activate"`, `"readyWeapon"`, `"togglePOV"`, `"jump"`, `"sprint"`, `"sneak"`, `"shout"`, `"toggleRun"`, and `"autoMove"`.
`takeAllMethod` | Determines the key used for the take all feature. Valid inputs are: `"activate"`, `"readyWeapon"`, `"togglePOV"`, `"jump"`, `"sprint"`, `"sneak"`, `"shout"`, `"toggleRun"`, and `"autoMove"`.
`searchMethod` | Determines the key used for the search feature. Valid inputs are: `"activate"`, `"readyWeapon"`, `"togglePOV"`, `"jump"`, `"sprint"`, `"sneak"`, `"shout"`, `"toggleRun"`, and `"autoMove"`.
`interfaceStyle` | Determines the interface style to use. Valid inputs are: `"default"`, and `"dialogue"`.
`sortOrder` | Defines the order items are sorted in the loot menu. Valid inputs are: `"name"`, `"count"`, `"value"`, `"weight"`, `"type"`, `"read"`, `"enchanted"`, `"pickPocketChance"`, `"valuePerWeight"`, and `"priority"`. Any sorting method can be prefixed with `"r_"` to reverse the sort order, e.g. `"r_name"` will sort in reverse alphabetical order. `sortOrder` is an array, so multiple inputs are acceptable.

## Console commands
<img width=120/> | SetQuickLootVariable
--- | ---
**Command** | `<SetQuickLootVariable\|sqlv> <variable-name> <new-value>`
**Example Usage** | `sqlv itemLimit 25`
**Description** | Sets the specified variable to the specified value. String and array variables can not be set from the console.

## Tips
* Hold the journal menu key (ESC) for 2 seconds toggle the loot menu on/off
