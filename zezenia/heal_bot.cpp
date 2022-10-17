#include <Windows.h>

#include "detours/detours.h"
#include "memory/Pattern.h"

// The values below will be updated every time the client calls it's update
// routine (where the hook is at)
int32_t _PlayerHealth;
int32_t _PlayerMana;

// Health inline detour
void(__cdecl* HookedHealthData)();
void  __declspec(naked) __cdecl MyHealthData()
{
    __asm
    {
        mov dword ptr ds : [_PlayerHealth] , eax
        mov dword ptr ds : [esp + 0x1C] , eax
        jmp HookedHealthData
    }
}

// Mana inline detour
void(__cdecl* HookedManaData)();
void  __declspec(naked) __cdecl MyManaData()
{
    __asm
    {
        mov dword ptr ds : [_PlayerMana] , eax
        mov dword ptr ds : [esp + 0x1C] , eax
        jmp HookedManaData
    }
}

DWORD WINAPI HealthHookThread(LPVOID param)
{
    PIMAGE_SECTION_HEADER pCodeSection;

    if (!GetSectionHeaderInfo(nullptr, ".text", pCodeSection)) return 0;

    const DWORD_PTR pCodeMemory = reinterpret_cast<DWORD_PTR>(GetModuleHandle(nullptr)) + pCodeSection->VirtualAddress;
    Pattern codeData = Pattern(reinterpret_cast<LPSTR>(pCodeMemory), pCodeMemory, pCodeSection->SizeOfRawData);

    // Pattern for the hooked health in .text
    const DWORD_PTR pHealthData = codeData.Find("\x83\xC4\x08\xC7\x45\xEC\x0F\x00\x00\x00\x99", "xx?xx?xxxxx", Pattern::Default);
    if (pHealthData == NULL) return -1;

    HookedHealthData = reinterpret_cast<void(__cdecl*)()>(pHealthData);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&reinterpret_cast<PVOID&>(HookedHealthData), reinterpret_cast<void*>(MyHealthData));

    if (DetourTransactionCommit() != ERROR_SUCCESS) return 0;

    return 0;
}

DWORD WINAPI ManaHookThread(LPVOID param)
{
    PIMAGE_SECTION_HEADER pCodeSection;

    if (!GetSectionHeaderInfo(nullptr, ".text", pCodeSection)) return 0;

    const DWORD_PTR pCodeMemory = reinterpret_cast<DWORD_PTR>(GetModuleHandle(nullptr)) + pCodeSection->VirtualAddress;
    Pattern codeData = Pattern(reinterpret_cast<LPSTR>(pCodeMemory), pCodeMemory, pCodeSection->SizeOfRawData);

    // Pattern for the hooked mana in .text
    const DWORD_PTR pManaData = codeData.Find("\x83\xC4\x08\x6A\x00\xD9\x5D\xC8\x66\x0F\x6E\xC8\xF3\x0F\x2C\x45", "xx?xxxx?xxxxxxxx", Pattern::Default);
    if (pManaData == NULL) return -1;

    HookedManaData = reinterpret_cast<void(__cdecl*)()>(pManaData);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&reinterpret_cast<PVOID&>(HookedManaData), reinterpret_cast<void*>(MyManaData));

    if (DetourTransactionCommit() != ERROR_SUCCESS) return 0;

    return 0;
}

DWORD WINAPI MainThread(LPVOID param)
{
    while (true) {
        if (_PlayerHealth < 300) {
            MessageBox(nullptr, "Health below 300!", "Warning", MB_ICONINFORMATION);
        }

        if (_PlayerMana < 10) {
            MessageBox(nullptr, "Mana below 10!", "Warning", MB_ICONINFORMATION);
        }
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Ideally, everything in a single thread, but this is just for
        // proof of concept
        CreateThread(0, 0, HealthHookThread, hModule, 0, 0);
        CreateThread(0, 0, ManaHookThread, hModule, 0, 0);
        CreateThread(0, 0, MainThread, hModule, 0, 0);
    }

    return true;
}
