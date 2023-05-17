#include "GameHook.h"
#include "ConfigManager.h"

bool GameHook::isManaHealNeeded = false;
bool GameHook::isHighHealthHealNeeded = false;
bool GameHook::isLowHealthHealNeeded = false;
bool GameHook::isParalyzed = false;

std::chrono::steady_clock::time_point GameHook::last_item_use = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point GameHook::last_healing_spell = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point GameHook::last_support_spell = std::chrono::steady_clock::now();

void(__cdecl* GameHook::HookedManaData)();
void(__cdecl* GameHook::HookedHealthData)();
void(__cdecl* GameHook::HookedStatusData)();

// Responsible for the detours
// Returns false if something was wrong
bool GameHook::Initialize()
{
    switch (ConfigManager::loadConfig()) {
    case 1:
        MessageBox(nullptr, "Error finding configuration file", "(Warning)", MB_ICONINFORMATION);
        return false;
    case 2:
        MessageBox(nullptr, "Error opening or reading configuration file", "(Warning)", MB_ICONINFORMATION);
        return false;
    case 3:
        MessageBox(nullptr, "Error reading configuration file", "(Warning)", MB_ICONINFORMATION);
        return false;
    default:
        MessageBox(nullptr, "Successfully loaded config file!", "(Success)", MB_ICONINFORMATION);
    }

    PIMAGE_SECTION_HEADER pCodeSection;

    if (!GetSectionHeaderInfo(nullptr, ".text", pCodeSection)) return false;

    const DWORD_PTR pCodeMemory = reinterpret_cast<DWORD_PTR>(GetModuleHandle(nullptr)) + pCodeSection->VirtualAddress;
    Pattern codeData = Pattern(reinterpret_cast<LPSTR>(pCodeMemory), pCodeMemory, pCodeSection->SizeOfRawData);

    // Pattern for the hooked values in .text
    const DWORD_PTR pManaData = codeData.Find("\x39\x46\x44\x74\x12\x6A\x00\x6A\x02\x68\x34\x52\xD9\x00\x56\x89\x46\x44\xFF\xD7\x83\xC4\x10\x8B\x45\xEC\x39\x46\x48\x74\x12\x6A\x00\x6A\x03\x68\x34\x52\xD9\x00\x56\x89\x46", "xx?x?xxxxx????xxx?xxxx?xx?xx?x?xxxxx????xxx", Pattern::Default);
    const DWORD_PTR pHealthData = codeData.Find("\x8B\x3D\x60\x28\xCE\x00\x39\x46\x3C\x74\x12\x6A\x00\x6A\x00\x68\x34\x52\xD9\x00\x56\x89\x46\x3C\xFF\xD7\x83\xC4\x10\x8B\x45\xEC\x39\x46\x40\x74", "xx????xx?x?xxxxx????xxx?xxxx?xx?xx?x", Pattern::Default);
    const DWORD_PTR pStatusData = codeData.Find("\x8B\x02\x83\xF8\x1D\x0F\x87\xAC\x0D\x00\x00\xFF\x24\x85\x44\x68\xC1\x00\x6A\xFF\x68\xE8\xE1\x65\x01\x53\xFF\x15\x64\x27\xEB\x00\x83\xC4\x0C\x8B\xC3\x8B\x4D\xF4\x64\x89\x0D\x00\x00\x00\x00\x59\x5F\x5E\x5B\x8B\xE5\x5D\xC3", "xxxxxxx????xxx????xxx????xxx????xx?xxxx?xxxxxxxxxxxxxxx", Pattern::Default);

    if (pManaData == NULL) return false;
    if (pHealthData == NULL) return false;
    if (pStatusData == NULL) return false;

    GameHook::HookedManaData = reinterpret_cast<void(__cdecl*)()>(pManaData);
    GameHook::HookedHealthData = reinterpret_cast<void(__cdecl*)()>(pHealthData);
    GameHook::HookedStatusData = reinterpret_cast<void(__cdecl*)()>(pStatusData);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&reinterpret_cast<PVOID&>(GameHook::HookedManaData), reinterpret_cast<void*>(GameHook::MyManaData));
    DetourAttach(&reinterpret_cast<PVOID&>(GameHook::HookedHealthData), reinterpret_cast<void*>(GameHook::MyHealthData));
    DetourAttach(&reinterpret_cast<PVOID&>(GameHook::HookedStatusData), reinterpret_cast<void*>(GameHook::MyStatusData));

    if (DetourTransactionCommit() != ERROR_SUCCESS) return false;

    return true;
}

bool GameHook::isGameOpen()
{
    char window_title[256];
    HWND foreground_window = GetForegroundWindow();

    if (foreground_window) {
        GetWindowText(foreground_window, window_title, sizeof(window_title));
        std::string title(window_title);

        if (title.find("Tibia") != std::string::npos) return true;
    }

    return false;
}

void GameHook::HandlePlayerMana(int32_t mana)
{
    // Prioritize low health healing instead of mana healing
    // This is subjective and could change depending on each player and circumstances
    // To my use case this is how it goes
    GameHook::isManaHealNeeded = (mana <= ConfigManager::mana && !GameHook::isLowHealthHealNeeded);
}

void __declspec(naked) __cdecl GameHook::MyManaData()
{
    __asm
    {
        pushad
        push eax
        call HandlePlayerMana
        add esp, 4
        popad
        jmp HookedManaData
    }
}

void GameHook::HandlePlayerHealth(int32_t health)
{
    GameHook::isLowHealthHealNeeded = (health <= ConfigManager::lowHealth);
    GameHook::isHighHealthHealNeeded = (health > ConfigManager::lowHealth && health <= ConfigManager::highHealth);
}

void __declspec(naked) __cdecl GameHook::MyHealthData()
{
    __asm
    {
        pushad
        push eax
        call HandlePlayerHealth
        add esp, 4
        popad
        jmp HookedHealthData
    }
}

void GameHook::HandlePlayerStatus(int32_t status)
{
    // I can't really do the code below:
    // GameHook::isParalyzed = (status == 5)
    // because status works as a list of status, so the next moment that is ran it will set to false
    if (status == 5) {
        GameHook::isParalyzed = true;
    }
}

void __declspec(naked) __cdecl GameHook::MyStatusData()
{
    __asm
    {
        pushad
        mov eax, [edx]
        push eax
        call HandlePlayerStatus
        add esp, 4
        popad
        jmp HookedStatusData
    }
}

void GameHook::pressButton(unsigned char key)
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

boolean GameHook::hasCooldownExpired(long long cooldown)
{
    if (cooldown <= 1050) return false;

    return true;
}
