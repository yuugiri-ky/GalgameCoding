// util.cpp

#include "util.h"


//=============================================================================
// Logger
//=============================================================================


static CAtlFile gLogFile;


void LogInit(LPCWSTR lpFileName)
{
    gLogFile.Create(lpFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_ALWAYS);
    gLogFile.Seek(0, FILE_END);
}


void LogWrite(LPCSTR lpMessage, ...)
{
    CStringA str;
    va_list args;

    va_start(args, lpMessage);
    str.FormatV(lpMessage, args);
    va_end(args);

    CStringW ucs = AnsiToUcs2(CP_ACP, str);
    CStringA utf = Ucs2ToUtf8(ucs);

    gLogFile.Write(utf.GetString(), utf.GetLength());
}


void LogWrite(LPCWSTR lpMessage, ...)
{
    CStringW str;
    va_list args;

    va_start(args, lpMessage);
    str.FormatV(lpMessage, args);
    va_end(args);

    CStringA utf = Ucs2ToUtf8(str);
    gLogFile.Write(utf.GetString(), utf.GetLength());
}


void LogWriteLine(LPCSTR lpMessage, ...)
{
    CStringA str;
    va_list args;

    time_t t;
    char tstr[32];
    time(&t);
    ctime_s(tstr, _countof(tstr), &t);

    str.Append(tstr);
    str.AppendChar(' ');

    va_start(args, lpMessage);
    str.AppendFormatV(lpMessage, args);
    va_end(args);

    str.Append("\r\n");

    CStringW ucs = AnsiToUcs2(CP_ACP, str);
    CStringA utf = Ucs2ToUtf8(ucs);

    gLogFile.Write(utf.GetString(), utf.GetLength());
}


void LogWriteLine(LPCWSTR lpMessage, ...)
{
    CStringW str;
    va_list args;

    time_t t;
    wchar_t tstr[32];
    time(&t);
    _wctime_s(tstr, _countof(tstr), &t);

    str.Append(tstr);
    str.AppendChar(L' ');

    va_start(args, lpMessage);
    str.AppendFormatV(lpMessage, args);
    va_end(args);

    str.Append(L"\r\n");

    CStringA utf = Ucs2ToUtf8(str);
    gLogFile.Write(utf.GetString(), utf.GetLength());
}


//=============================================================================
// Pattern Search Helper
//=============================================================================


PVOID GetModuleBase(HMODULE hModule)
{
    MEMORY_BASIC_INFORMATION mem;

    if (!VirtualQuery(hModule, &mem, sizeof(mem)))
        return NULL;

    return mem.AllocationBase;
}


DWORD GetModuleSize(HMODULE hModule)
{
    return ((IMAGE_NT_HEADERS*)((DWORD_PTR)hModule + ((IMAGE_DOS_HEADER*)hModule)->e_lfanew))->OptionalHeader.SizeOfImage;
}


ULONG SearchSignature(ULONG SearchAddress, ULONG SearchLength, PCSTR Signature, ULONG SignatureLength)
{
    auto s1 = reinterpret_cast<std::string_view::const_pointer>(SearchAddress);
    auto n1 = static_cast<std::string_view::size_type>(SearchLength);
    auto s2 = reinterpret_cast<std::string_view::const_pointer>(Signature);
    auto n2 = static_cast<std::string_view::size_type>(SignatureLength);

    std::string_view v1(s1, n1);
    std::string_view v2(s1, n2);

    auto pred = [](auto a, auto b) -> bool
    {
        return (b == 0x2a) || (a == b);
    };

    auto it = std::search(v1.begin(), v1.end(), v2.begin(), v2.end(), pred);

    if (it == v1.end())
        return NULL;

    return it - v1.begin() + SearchAddress;
}


//=============================================================================
// Patch Helper
//=============================================================================


void PatchRead(LPVOID lpAddr, LPVOID lpBuf, DWORD nSize)
{
    DWORD dwProtect;
    if (VirtualProtect(lpAddr, nSize, PAGE_EXECUTE_READWRITE, &dwProtect))
    {
        CopyMemory(lpBuf, lpAddr, nSize);
        VirtualProtect(lpAddr, nSize, dwProtect, &dwProtect);
    }
    else
    {
        FatalError("Failed to modify protection at %08X !", lpAddr);
    }
}


void PatchWrite(LPVOID lpAddr, LPCVOID lpBuf, DWORD nSize)
{
    DWORD dwProtect;
    if (VirtualProtect(lpAddr, nSize, PAGE_EXECUTE_READWRITE, &dwProtect))
    {
        CopyMemory(lpAddr, lpBuf, nSize);
        VirtualProtect(lpAddr, nSize, dwProtect, &dwProtect);
    }
    else
    {
        FatalError("Failed to modify protection at %08X !", lpAddr);
    }
}


void PatchNop(LPVOID lpAddr, int nCount)
{
    DWORD dwProtect;
    if (VirtualProtect(lpAddr, nCount, PAGE_EXECUTE_READWRITE, &dwProtect))
    {
        memset(lpAddr, 0x90, nCount);
        VirtualProtect(lpAddr, nCount, dwProtect, &dwProtect);
    }
    else
    {
        FatalError("Failed to modify protection at %08X !", lpAddr);
    }
}


//=============================================================================
// Hook Helper
//=============================================================================


static inline PBYTE RvaAdjust(_Pre_notnull_ PIMAGE_DOS_HEADER pDosHeader, _In_ DWORD raddr)
{
    if (raddr != NULL) {
        return ((PBYTE)pDosHeader) + raddr;
    }
    return NULL;
}


BOOL IATHook(HMODULE hModule, PCSTR pszFileName, PCSTR pszProcName, PVOID pNewProc)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (hModule == NULL)
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader + pDosHeader->e_lfanew);

    if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0)
        return FALSE;

    PIMAGE_IMPORT_DESCRIPTOR iidp = (PIMAGE_IMPORT_DESCRIPTOR)RvaAdjust(pDosHeader, pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    if (iidp == NULL)
        return FALSE;

    for (; iidp->OriginalFirstThunk != 0; iidp++)
    {
        PCSTR pszName = (PCHAR)RvaAdjust(pDosHeader, iidp->Name);

        if (pszName == NULL)
            return FALSE;

        if (_stricmp(pszName, pszFileName) != 0)
            continue;

        PIMAGE_THUNK_DATA pThunks = (PIMAGE_THUNK_DATA)RvaAdjust(pDosHeader, iidp->OriginalFirstThunk);
        PVOID* pAddrs = (PVOID*)RvaAdjust(pDosHeader, iidp->FirstThunk);

        if (pThunks == NULL)
            continue;

        for (DWORD nNames = 0; pThunks[nNames].u1.Ordinal; nNames++)
        {
            DWORD nOrdinal = 0;
            PCSTR pszFunc = NULL;

            if (IMAGE_SNAP_BY_ORDINAL(pThunks[nNames].u1.Ordinal))
                nOrdinal = (DWORD)IMAGE_ORDINAL(pThunks[nNames].u1.Ordinal);
            else
                pszFunc = (PCSTR)RvaAdjust(pDosHeader, (DWORD)pThunks[nNames].u1.AddressOfData + 2);

            if (pszFunc == NULL)
                continue;

            if (strcmp(pszFunc, pszProcName) != 0)
                continue;

            PatchWrite(&pAddrs[nNames], pNewProc);
            return TRUE;
        }
    }

    return FALSE;
}


//=============================================================================
// Encoding
//=============================================================================


CStringW AnsiToUcs2(int cp, const CStringA& str)
{
    if (str.GetLength() == 0)
        return CStringW();
    int nLen = MultiByteToWideChar(cp, 0, str.GetString(), str.GetLength(), NULL, 0);
    if (nLen == 0)
        return CStringW();
    CStringW ret(L'\0', nLen);
    if (MultiByteToWideChar(cp, 0, str.GetString(), str.GetLength(), ret.GetBuffer(), ret.GetAllocLength()) == 0)
        return CStringW();
    return ret;
}


CStringA Ucs2ToAnsi(int cp, const CStringW& str)
{
    if (str.GetLength() == 0)
        return CStringA();
    int nLen = WideCharToMultiByte(cp, 0, str.GetString(), str.GetLength(), NULL, 0, NULL, NULL);
    if (nLen == 0)
        return CStringA();
    CStringA ret('\0', nLen);
    if (WideCharToMultiByte(cp, 0, str.GetString(), str.GetLength(), ret.GetBuffer(), ret.GetAllocLength(), NULL, NULL) == 0)
        return CStringA();
    return ret;
}


CStringW Utf8ToUcs2(const CStringA& str)
{
    return AnsiToUcs2(CP_UTF8, str);
}


CStringA Ucs2ToUtf8(const CStringW& str)
{
    return Ucs2ToAnsi(CP_UTF8, str);
}


CStringW ShiftJisToUcs2(const CStringA& str)
{
    return AnsiToUcs2(CP_SHIFTJIS, str);
}


CStringA Ucs2ToShiftJis(const CStringW& str)
{
    return Ucs2ToAnsi(CP_SHIFTJIS, str);
}


CStringW GbkToUcs2(const CStringA& str)
{
    return AnsiToUcs2(CP_GBK, str);
}


CStringA Ucs2ToGbk(const CStringW& str)
{
    return Ucs2ToAnsi(CP_GBK, str);
}


//=============================================================================
// File & Path
//=============================================================================


CPathA GetAppDirectoryA()
{
    CHAR szPath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), szPath, ARRAYSIZE(szPath));
    if (GetLastError() != ERROR_SUCCESS)
        return CPathA();
    CPathA ret(szPath);
    if (ret.RemoveFileSpec() != TRUE)
        return CPathA();
    return ret;
}


CPathW GetAppDirectoryW()
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleW(NULL), szPath, ARRAYSIZE(szPath));
    if (GetLastError() != ERROR_SUCCESS)
        return CPathW();
    CPathW ret(szPath);
    if (ret.RemoveFileSpec() != TRUE)
        return CPathW();
    return ret;
}


CPathA GetAppPathA()
{
    CHAR szPath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), szPath, ARRAYSIZE(szPath));
    if (GetLastError() != ERROR_SUCCESS)
        return CPathA();
    return CPathA(szPath);
}


CPathW GetAppPathW()
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleW(NULL), szPath, ARRAYSIZE(szPath));
    if (GetLastError() != ERROR_SUCCESS)
        return CPathW();
    return CPathW(szPath);
}


//=============================================================================
// Error Handling
//=============================================================================


__declspec(noreturn) void FatalError(LPCSTR lpMessage, ...)
{
    CStringA strMsg;
    va_list args;

    va_start(args, lpMessage);
    strMsg.FormatV(lpMessage, args);
    va_end(args);

    MessageBoxA(GetActiveWindow(), strMsg, "Fatal Error", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}


__declspec(noreturn) void FatalError(LPCWSTR lpMessage, ...)
{
    CStringW strMsg;
    va_list args;

    va_start(args, lpMessage);
    strMsg.FormatV(lpMessage, args);
    va_end(args);

    MessageBoxW(GetActiveWindow(), strMsg, L"Fatal Error", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}


//=============================================================================
// PE Helper
//=============================================================================


PIMAGE_SECTION_HEADER FindSectionFromModule(HMODULE hModule, PCSTR pName)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (hModule == NULL)
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader + pDosHeader->e_lfanew);

    if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
        return NULL;

    if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0)
        return NULL;

    PIMAGE_SECTION_HEADER pSectionHeaders = (PIMAGE_SECTION_HEADER)((PBYTE)pNtHeader + sizeof(pNtHeader->Signature) + sizeof(pNtHeader->FileHeader) + pNtHeader->FileHeader.SizeOfOptionalHeader);

    for (DWORD n = 0; n < pNtHeader->FileHeader.NumberOfSections; n++)
    {
        if (strcmp((PCHAR)pSectionHeaders[n].Name, pName) == 0)
        {
            if (pSectionHeaders[n].VirtualAddress == 0 || pSectionHeaders[n].SizeOfRawData == 0)
                return NULL;

            return &pSectionHeaders[n];
        }
    }

    return NULL;
}


//=============================================================================
// GUI
//=============================================================================


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


static HANDLE ActCtx = INVALID_HANDLE_VALUE;
static ULONG_PTR ActCookie = 0;

void InitComCtl(HMODULE hModule)
{
    if (ActCtx != INVALID_HANDLE_VALUE)
        return;

    ACTCTXW ctx = {};
    ctx.cbSize = sizeof(ctx);
    ctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
    ctx.lpResourceName = MAKEINTRESOURCEW(2);
    ctx.hModule = hModule;
    // This sample implies your DLL stores Common Controls version 6.0 manifest in its resources with ID 2.

    ActCtx = CreateActCtxW(&ctx);

    if (ActCtx == INVALID_HANDLE_VALUE)
        return;

    ActivateActCtx(ActCtx, &ActCookie);
}


void ReleaseComCtl()
{
    if (ActCtx == INVALID_HANDLE_VALUE)
        return;

    DeactivateActCtx(0, ActCookie);
    ReleaseActCtx(ActCtx);
}

