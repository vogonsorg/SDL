/*
  Windows 2000 XP API Wrapper Pack
  Copyright (C) 2008 OldCigarette

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __win2k_xp_debug_h__
#define __win2k_xp_debug_h__

#define DBG_ALWAYS 0
#define DBG_ERROR  1
#define DBG_WARN   2
#define DBG_INFO   3
#define DBG_TRACE  4

#define S_BLACK  "^0"
#define S_RED    "^1"
#define S_GREEN  "^2"
#define S_YELLOW "^3"
#define S_BLUE   "^4"
#define S_CYAN   "^5"
#define S_PINK   "^6"
#define S_WHITE  "^7"

#define STUB(s) DbgPrintf(DBG_WARN, S_YELLOW "STUB: " s "\n")

#ifdef WIN2K_XP_DEBUG_STATIC

extern BOOL debug;
void __cdecl InitDebug();
void __cdecl DbgPrintf(int, LPCSTR text, ...);
void __cdecl vDbgPrintf(int, LPCSTR text, va_list va);
int  __cdecl DebugLevel();

#else

typedef void (__cdecl *DbgPrintf_t) (int, LPCSTR, ...);
typedef void (__cdecl *vDbgPrintf_t) (int, LPCSTR, va_list va);
typedef int (__cdecl *DebugLevel_t) ();

__inline DbgPrintf_t GetDbgPrintf() {
	return (DbgPrintf_t)GetProcAddress(GetModuleHandle("KERNEL32.DLL"), "DbgPrintf");
}

__inline vDbgPrintf_t GetvDbgPrintf() {
	return (vDbgPrintf_t)GetProcAddress(GetModuleHandle("KERNEL32.DLL"), "vDbgPrintf");
}

__inline DebugLevel_t GetDebugLevel() {
	return (DebugLevel_t)GetProcAddress(GetModuleHandle("KERNEL32.DLL"), "DebugLevel");
}

#endif

#endif
