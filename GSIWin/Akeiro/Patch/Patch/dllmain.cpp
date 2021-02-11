// dllmain.cpp

#include "util.h"


//=============================================================================
// Globals
//=============================================================================


HMODULE hAppModule = NULL;
HMODULE hThisModule = NULL;


//=============================================================================
// Files
//=============================================================================


//=============================================================================
// Patches
//=============================================================================


static const DWORD DBCSRVA[] =
{
    0x2868E,
    0x28C2A,
    0x28CCF,
    0x28E20,
    0x3DA26,
    0x40C43,
    0x40D22,
    0x55446,
    0x6C333,
    0x6C417,
    0x7C8D3,
    0x8C8C4,
    0x8C934,
    0x8C967,
    0xA09F3,
    0xA52A6,
    0xB1A79,
    0xB38C3,
    0xC7402,
    0xC7C55,
    0xCB17E,
    0xCB939,
    0xCBCFB,
    0xCC515,
};

void InstallDBCSPatch(ULONG_PTR ModuleBase)
{
    PVOID   addr;
    BYTE    code[10];
    DWORD   i;

    for (i = 0; i < ARRAYSIZE(DBCSRVA); i++)
    {
        addr = (PVOID)(ModuleBase + DBCSRVA[i]);

        PatchRead(addr, code, 10);

        switch (code[0])
        {
            case 0x3C:
            {
                code[5] = 0xFE;
                break;
            }
            case 0x80:
            {
                code[7] = 0xFE;
                break;
            }
            default:
                FatalError(L"读取到错误的代码");
        }

        PatchWrite(addr, code, 10);
    }
}


static const char Font[] = { 'S', 'i', 'm', 'H', 'e', 'i', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const char Node[] = { '/', 'F', 'o', 'n', 't', '/', 'F', 'x', 'x', 'x', 0x00, 0x00 };

void InstallFontPatch(ULONG_PTR ModuleBase)
{
    PVOID   addr;
    BYTE    code;

    /*******************************************************************************
     * 修改字体名
     *
     * 00532FF4  82 6C 82 72 20 83 53 83 56 83 62 83 4E 00 00 00  .l.r .S.V.b.N...      | 原版
     * 00532FF4  53 69 6D 48 65 69 00 00 00 00 00 00 00 00 00 00  SimHei..........      | 修改
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0x132FF4);
    PatchWrite(addr, Font, 16);

    /*******************************************************************************
     * 不要读XML里的字体名
     *
     * 005352A0  2F 46 6F 6E 74 2F 46 61 63 65 00 00 83 6E 81 5B  /Font/Face...n.[      | 原版
     * 005352A0  2F 46 6F 6E 74 2F 46 78 78 78 00 00 83 6E 81 5B  /Font/Fxxx...n.[      | 修改
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0x1352A0);
    PatchWrite(addr, Node, 12);

    /*******************************************************************************
     * 修改 LOGFONTA.lfCharSet 的值为 ANSI_CHARSET
     *
     * 004A4E51 | C645 C7 80        | mov byte ptr ss:[ebp-39],80           | 原版
     * 004A4E51 | C645 C7 80        | mov byte ptr ss:[ebp-39],0            | 修改
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0xA4E54);
    code = ANSI_CHARSET;
    PatchWrite(addr, code);

    /*******************************************************************************
     * 修改 LOGFONTA.lfCharSet 的值为 ANSI_CHARSET
     *
     * 004A51A7 | C745 D4 00000080  | mov dword ptr ss:[ebp-2C],80000000    | 原版
     * 004A51A7 | C745 D4 00000000  | mov dword ptr ss:[ebp-2C],0           | 修改
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0xA51AD);
    code = ANSI_CHARSET;
    PatchWrite(addr, code);
}


void InstallScriptPatch(ULONG_PTR ModuleBase)
{
    PVOID   addr;
    BYTE    b;
    DWORD   dw;

    /*******************************************************************************
     * 去掉解密计算
     *
     * 004A09EE | B9 627D0000       | mov ecx,7D62                          | 原版
     * 004A09EE | B9 627D0000       | mov ecx,0                             | 修改
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0xA09EF);
    dw = 0;
    PatchWrite(addr, dw);

    /*******************************************************************************
     * 读出来的英文字符存储在EBX低位，原版的代码是MOV高位，MOV了个寂寞
     *
     * 004A0B5C | 883C08            | mov byte ptr ds:[eax+ecx],bh          | 原版
     * 004A0B5C | 881C08            | mov byte ptr ds:[eax+ecx],bl          | 修改
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0xA0B5D);
    b = 0x1C;
    PatchWrite(addr, b);

    /*******************************************************************************
     * 不知道为什么要复制两遍，导致英文字符重复，NOP掉后面这一段
     *
     * 004A0BA1 | 83FA 10           | cmp edx,10                            |
     * 004A0BA4 | 8D45 D4           | lea eax,dword ptr ss:[ebp-2C]         |
     * 004A0BA7 | 0F4345 D4         | cmovae eax,dword ptr ss:[ebp-2C]      |
     * 004A0BAB | 881C01            | mov byte ptr ds:[ecx+eax],bl          |
     * 004A0BAE | 8D45 D4           | lea eax,dword ptr ss:[ebp-2C]         |
     * 004A0BB1 | 837D E8 10        | cmp dword ptr ss:[ebp-18],10          |
     * 004A0BB5 | 8975 E4           | mov dword ptr ss:[ebp-1C],esi         |
     * 004A0BB8 | 0F4345 D4         | cmovae eax,dword ptr ss:[ebp-2C]      |
     * 004A0BBC | C60406 00         | mov byte ptr ds:[esi+eax],0           |
     *
    *******************************************************************************/
    addr = (PVOID)(ModuleBase + 0xA0BA1);
    PatchNop(addr, 31);
}


#include "str.h"

void FindStringRefs()
{
    PIMAGE_SECTION_HEADER pSection = FindSectionFromModule(hAppModule, ".text");
    ULONG_PTR ImageBase = (ULONG_PTR)hAppModule;

    for (int i = 0; i < _countof(StringDefs); i++)
    {
        ULONG_PTR Base = ImageBase + pSection->VirtualAddress;
        ULONG_PTR Size = pSection->Misc.VirtualSize;

        auto s1 = reinterpret_cast<std::string_view::const_pointer>(Base);
        auto n1 = static_cast<std::string_view::size_type>(Size);
        std::string_view v1(s1, n1);

        std::vector<size_t> refs;

        {
            #pragma pack(push, 1)
            struct {
                BYTE Op;    // 68 [?? ?? ?? ??]
                LONG Va;
            } Code;
            #pragma pack(pop)

            // push asc_xxxxxxxx
            Code.Op = 0x68;
            Code.Va = ImageBase + StringDefs[i].Rva;

            auto s2 = reinterpret_cast<std::string_view::const_pointer>(&Code);
            auto n2 = static_cast<std::string_view::size_type>(sizeof(Code));
            std::string_view v2(s2, n2);

            auto off = v1.find(v2, 0);

            while (off != std::string_view::npos)
            {
                LogWrite(L"REF(0x%03X, 0x%05X), // push ??", i, off + 1 + pSection->VirtualAddress);

                refs.push_back(off + 1);
                off = v1.find(v2, off + 1);
            }
        }

        {
            #pragma pack(push, 1)
            struct {
                BYTE Op[4];    // C7 44 24 ?? [?? ?? ?? ??]
                LONG Va;

            } Code;
            #pragma pack(pop)

            // mov [esp+??], asc_xxxxxxxx
            Code.Op[0] = 0xC7;
            Code.Op[1] = 0x44;
            Code.Op[2] = 0x24;
            Code.Op[3] = 0x2A; // *
            Code.Va = ImageBase + StringDefs[i].Rva;

            auto s2 = reinterpret_cast<std::string_view::const_pointer>(&Code);
            auto n2 = static_cast<std::string_view::size_type>(sizeof(Code));
            std::string_view v2(s2, n2);

            auto pred = [](char a, char b) -> bool
            {
                return b == 0x2A || a == b;
            };

            auto it = std::search(v1.begin(), v1.end(), v2.begin(), v2.end(), pred);

            while (it < v1.end())
            {
                auto off = it - v1.begin();
                refs.push_back(off + 4);

                LogWrite(L"REF(0x%03X, 0x%05X), // mov [esp+??], ??", i, off + 4 + pSection->VirtualAddress);

                it++;
                it = std::search(it, v1.end(), v2.begin(), v2.end(), pred);
            }
        }

        auto cch = WideCharToMultiByte(936, 0, StringDefs[i].Str, -1, NULL, 0, NULL, NULL);
        auto ptr = new char[cch];
        WideCharToMultiByte(936, 0, StringDefs[i].Str, -1, ptr, cch, NULL, NULL);

        if (ptr)
        {
            for (auto off : refs)
            {
                PVOID addr = (PVOID)(off + Base);
                PatchWrite(addr, ptr);
            }
        }
    }
}


void InstallTextPatch(ULONG_PTR ModuleBase)
{
    for (int i = 0; i < _countof(StringRefs); i++)
    {
        auto& ref = StringRefs[i];
        auto& def = StringDefs[ref.Idx];

        if (def.Ptr == NULL)
        {
            auto cch = WideCharToMultiByte(CP_GBK, 0, def.Str, -1, NULL, 0, NULL, NULL);
            auto ptr = new CHAR[cch];
            WideCharToMultiByte(CP_GBK, 0, def.Str, -1, ptr, cch, NULL, NULL);
            def.Ptr = ptr;
        }

        if (def.Ptr == 0)
            FatalError(L"空字符串指针");

        auto addr = (PVOID)(ModuleBase + ref.Rva);
        PatchWrite(addr, def.Ptr);
    }
}


void InstallFilePatch(ULONG_PTR ModuleBase)
{
    PVOID   addr;
    PCSTR   str;

    /*******************************************************************************
     * 读汉化脚本封包
     * 
     * .text:004CD775 00C 68 14 50 53 00            push    offset asc_535014 ; "script.arc"
     * .text:004CD783 010 68 20 50 53 00            push    offset asc_535020 ; "/GameFile/ScriptArchive/File"
     * 
    *******************************************************************************/
    str = "script.chs.arc";
    addr = (PVOID)(ModuleBase + 0xCD776);
    PatchWrite(addr, str);

    str = "/GameFile/ScriptArchive/Fxxx";
    addr = (PVOID)(ModuleBase + 0xCD784);
    PatchWrite(addr, str);
}


//=============================================================================
// Hooks
//=============================================================================


auto pfnLoadMenuA = LoadMenuA;

HMENU WINAPI MineLoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName)
{
    if (hInstance == hAppModule)
    {
        if (lpMenuName == MAKEINTRESOURCEA(105))
            return pfnLoadMenuA(hThisModule, lpMenuName);
    }

    return pfnLoadMenuA(hInstance, lpMenuName);
}


auto pfnLoadStringA = LoadStringA;

int WINAPI MineLoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax)
{
    if (hInstance == hAppModule)
    {
        if (uID == 101)
            return pfnLoadStringA(hThisModule, uID, lpBuffer, cchBufferMax);
    }

    return pfnLoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}


auto pfnDialogBoxParamA = DialogBoxParamA;

INT_PTR WINAPI MineDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    if (hInstance == hAppModule)
    {
        InitComCtl(hThisModule);

        if (lpTemplateName == MAKEINTRESOURCEA(111) || lpTemplateName == MAKEINTRESOURCEA(115))
            return pfnDialogBoxParamA(hThisModule, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }

    return pfnDialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}


auto pfnCreateDialogParamA = CreateDialogParamA;

HWND WINAPI MineCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    if (hInstance == hAppModule)
    {
        if (lpTemplateName == MAKEINTRESOURCEA(113) || lpTemplateName == MAKEINTRESOURCEA(114))
            return pfnCreateDialogParamA(hThisModule, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }

    return pfnCreateDialogParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}


auto pfnMessageBoxA = MessageBoxA;

int WINAPI MineMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    CStringW textA = ShiftJisToUcs2(lpText);
    CStringW textB = GbkToUcs2(lpText);
    CStringW title = ShiftJisToUcs2(lpCaption);

    //LogWrite(L"MessageBox: [JIS](%s) [GBK](%s)", textA.GetString(), textB.GetString());

    return MessageBoxW(hWnd, textA.GetString(), title.GetString(), uType);
}


//=============================================================================
// Startup Code
//=============================================================================


void InstallFiles()
{
}


void InstallPatches()
{
    ULONG_PTR ModuleBase = (ULONG_PTR)hAppModule;

    //FindStringRefs();

    InstallDBCSPatch(ModuleBase);
    InstallFontPatch(ModuleBase);
    InstallScriptPatch(ModuleBase);
    InstallTextPatch(ModuleBase);
    InstallFilePatch(ModuleBase);
}


void InstallHooks()
{
    IATHook(hAppModule, "user32.dll", "LoadMenuA", MineLoadMenuA);
    IATHook(hAppModule, "user32.dll", "LoadStringA", MineLoadStringA);
    IATHook(hAppModule, "user32.dll", "DialogBoxParamA", MineDialogBoxParamA);
    IATHook(hAppModule, "user32.dll", "CreateDialogParamA", MineCreateDialogParamA);
    IATHook(hAppModule, "user32.dll", "MessageBoxA", MineMessageBoxA);
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

            hAppModule = GetModuleHandleW(NULL);
            hThisModule = hModule;

            InstallFiles();
            InstallPatches();
            InstallHooks();

            break;
        }
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
        {
            ReleaseComCtl();
            break;
        }
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
