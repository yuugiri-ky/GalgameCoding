// dllmain.cpp

#include "util.h"


//=============================================================================
// Hook Code
//=============================================================================


LSTATUS APIENTRY MineRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
    auto subKey = ShiftJisToUcs2(lpSubKey);
    return RegOpenKeyW(hKey, subKey, phkResult);
}


LSTATUS APIENTRY MineRegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
{
    auto subKey = ShiftJisToUcs2(lpSubKey);
    return RegDeleteKeyW(hKey, subKey);
}


LSTATUS APIENTRY MineRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    auto subKey = ShiftJisToUcs2(lpSubKey);
    WCHAR szClass[16] { };
    return RegCreateKeyExW(hKey, subKey, 0, szClass, dwOptions, samDesired, NULL, phkResult, lpdwDisposition);
}


int WINAPI MineMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    auto text = ShiftJisToUcs2(lpText);
    auto caption = ShiftJisToUcs2(lpCaption);
    return MessageBoxW(hWnd, text, caption, uType);
}


//=============================================================================
// Startup Code
//=============================================================================


void InstallPatches()
{
    // .text:004376A9                             ; 14:           lf.lfCharSet = 0x80;
    // .text:004376A9 048 C6 44 24 23 80                          mov     [esp+48h+lf.lfCharSet], 80h
    PatchWrite((PVOID)(0x4376A9 + 4), (BYTE)0x01);

    // .text:004422F5                             ; 45:           stru_59B150.lfCharSet = 0x80;
    // .text:004422F5 050 C6 05 67 B1 59 00 80                    mov     stru_59B150.lfCharSet, 80h
    PatchWrite((PVOID)(0x4422F5 + 6), (BYTE)0x01);

    // .data:004A7280 82 6C 82 72 20 83 53 83+    asc_4A7280      db 'ＭＳ ゴシック',0
    char asc_4A7280[] = "SimHei";
    PatchWrite((PVOID)0x4A7280, asc_4A7280);

    // .data:004A73E8 96 BE 92 A9 00              asc_4A73E8      db '明朝',0
    char asc_4A73E8[] = "SimHei";
    PatchWrite((PVOID)0x4A73E8, asc_4A73E8);

    // .data:004A7300 81 78 81 76 00              asc_4A7300      db '』」',0
    char asc_4A7300[] = "\xA1\xBB\xA1\xB9";
    PatchWrite((PVOID)0x4A7300, asc_4A7300);

    // .data:004A7308 81 6A 81 76 00              asc_4A7308      db '）」',0
    char asc_4A7308[] = "\xA3\xA9\xA1\xB9";
    PatchWrite((PVOID)0x4A7308, asc_4A7308);

    // .data:004A7310 81 75 81 77 00              asc_4A7310      db '「『',0
    char asc_4A7310[] = "\xA1\xB8\xA1\xBA";
    PatchWrite((PVOID)0x4A7310, asc_4A7310);

    // .data:004A7318 81 75 81 69 00              asc_4A7318      db '「（',0
    char asc_4A7318[] = "\xA1\xB8\xA3\xA8";
    PatchWrite((PVOID)0x4A7318, asc_4A7318);

    // .data:004A74A8 81 78 00                    asc_4A74A8      db '』',0
    char asc_4A74A8[] = "\xA1\xBB";
    PatchWrite((PVOID)0x4A74A8, asc_4A74A8);

    // .data:004A74AC 81 77 00                    asc_4A74AC      db '『',0
    char asc_4A74AC[] = "\xA1\xBA";
    PatchWrite((PVOID)0x4A74AC, asc_4A74AC);

    // .data:004A74B0 81 6A 00                    asc_4A74B0      db '）',0
    char asc_4A74B0[] = "\xA3\xA9";
    PatchWrite((PVOID)0x4A74B0, asc_4A74B0);

    // .data:004A74B4 81 69 00                    asc_4A74B4      db '（',0
    char asc_4A74B4[] = "\xA3\xA8";
    PatchWrite((PVOID)0x4A74B4, asc_4A74B4);

    // .data:004A74B8 81 76 00                    asc_4A74B8      db '」',0
    char asc_4A74B8[] = "\xA1\xB9";
    PatchWrite((PVOID)0x4A74B8, asc_4A74B8);

    // .data:004A74BC 81 75 00                    asc_4A74BC      db '「',0 
    char asc_4A74BC[] = "\xA1\xB8";
    PatchWrite((PVOID)0x4A74BC, asc_4A74BC);

    // .data:004A74D0 81 75 81 77 81 69 81 79+    asc_4A74D0      db '「『（【‘“〔［｛〈《',0
    char asc_4A74D0[] = "\xA1\xB8\xA1\xBA\xA3\xA8\xA1\xBE\xA1\xAE\xA1\xB0\xA1\xB2\xA3\xDB\xA3\xFB\xA1\xB4\xA1\xB6";
    PatchWrite((PVOID)0x4A74D0, asc_4A74D0);

    // .data:004A74E8 81 40 00                    asc_4A74E8      db '　',0
    char asc_4A74E8[] = "\xA1\xA1";
    PatchWrite((PVOID)0x4A74E8, asc_4A74E8);

    // .data:004A7514 81 76 81 76 00              asc_4A7514      db '」」',0
    char asc_4A7514[] = "\xA1\xB9\xA1\xB9";
    PatchWrite((PVOID)0x4A7514, asc_4A7514);

    // .data:004A751C 81 75 81 75 00              asc_4A751C      db '「「',0
    char asc_4A751C[] = "\xA1\xB8\xA1\xB8";
    PatchWrite((PVOID)0x4A751C, asc_4A751C);

    // .data:004A7534 81 5B 81 41 81 42 81 43+    asc_4A7534      db 'ー、。，．」』）？！ぁぃぅぇぉっゃゅょゎァィゥェォャュョヮヵヶ゛゜ゝゞヽヾ〟・…：；‐】〕］｝〉》',0
    char asc_4A7534[] = "\xA9\x60\xA1\xA2\xA1\xA3\xA3\xAC\xA3\xAE\xA1\xB9\xA1\xBB\xA3\xA9\xA3\xBF\xA3\xA1\xA4\xA1\xA4\xA3\xA4\xA5\xA4\xA7\xA4\xA9\xA4\xC3\xA4\xE3\xA4\xE5\xA4\xE7\xA4\xEE\xA5\xA1\xA5\xA3\xA5\xA5\xA5\xA7\xA5\xA9\xA5\xE3\xA5\xE5\xA5\xE7\xA5\xEE\xA5\xF5\xA5\xF6\xA9\x61\xA9\x62\xA9\x66\xA9\x67\xA9\x63\xA9\x64\xA1\xA1\xA1\xA1\xA1\xAD\xA3\xBA\xA3\xBB\xA9\x5C\xA1\xBF\xA1\xB3\xA3\xDD\xA3\xFD\xA1\xB5\xA1\xB7";
    PatchWrite((PVOID)0x4A7534, asc_4A7534);

    // .text:0041EB1E C64 68 75 81 00 00                          push    8175h
    PatchWrite((PVOID)(0x41EB1E + 1), (DWORD)0xA1B8);

    // .text:0041EB3F C64 68 76 81 00 00                          push    8176h
    PatchWrite((PVOID)(0x41EB3F + 1), (DWORD)0xA1B9);

    // .text:0041EB6A C64 68 75 81 00 00                          push    8175h
    PatchWrite((PVOID)(0x41EB6A + 1), (DWORD)0xA1B8);

    // .text:0041F513 C64 68 F1 81 00 00                          push    81F1h
    PatchWrite((PVOID)(0x41F513 + 1), (DWORD)0xA1EB);

    // .text:0041F530 C64 68 F1 81 00 00                          push    81F1h
    PatchWrite((PVOID)(0x41F530 + 1), (DWORD)0xA1EB);

    // .text:0041F5FE C64 68 75 81 00 00                          push    8175h
    PatchWrite((PVOID)(0x41F5FE + 1), (DWORD)0xA1B8);

    // .text:0041F674 C78 68 76 81 00 00                          push    8176h
    PatchWrite((PVOID)(0x41F674 + 1), (DWORD)0xA1B9);
}


void InstallHooks()
{
    IATHook(NULL, "ADVAPI32.DLL", "RegOpenKeyA", MineRegOpenKeyA);
    IATHook(NULL, "ADVAPI32.DLL", "RegDeleteKeyA", MineRegDeleteKeyA);
    IATHook(NULL, "ADVAPI32.DLL", "RegCreateKeyExA", MineRegCreateKeyExA);
    IATHook(NULL, "USER32.DLL", "MessageBoxA", MineMessageBoxA);
}


//=============================================================================
// DLL Entry Point
//=============================================================================


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            // See https://github.com/microsoft/Detours/wiki/DetourRestoreAfterWith
            DetourRestoreAfterWith();

            // Open the log file
            // LogInit(L"dev.log");

            InstallPatches();
            InstallHooks();

            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}


//=============================================================================
// Dummy Export Symbol
//=============================================================================


BOOL APIENTRY CreateObject()
{
    return TRUE;
}
