#include "Stdafx.h"
#include <winuser.h>
#include <detours/detours.h>
#include <memory/Pattern.h>
#include <chrono>
#include <string>

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

DWORD WINAPI initThread(LPVOID param)
{
    // Mana shit
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

    auto last_mana_pot = std::chrono::steady_clock::now();
    auto last_healing_spell = std::chrono::steady_clock::now();

    while (true) 
    {
        char window_title[256];
        HWND foreground_window = GetForegroundWindow();
        if (foreground_window)
        {
            GetWindowText(foreground_window, window_title, sizeof(window_title));
            std::string title(window_title);
            if (title.find("Tibia") != std::string::npos)
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed_mana_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_mana_pot).count();
                auto elapsed_healing_spell_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_healing_spell).count();

                if (elapsed_mana_time >= 1050)
                {
                    if (PlayerData.CurrentMana < 1100)
                    {
                        pressButton(0x35); // 5
                        last_mana_pot = now;
                        //HookDebug::Message("%d", _PlayerMana);
                        //MessageBox(nullptr, TEXT("%d", _PlayerMana), TEXT("(Warning)"), MB_ICONINFORMATION);
                    }
                }

                if (elapsed_healing_spell_time >= 1050)
                {
                    if (PlayerData.CurrentHealth <= 800)
                    {
                        pressButton(0x31); // 1
                        last_healing_spell = now;
                    }
                }
            }
        }

        Sleep(50);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            CreateThread(0, 0, initThread, hModule, 0, 0);
    }

    return true;
}
