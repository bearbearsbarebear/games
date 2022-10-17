#include <Windows.h>
#include <vector>

#include "detours/detours.h"
#include "memory/Pattern.h"

bool GetSectionHeaderInfo(LPCSTR moduleName, LPCSTR sectionName, PIMAGE_SECTION_HEADER& dest);

#pragma pack(1)
class PacketHeader
{
public:
    int16_t opCode1;
    int8_t empty1;
    int16_t opCode2;
    int8_t unknown1;
    int16_t opCode3;
    int8_t opCode4;
    int16_t strLen;
    char packetTitle[11];
};

struct Mission
{
    int16_t TimeLeft;
    char* PlaceName;
    char* Description;
};

void __cdecl pMyParseWorldBossData(BYTE* packetData, int32_t packetSize)
{
    const PacketHeader* header = reinterpret_cast<PacketHeader*>(packetData);

    if (header->opCode1 == 0x108 && header->opCode2 == 0x102 && header->opCode3 == 2 && header->opCode4 == 7)
    {
        char* str = new char[header->strLen + 1]();
        memcpy_s(str, header->strLen + 1, header->packetTitle, header->strLen);

        if (_stricmp(str, "worldbosses") != 0) { delete[] str; return; }

        delete[] str;

        const int8_t totalBossCount = static_cast<int8_t>(packetData[22]);
        int32_t parsedBossCount = 0;
        int32_t dataPosition = 0;

        std::vector<Mission> missionList;

        while (parsedBossCount < totalBossCount)
        {
            Mission mission = Mission();

            // Search for "time"
            while (*reinterpret_cast<int32_t*>(packetData + dataPosition) != 0x656D6974)
            {
                dataPosition++;

                if (dataPosition >= packetSize) break;
            }

            if (dataPosition >= packetSize) break;

            mission.TimeLeft = *reinterpret_cast<int16_t*>(packetData + dataPosition + 9);
            // TODO: TimeLeft's type changes depending on the remaining time
            // need to read the previous bytes and interpret what it will be

            // Search for "plac"
            while (*reinterpret_cast<int32_t*>(packetData + dataPosition) != 0x63616C70)
            {
                dataPosition++;

                if (dataPosition >= packetSize) break;
            }

            if (dataPosition >= packetSize) break;

            const int16_t placeNameLen = *reinterpret_cast<int16_t*>(packetData + dataPosition + 10);

            mission.PlaceName = new char[placeNameLen + 1]();
            memcpy_s(mission.PlaceName, placeNameLen + 1, reinterpret_cast<char*>(packetData + dataPosition + 12), placeNameLen);

            // Search for "desc"
            while (*reinterpret_cast<int32_t*>(packetData + dataPosition) != 0x63736564)
            {
                dataPosition++;

                if (dataPosition >= packetSize) break;
            }

            if (dataPosition >= packetSize) break;

            const uint16_t descLen = *reinterpret_cast<uint16_t*>(packetData + dataPosition + 12);

            mission.Description = new char[descLen + 1]();
            memcpy_s(mission.Description, descLen + 1, reinterpret_cast<char*>(packetData + dataPosition + 14), descLen);

            missionList.push_back(mission);

            parsedBossCount++;

            dataPosition += 14 + descLen;

            if (dataPosition >= packetSize) break;
        }

        FILE* stream;
        const errno_t err = fopen_s(&stream, "C:\\Log.txt", "w+");
        if (err == 0)
        {
            for (Mission& m : missionList)
            {
                fprintf_s(stream, "%s\n", m.PlaceName);
                fprintf_s(stream, "%d\n", m.TimeLeft);
                fprintf_s(stream, "%s\n\n", m.Description);

                delete[] m.PlaceName;
                delete[] m.Description;
            }
            fclose(stream);
        }
    }
}
void(__cdecl* Hook_ParseWorldBossData)();
void  __declspec(naked) __cdecl MyParseWorldBossData()
{
    __asm
    {
        pushad
        push dword ptr [ebp - 0x34]
        push dword ptr [ebp - 0x38]
        call pMyParseWorldBossData
        add esp, 8
        popad
        jmp Hook_ParseWorldBossData
    }
} 

DWORD WINAPI MainThread(LPVOID param)
{
    PIMAGE_SECTION_HEADER pCodeSection;

    if (!GetSectionHeaderInfo(nullptr, ".text", pCodeSection)) return 0;

    const DWORD_PTR pCodeMemory = reinterpret_cast<DWORD_PTR>(GetModuleHandle(nullptr)) + pCodeSection->VirtualAddress;
    Pattern codeData = Pattern(reinterpret_cast<LPSTR>(pCodeMemory), pCodeMemory, pCodeSection->SizeOfRawData);

    // Pattern for the hooked function in .text
    const DWORD_PTR pParseWorldBossData = codeData.Find("\x8D\x5D\xD0\x8B\x45\xB0\x83\xEC\x04\x89\x9D\x6C\xFF\xFF\xFF\x8B\x55\xCC\x39\xD9\x0F\x84\xA7\x01\x00\x00\x8D\x75\xB8\x8B\x5D\xD0\x39\xF0\x0F\x84", "xx?xx?xx?xx????xx?xxxx????xx?xx?xxxx", Pattern::Default, 1);

    if (pParseWorldBossData == NULL) return -1;

    Hook_ParseWorldBossData = reinterpret_cast<void(__cdecl*)()>(pParseWorldBossData);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&reinterpret_cast<PVOID&>(Hook_ParseWorldBossData), reinterpret_cast<void*>(MyParseWorldBossData));

    if (DetourTransactionCommit() != ERROR_SUCCESS) return 0;

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        MessageBox(nullptr, "DLL Injected successfully", "Success", MB_ICONINFORMATION);
        CreateThread(0, 0, MainThread, hModule, 0, 0);
    }

    return TRUE;
}
