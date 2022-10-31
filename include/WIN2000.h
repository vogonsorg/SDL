// This file covers all missing dll export functions to get sdl2 working on Windows 2000 and XP SP1

#ifndef __WIN2000_H__
#define __WIN2000_H__

#include "myList.h"
#include <dinput.h>

#define MOUSE_DEVICE_HANDLE  0x1337
#define KBD_DEVICE_HANDLE    0xF001
#define RAWINPUTHANDLETABLESIZE     32

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define MAX_DEVICE_NAME      32
#define MOUSE_DEVICE_HANDLE  0x1337
#define KBD_DEVICE_HANDLE    0xF001
#define INFO_DEVICE_NAME     "Win2kXPRawInputQuery"

#define FILE_ANY_ACCESS                 0
#define METHOD_BUFFERED                 0

#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MOUSE               0x0000000f

#define IOCTL_MOUSE_QUERY_ATTRIBUTES         CTL_CODE(FILE_DEVICE_MOUSE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_ATTRIBUTES      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef HRESULT (WINAPI *DirectInput8Create_t) (HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);

typedef struct _MOUSE_ATTRIBUTES {
    USHORT MouseIdentifier;
    USHORT NumberOfButtons;
    USHORT SampleRate;
    ULONG  InputDataQueueLength;
} MOUSE_ATTRIBUTES, *PMOUSE_ATTRIBUTES;

typedef struct _KEYBOARD_ID {
    UCHAR Type;       // Keyboard type
    UCHAR Subtype;    // Keyboard subtype (OEM-dependent value)
} KEYBOARD_ID, *PKEYBOARD_ID;

typedef struct _KEYBOARD_TYPEMATIC_PARAMETERS {
    USHORT  UnitId;
    USHORT  Rate;
    USHORT  Delay;
} KEYBOARD_TYPEMATIC_PARAMETERS, *PKEYBOARD_TYPEMATIC_PARAMETERS;

typedef struct _KEYBOARD_ATTRIBUTES {
  KEYBOARD_ID  KeyboardIdentifier;
  USHORT  KeyboardMode;
  USHORT  NumberOfFunctionKeys;
  USHORT  NumberOfIndicators;
  USHORT  NumberOfKeysTotal;
  ULONG  InputDataQueueLength;
  KEYBOARD_TYPEMATIC_PARAMETERS  KeyRepeatMinimum;
  KEYBOARD_TYPEMATIC_PARAMETERS  KeyRepeatMaximum;
} KEYBOARD_ATTRIBUTES, *PKEYBOARD_ATTRIBUTES;

//kernel32
BOOL XP_AttachConsole(DWORD dwProcessId);

//user32
void SetRawInputMouseHeader(RAWINPUT *ri);
VOID AddRawInputToWindowList(RAWINPUT *ri, myList *windowList);
RAWINPUT *GetDirectInputMouseData();
DWORD WINAPI PollDirectInputMouse(PVOID data);
VOID DeRegisterDirectInputMouse();
BOOL RegisterDirectInputMouse(PCRAWINPUTDEVICE pRawInputDevices, HWND hWnd);
LRESULT CALLBACK myLowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK myLowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
BOOL XP_RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);

BOOL GetKeyboardDeviceInfo(KEYBOARD_ATTRIBUTES *attrib);
BOOL GetMouseDeviceInfo(MOUSE_ATTRIBUTES *attrib);
UINT XP_GetRawInputDeviceInfoA(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize);
UINT XP_GetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize);

#endif