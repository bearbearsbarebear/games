#include "Stdafx.h"
#include "GameHook.h"
#include "ConfigManager.h"

DWORD WINAPI initThread(LPVOID param)
{
    if (!GameHook::Initialize()) {
        MessageBox(nullptr, "Something went wrong with the game hook", "(Warning)", MB_ICONINFORMATION);
        return 1;
    }

    while (true) {
        if (GameHook::isGameOpen()) {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            long long elapsed_item_use_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - GameHook::last_item_use).count();
            long long elapsed_healing_spell_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - GameHook::last_healing_spell).count();
            long long elapsed_support_spell_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - GameHook::last_support_spell).count();

            // Health Routine
            if (GameHook::isLowHealthHealNeeded) {
                if (GameHook::hasCooldownExpired(elapsed_healing_spell_time)) {
                    GameHook::pressButton(ConfigManager::stringToKeyCode(ConfigManager::lowHealthSpellKey));
                    GameHook::last_healing_spell = now;
                }

                if (GameHook::hasCooldownExpired(elapsed_item_use_time)) {
                    GameHook::pressButton(ConfigManager::stringToKeyCode(ConfigManager::lowHealthItemKey));
                    GameHook::last_item_use = now;
                }
            } else if (GameHook::isHighHealthHealNeeded) {
                if (GameHook::hasCooldownExpired(elapsed_healing_spell_time)) {
                    GameHook::pressButton(ConfigManager::stringToKeyCode(ConfigManager::highHealthkey));
                    GameHook::last_healing_spell = now;
                }
            }

            // Mana Routine
            if (GameHook::isManaHealNeeded) {
                if (GameHook::hasCooldownExpired(elapsed_item_use_time)) {
                    if (ConfigManager::manaKey.compare("false") != 0) {
                        GameHook::pressButton(ConfigManager::stringToKeyCode(ConfigManager::manaKey));
                        GameHook::last_item_use = now;
                    }
                }
            }

            // Paralyze Routine
            if (GameHook::isParalyzed && ConfigManager::healParalyze) {
                if (GameHook::hasCooldownExpired(elapsed_support_spell_time)) {
                    GameHook::pressButton(ConfigManager::stringToKeyCode(ConfigManager::healParalyzeKey));
                    GameHook::last_support_spell = now;
                    GameHook::isParalyzed = false;
                }
            }
        }

        Sleep(25);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, initThread, hModule, 0, 0);
    }

    return true;
}
