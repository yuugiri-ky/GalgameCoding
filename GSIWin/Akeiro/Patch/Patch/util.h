// util.h
#pragma once


//=============================================================================
// Windows SDK Headers
//=============================================================================


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#include <intrin.h>
#include <pathcch.h>
#include <strsafe.h>
#include <atlstr.h>

#pragma warning(push)
#pragma warning(disable:4995)
#include <atlpath.h>
#pragma warning(pop)

#include <atlfile.h>


//=============================================================================
// Detour Header
//=============================================================================


#include <detours.h>
#include "util_detours.h"


//=============================================================================
// C Runtime Headers
//=============================================================================


#include <time.h>


//=============================================================================
// C++ Runtime Headers
//=============================================================================


#include <algorithm>
using std::remove_if;
using std::remove_copy_if;


#include <string_view>
using std::string_view;

#include <string>
using std::string;
using std::wstring;

#include <functional>

#include <memory>
using std::shared_ptr;
using std::make_shared;
using std::unique_ptr;
using std::make_unique;


#include <list>
using std::list;


#include <vector>
using std::vector;


#include <map>
using std::map;


#include <unordered_map>
using std::unordered_map;


#include <set>
using std::set;


//=============================================================================
// Logger
//=============================================================================


void LogInit(LPCWSTR lpFileName);
void LogWrite(LPCSTR lpMessage, ...);
void LogWrite(LPCWSTR lpMessage, ...);
void LogWriteLine(LPCSTR lpMessage, ...);
void LogWriteLine(LPCWSTR lpMessage, ...);


//=============================================================================
// Pattern Search Helper
//=============================================================================


PVOID GetModuleBase(HMODULE hModule);
DWORD GetModuleSize(HMODULE hModule);
ULONG SearchSignature(ULONG SearchAddress, ULONG SearchLength, PCSTR Signature, ULONG SignatureLength);

template<class T>
constexpr ULONG sizeofsig(const T& x) { return sizeof(x) - 1; }


//=============================================================================
// Patch Helper
//=============================================================================


void PatchRead(LPVOID lpAddr, LPVOID lpBuf, DWORD nSize);
void PatchWrite(LPVOID lpAddr, LPCVOID lpBuf, DWORD nSize);


void PatchNop(LPVOID lpAddr, int nCount);


template<class T>
void PatchRead(LPVOID lpAddr, T& lpBuf)
{
    PatchRead(lpAddr, &lpBuf, sizeof(T));
}


template<class T>
void PatchWrite(LPVOID lpAddr, T&& lpBuf)
{
    PatchWrite(lpAddr, &lpBuf, sizeof(T));
}


//=============================================================================
// Memory Address Helper
//=============================================================================


template<class T = DWORD_PTR>
inline constexpr DWORD_PTR MakeRVA(T base, DWORD_PTR va)
{
    return (DWORD_PTR)((DWORD_PTR)va - (DWORD_PTR)base);
}


template<class T = DWORD_PTR>
inline constexpr PVOID MakeVA(T base, DWORD_PTR rva)
{
    return (PVOID)((DWORD_PTR)base + (DWORD_PTR)rva);
}


//=============================================================================
// Hook Helper
//=============================================================================


template<class T>
void InlineHook(T& OriginalFunction, T DetourFunction)
{
    DetourUpdateThread(GetCurrentThread());
    DetourTransactionBegin();
    DetourAttach(&(PVOID&)OriginalFunction, (PVOID&)DetourFunction);
    DetourTransactionCommit();
}


template<class T>
void UnInlineHook(T& OriginalFunction, T DetourFunction)
{
    DetourUpdateThread(GetCurrentThread());
    DetourTransactionBegin();
    DetourDetach(&(PVOID&)OriginalFunction, (PVOID&)DetourFunction);
    DetourTransactionCommit();
}

BOOL IATHook(HMODULE hModule, PCSTR pszFileName, PCSTR pszProcName, PVOID pNewProc);


//=============================================================================
// Encoding
//=============================================================================


#define CP_SHIFTJIS 932
#define CP_GBK 936


CStringW AnsiToUcs2(int cp, const CStringA& str);
CStringA Ucs2ToAnsi(int cp, const CStringW& str);
CStringW Utf8ToUcs2(const CStringA& str);
CStringA Ucs2ToUtf8(const CStringW& str);
CStringW ShiftJisToUcs2(const CStringA& str);
CStringA Ucs2ToShiftJis(const CStringW& str);
CStringW GbkToUcs2(const CStringA& str);
CStringA Ucs2ToGbk(const CStringW& str);


//=============================================================================
// File & Path
//=============================================================================


CPathA GetAppDirectoryA();
CPathW GetAppDirectoryW();
CPathA GetAppPathA();
CPathW GetAppPathW();


//=============================================================================
// Error Handling
//=============================================================================


__declspec(noreturn) void FatalError(LPCSTR lpMessage, ...);
__declspec(noreturn) void FatalError(LPCWSTR lpMessage, ...);


//=============================================================================
// PE Helper
//=============================================================================


PIMAGE_SECTION_HEADER FindSectionFromModule(HMODULE hModule, PCSTR pName);


//=============================================================================
// GUI
//=============================================================================


void InitComCtl(HMODULE hModule);
void ReleaseComCtl();
