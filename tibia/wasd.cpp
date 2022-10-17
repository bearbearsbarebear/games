#include <Windows.h>
#include <cstdint>
#include "detours/detours.h"

/*
 * The pIsOnlineAddr I got somewhere at https://otland.net/
 *
 * This is just a proof of concept, not exactly ready to use.
 */

// Addresses support 8.6
constexpr DWORD pProcessKeyInputAddr = 0x458200;
constexpr DWORD pIsOnlineAddr = 0x79CF28;

bool wasdActive = false;

bool isOnline()
{
    return (*reinterpret_cast<DWORD*>(pIsOnlineAddr)) == 8;
}

/* Mid-code Hook for funsies
int16_t __cdecl pMyGameStarted(int16_t key)
{
    // Do Whatever
}
void(__cdecl* GameProcess::Hook_GameStarted)();
void  __declspec(naked) __cdecl :MyGameStarted()
{
    __asm
    {
        pushad
        push eax
        call GameProcess::pMyGameStarted
        add esp, 4
        mov dword ptr ds : [esp + 0x1C], eax
        popad
        jmp GameProcess::Hook_GameStarted
    }
}
*/

// key pushed to stack is a WORD(2 bytes)
void(__cdecl* Hook_ProcessKeyInput)(int16_t key);
void __cdecl MyProcessKeyInput(int16_t key)
{
    if (isOnline()) {
        if (wasdActive) {
            switch (tolower(key))
            {
                case 'w':
                    key = 268;
                    break;
                case 'a':
                    key = 269;
                    break;
                case 's':
                    key = 271;
                    break;
                case 'd':
                    key = 270;
                    break;
                default:
                    break;
            }
        }

	// 272 = insert
        if (key == 272) wasdActive = !wasdActive;
    }

    Hook_ProcessKeyInput(key);
}

DWORD WINAPI MainThread(LPVOID param)
{
    constexpr DWORD_PTR pProcessKeyInput = pProcessKeyInputAddr;

    Hook_ProcessKeyInput = reinterpret_cast<void(__cdecl*)(int16_t)>(pProcessKeyInput);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&reinterpret_cast<PVOID&>(Hook_ProcessKeyInput), reinterpret_cast<void*>(MyProcessKeyInput));

    if (DetourTransactionCommit() != ERROR_SUCCESS) return 0;

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) 
    {
        case DLL_PROCESS_ATTACH: 
        {
            CreateThread(0, 0, MainThread, hModule, 0, 0);
            break;
        }
    	default:
        {
	        break;
        }
    }

    return TRUE;
}
