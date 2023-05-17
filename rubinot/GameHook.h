#pragma once

#include <detours/detours.h>
#include <memory/Pattern.h>
#include <string>
#include <chrono>

class GameHook
{
public:
    static bool isManaHealNeeded;
    static bool isHighHealthHealNeeded;
    static bool isLowHealthHealNeeded;
    static bool isParalyzed;

    static std::chrono::steady_clock::time_point last_item_use;
    static std::chrono::steady_clock::time_point last_healing_spell;
    static std::chrono::steady_clock::time_point last_support_spell;

    static bool Initialize();

    static void(__cdecl* HookedManaData)();
    static void(__cdecl* HookedHealthData)();
    static void(__cdecl* HookedStatusData)();

    static void HandlePlayerMana(int32_t mana);
    static void __cdecl MyManaData();
    static void HandlePlayerHealth(int32_t health);
    static void __cdecl MyHealthData();
    static void HandlePlayerStatus(int32_t status);
    static void __cdecl MyStatusData();

    // These functions are related to the hooks but
    // They are done externally
    // This is due to my extreme laziness to reverse the 
    // Button press and cooldown functions from the client
    static void pressButton(unsigned char key);
    static boolean hasCooldownExpired(long long cooldown);
    static bool isGameOpen();
};
