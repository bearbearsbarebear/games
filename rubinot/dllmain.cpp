#include "Stdafx.h"
#include <winuser.h>
#include <detours/detours.h>
#include <memory/Pattern.h>
#include <chrono>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>

struct PlayerInfo
{
    public:
    int32_t CurrentMana;
    int32_t CurrentHealth;
};

static PlayerInfo PlayerData = PlayerInfo();

// Mana inline detour
void(__cdecl* HookedManaData)();
void  __declspec(naked) __cdecl MyManaData()
{
    __asm
    {
        mov PlayerData.CurrentMana, eax
        jmp HookedManaData
    }
}

// Health inline detour
void(__cdecl* HookedHealthData)();
void  __declspec(naked) __cdecl MyHealthData()
{
    __asm
    {
        mov PlayerData.CurrentHealth, eax
        jmp HookedHealthData
    }
}

void pressButton(unsigned char key)
{
    INPUT inputs[2] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = key;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

boolean hasCooldownExpired(long long cooldown)
{
    if (cooldown <= 1050) return false;

    return true;
}

int stringToKeyCode(std::string keyString)
{
    // Remove the "0x" prefix from the string
    if (keyString.substr(0, 2) == "0x") keyString = keyString.substr(2);

    // Convert the hexadecimal string to an integer
    int keyValue = std::stoi(keyString, nullptr, 16);

    return keyValue;
}

DWORD WINAPI initThread(LPVOID param)
{
    PIMAGE_SECTION_HEADER pCodeSection;

    if (!GetSectionHeaderInfo(nullptr, ".text", pCodeSection)) return 0;

    const DWORD_PTR pCodeMemory = reinterpret_cast<DWORD_PTR>(GetModuleHandle(nullptr)) + pCodeSection->VirtualAddress;
    Pattern codeData = Pattern(reinterpret_cast<LPSTR>(pCodeMemory), pCodeMemory, pCodeSection->SizeOfRawData);

    // Pattern for the hooked values in .text
    const DWORD_PTR pManaData = codeData.Find("\x39\x46\x44\x74\x12\x6A\x00\x6A\x02\x68\x34\x52\xD9\x00\x56\x89\x46\x44\xFF\xD7\x83\xC4\x10\x8B\x45\xEC\x39\x46\x48\x74\x12\x6A\x00\x6A\x03\x68\x34\x52\xD9\x00\x56\x89\x46", "xx?x?xxxxx????xxx?xxxx?xx?xx?x?xxxxx????xxx", Pattern::Default);
    const DWORD_PTR pHealthData = codeData.Find("\x8B\x3D\x60\x28\xCE\x00\x39\x46\x3C\x74\x12\x6A\x00\x6A\x00\x68\x34\x52\xD9\x00\x56\x89\x46\x3C\xFF\xD7\x83\xC4\x10\x8B\x45\xEC\x39\x46\x40\x74", "xx????xx?x?xxxxx????xxx?xxxx?xx?xx?x", Pattern::Default);
   
    if (pManaData == NULL) return -1;
    if (pHealthData == NULL) return -1;

    HookedManaData = reinterpret_cast<void(__cdecl*)()>(pManaData);
    HookedHealthData = reinterpret_cast<void(__cdecl*)()>(pHealthData);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&reinterpret_cast<PVOID&>(HookedManaData), reinterpret_cast<void*>(MyManaData));
    DetourAttach(&reinterpret_cast<PVOID&>(HookedHealthData), reinterpret_cast<void*>(MyHealthData));

    if (DetourTransactionCommit() != ERROR_SUCCESS) return 0;

    // Retrieving config file
    // File must be at client.exe folder and be called config.ini
    boost::property_tree::ptree pt;
    try {
        boost::property_tree::ini_parser::read_ini("config.ini", pt);
    } catch (const boost::property_tree::ini_parser_error& e) {
        MessageBox(nullptr, "Error finding configuration file", "(Warning)", MB_ICONINFORMATION);
        return 1;
    } catch (const std::exception& e) {
        MessageBox(nullptr, "Error opening or reading configuration file", "(Warning)", MB_ICONINFORMATION);
        return 1;
    }

    int highHealth, lowHealth, mana;
    std::string highHealthkey, lowHealthSpellKey, lowHealthItemKey, manaKey, healParalyzeKey;
    bool healParalyze;
    try {
        // Retrieve the values
        highHealth = pt.get<int>("HighHealth");
        lowHealth = pt.get<int>("LowHealth");
        mana = pt.get<int>("Mana");

        healParalyze = pt.get<bool>("HealParalyze");

        highHealthkey = pt.get<std::string>("HighHealthKey");
        lowHealthSpellKey = pt.get<std::string>("LowHealthSpellKey");
        lowHealthItemKey = pt.get<std::string>("LowHealthItemKey");
        manaKey = pt.get<std::string>("ManaKey");
        healParalyzeKey = pt.get<std::string>("HealParalyzeKey");
    } catch (const boost::property_tree::ptree_error& e) {
        MessageBox(nullptr, "Error reading configuration file", "(Warning)", MB_ICONINFORMATION);
        return 1;
    }

    MessageBox(nullptr, "Successfully loaded config file!", "(Success)", MB_ICONINFORMATION);

    auto last_item_use = std::chrono::steady_clock::now();
    auto last_healing_spell = std::chrono::steady_clock::now();
    auto last_support_spell = std::chrono::steady_clock::now();

    while (true) {
        char window_title[256];
        HWND foreground_window = GetForegroundWindow();
        if (foreground_window) {
            GetWindowText(foreground_window, window_title, sizeof(window_title));
            std::string title(window_title);
            if (title.find("Tibia") != std::string::npos) {
                // The time now
                auto now = std::chrono::steady_clock::now();

                // How long since an item has been used
                auto elapsed_item_use_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_item_use).count();

                // How long since a healing spell has been used
                auto elapsed_healing_spell_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_healing_spell).count();

                // Heal HP
                if (PlayerData.CurrentHealth <= lowHealth && lowHealth > 0) {
                    if (hasCooldownExpired(elapsed_healing_spell_time) && lowHealthSpellKey.compare("false") != 0) {
                        pressButton(stringToKeyCode(lowHealthSpellKey)); // Low Health Spell Shortcut
                        last_healing_spell = now;
                    }

                    if (hasCooldownExpired(elapsed_item_use_time) && lowHealthItemKey.compare("false") != 0) {
                        pressButton(stringToKeyCode(lowHealthItemKey)); // Low Health Item Shortcut
                        last_item_use = now;
                    }
                } else if (PlayerData.CurrentHealth <= highHealth && highHealth > 0) {
                    if (hasCooldownExpired(elapsed_healing_spell_time) && highHealthkey.compare("false") != 0) {
                        pressButton(stringToKeyCode(highHealthkey)); // High Health Shortcut
                        last_healing_spell = now;
                    }
                }

                // Heal Mana
                if (PlayerData.CurrentMana <= mana && mana > 0) {
                    if (PlayerData.CurrentHealth > lowHealth) {
                        if (hasCooldownExpired(elapsed_item_use_time) && manaKey.compare("false") != 0) {
                            pressButton(stringToKeyCode(manaKey)); // Mana Potion Shortcut
                            last_item_use = now;
                        }
                    }
                }

                // TODO: Heal Paralyze
            }
        }

        Sleep(50);
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
