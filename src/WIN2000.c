#include "WIN2000.h"

/**kernel32**/
BOOL XP_AttachConsole(DWORD dwProcessId) {
	/*if (dwProcessId != GetCurrentProcessId())
		//DbgPrintf(DBG_WARN, S_YELLOW "kernel32: AttachConsole does not support other processes");
		MssageBox(NULL, "kernel32: AttachConsole does not support other processes", "Info", MB_OK);
	*/
	GetCurrentProcessId();
		//Just make sure we have a console
	AllocConsole();
	return TRUE;
}

/**user32**/
int UseDirectInput;
DirectInput8Create_t dlDirectInput8Create;

/*HANDLE to the heap*/
HANDLE                hHeap = NULL;

static HHOOK rawInputMouseHook = NULL;
static HHOOK rawInputKeyboardHook = NULL;

static myList * rawMouseInputWindowList    = NULL;
static myList * rawKeyboardInputWindowList = NULL;
static myList * rawInputHandleList         = NULL;

static BOOL  mouseFirstRun = TRUE;

LPDIRECTINPUT8         lpdi            = NULL;
LPDIRECTINPUTDEVICE8   m_mouse         = NULL;
LPDIRECTINPUTDEVICE8   m_keyboard      = NULL;
HANDLE                 mouseInputEvent = NULL;

BOOL b_registered_mouse;
BOOL b_registered_keyboard;
RAWINPUTDEVICE rid_mouse;
RAWINPUTDEVICE rid_keyboard;

DIOBJECTDATAFORMAT c_rgodfDIMouse2[11] = {
{ &GUID_XAxis, 0, 0x00FFFF03, 0 },
{ &GUID_YAxis, 4, 0x00FFFF03, 0 },
{ &GUID_ZAxis, 8, 0x80FFFF03, 0 },
{ NULL, 12, 0x00FFFF0C, 0 },
{ NULL, 13, 0x00FFFF0C, 0 },
{ NULL, 14, 0x80FFFF0C, 0 },
{ NULL, 15, 0x80FFFF0C, 0 },
{ NULL, 16, 0x80FFFF0C, 0 },
{ NULL, 17, 0x80FFFF0C, 0 },
{ NULL, 18, 0x80FFFF0C, 0 },
{ NULL, 19, 0x80FFFF0C, 0 }
};
const DIDATAFORMAT c_dfDIMouse2 = { 24, 16, 0x2, 20, 11, c_rgodfDIMouse2 }; 

//X,Y coords to create relative movement of mouse from low level hook
static LONG mouseHookLastX = 0, mouseHookLastY = 0;

void SetRawInputMouseHeader(RAWINPUT *ri) {
	ri->header.dwType = RIM_TYPEMOUSE;
    ri->header.dwSize = sizeof(RAWINPUT);
    ri->header.hDevice = MOUSE_DEVICE_HANDLE;
    ri->header.wParam = RIM_INPUT;
	return;	
}

VOID AddRawInputToWindowList(RAWINPUT *ri, myList *windowList) {
	myNode * cour = NULL;
	GetLock(rawInputHandleList);
	
	//Remove old rawinputs
	if(rawInputHandleList && rawInputHandleList->length > RAWINPUTHANDLETABLESIZE)
		HeapFree(hHeap, 0, (RAWINPUT *)PopFrontAndFree(rawInputHandleList));

	//Add this new RAWINPUT
    PushBackByVal(rawInputHandleList, (DWORD)ri);

    GetLock(windowList);
    for(cour = windowList->head; cour; cour = cour->next) {
		PostMessageA((HWND)cour->data, WM_INPUT, RIM_INPUT, (LPARAM)ri);
    }				
    ReleaseLock(windowList);
	ReleaseLock(rawInputHandleList);
	return;
}

RAWINPUT *GetDirectInputMouseData() {
	static DIMOUSESTATE2 mouse_state;
	static DIMOUSESTATE2 mouse_state_prev;
	RAWINPUT *raw;
	HRESULT hr;
	int i;

	raw = (RAWINPUT*)HeapAlloc(hHeap, 0, sizeof(RAWINPUT));
	SetRawInputMouseHeader(raw);

	//Try to get input, and maybe reaquire input
	hr = m_mouse->lpVtbl->GetDeviceState(m_mouse, sizeof(DIMOUSESTATE2), (LPVOID)&mouse_state);
	if(FAILED(hr)) {
		m_mouse->lpVtbl->Acquire(m_mouse);
		while(hr == DIERR_INPUTLOST) {
			hr = m_mouse->lpVtbl->Acquire(m_mouse);
		}
		if(FAILED(hr)) {
			HeapFree(hHeap, 0, raw);
			return NULL;
		}
		m_mouse->lpVtbl->GetDeviceState(m_mouse, sizeof(DIMOUSESTATE2), (LPVOID)&mouse_state);
	}

	raw->data.mouse.usFlags = MOUSE_MOVE_RELATIVE;
	raw->data.mouse.lLastX = mouse_state.lX;
	raw->data.mouse.lLastY = mouse_state.lY;
	raw->data.mouse.usButtonData = mouse_state.lZ & 0xFFFF;
	raw->data.mouse.usButtonFlags = 0;
	raw->data.mouse.ulRawButtons = 0;

	if(raw->data.mouse.usButtonData != 0) raw->data.mouse.usButtonFlags |= RI_MOUSE_WHEEL;

	for(i = 0; i < 8; i++) {
		if(mouse_state.rgbButtons[i] & 0x80) {
			raw->data.mouse.ulRawButtons |= 1<<i;
		}
	}

	if(mouse_state.rgbButtons[0] & 0x80 && !(mouse_state_prev.rgbButtons[0] & 0x80))
		raw->data.mouse.usButtonFlags |= RI_MOUSE_LEFT_BUTTON_DOWN;

	if(!(mouse_state.rgbButtons[0] & 0x80) && mouse_state_prev.rgbButtons[0] & 0x80)
		raw->data.mouse.usButtonFlags |= RI_MOUSE_LEFT_BUTTON_UP;

	if(mouse_state.rgbButtons[1] & 0x80 && !(mouse_state_prev.rgbButtons[1] & 0x80))
		raw->data.mouse.usButtonFlags |= RI_MOUSE_RIGHT_BUTTON_DOWN;

	if(!(mouse_state.rgbButtons[1] & 0x80) && mouse_state_prev.rgbButtons[1] & 0x80)
		raw->data.mouse.usButtonFlags |= RI_MOUSE_RIGHT_BUTTON_UP;

	if(mouse_state.rgbButtons[2] & 0x80 && !(mouse_state_prev.rgbButtons[2] & 0x80))
		raw->data.mouse.usButtonFlags |= RI_MOUSE_MIDDLE_BUTTON_DOWN;

	if(!(mouse_state.rgbButtons[2] & 0x80) && mouse_state_prev.rgbButtons[2] & 0x80)
		raw->data.mouse.usButtonFlags |= RI_MOUSE_MIDDLE_BUTTON_UP;

	if(mouse_state.rgbButtons[3] & 0x80 && !(mouse_state_prev.rgbButtons[3] & 0x80))
		raw->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_4_DOWN;

	if(!(mouse_state.rgbButtons[3] & 0x80) && mouse_state_prev.rgbButtons[3] & 0x80)
		raw->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_4_UP;

	if(mouse_state.rgbButtons[4] & 0x80 && !(mouse_state_prev.rgbButtons[4] & 0x80))
		raw->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_5_DOWN;

	if(!(mouse_state.rgbButtons[4] & 0x80) && mouse_state_prev.rgbButtons[4] & 0x80)
		raw->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_5_UP;

	memcpy(&mouse_state_prev, &mouse_state, sizeof(DIMOUSESTATE2));

	return raw;
}

//Mouse and Keyboard seperate just to simplify things a bit
DWORD WINAPI PollDirectInputMouse(PVOID data) {
	RAWINPUT *raw;
	for(;;) {
		if(WaitForSingleObject(mouseInputEvent, 1000*2) != WAIT_ABANDONED) {
			raw = GetDirectInputMouseData();
			if(raw) AddRawInputToWindowList(raw, rawMouseInputWindowList);
		}
	}
	return 0;
}

VOID DeRegisterDirectInputMouse() {
	m_mouse->lpVtbl->Unacquire(m_mouse);
	m_mouse->lpVtbl->SetEventNotification(m_mouse, NULL);
}

BOOL RegisterDirectInputMouse(PCRAWINPUTDEVICE pRawInputDevices, HWND hWnd) {
	HRESULT hr;
	DWORD flags;
	
	//Try to map these flags to DirectX
	flags = 0;
	if(pRawInputDevices->dwFlags & RIDEV_INPUTSINK) flags |= DISCL_BACKGROUND;
	else                                            flags |= DISCL_FOREGROUND;
	flags |= DISCL_NONEXCLUSIVE;
		
	GetLock(rawMouseInputWindowList);
	//Init DirectInput if nessecary
	if (lpdi == NULL && FAILED(dlDirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8, (void**)&lpdi, NULL))) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to create DirectInput interface\n");
		ReleaseLock(rawMouseInputWindowList);
		return FALSE;
	}
	
	//Init MouseDevice if nessecary
	if (m_mouse == NULL && FAILED(lpdi->lpVtbl->CreateDevice(lpdi, &GUID_SysMouse, &m_mouse, NULL))) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to create DirectInput mouse device\n");
		ReleaseLock(rawMouseInputWindowList);
		return FALSE;
	}
	ReleaseLock(rawMouseInputWindowList);
	
	//Make friendly with this hWnd
	if (FAILED(m_mouse->lpVtbl->SetCooperativeLevel(m_mouse, hWnd, flags))) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to set cooperative level on DirectInput mouse device\n");
		return FALSE;
	}
	
	if (FAILED(m_mouse->lpVtbl->SetDataFormat(m_mouse, &c_dfDIMouse2))) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to set data format on DirectInput mouse device\n");
		return FALSE;
	}

	GetLock(rawMouseInputWindowList);
	if(!mouseInputEvent) {
		mouseInputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(!mouseInputEvent) {
			//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to create event for DirectInput mouse\n");
			ReleaseLock(rawMouseInputWindowList);
			return FALSE;
		}
		
		if(!CreateThread(NULL, 0, PollDirectInputMouse, NULL, 0, NULL)) {
			//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to create polling thread for DirectInput mouse\n");
			ReleaseLock(rawMouseInputWindowList);
			return FALSE;
		}
	}
	
	//Must be NOT be acquired to SetEventNotification
	m_mouse->lpVtbl->Unacquire(m_mouse);
	if(FAILED(m_mouse->lpVtbl->SetEventNotification(m_mouse, mouseInputEvent))) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to SetEventNotification for DirectInput mouse\n");
		ReleaseLock(rawMouseInputWindowList);
		return FALSE;		
	}
	
	ReleaseLock(rawMouseInputWindowList);
	return TRUE;
}

//SetWindowsHookEx WH_MOUSE_LL handler
LRESULT CALLBACK myLowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    static ULONG ulRawButtons = 0;
    RAWMOUSE *rm = NULL;
    RAWINPUT *ri = NULL;
    MSLLHOOKSTRUCT * hs = (MSLLHOOKSTRUCT*)lParam;
	   
    if(nCode == HC_ACTION) { //Should always be true
        ri = (RAWINPUT*)HeapAlloc(hHeap, 0, sizeof(RAWINPUT));
        if(ri) {
			SetRawInputMouseHeader(ri);
			
            rm = &ri->data.mouse;
            if(mouseFirstRun) { //first time around give the absolute position
                rm->usFlags = MOUSE_MOVE_ABSOLUTE;
                rm->lLastX = hs->pt.x;
                rm->lLastY = hs->pt.y;
            } else {
                rm->usFlags = MOUSE_MOVE_RELATIVE;
                if(hs->flags & LLMHF_INJECTED) {
                    rm->lLastX = 0;
                    rm->lLastY = 0;
                } else {
                    rm->lLastX = (LONG)hs->pt.x - mouseHookLastX;
                    rm->lLastY = (LONG)hs->pt.y - mouseHookLastY;
                }
            }
            
            mouseHookLastX = hs->pt.x;
            mouseHookLastY = hs->pt.y;	
            
            rm->ulButtons = 0;
            rm->usButtonData = 0;
            rm->usButtonFlags = 0;
            rm->ulExtraInformation = 0;
        
			//note ulRawButtons is static and we keep track of the state of the buttons this way
            switch(wParam)
            {
                case WM_LBUTTONDOWN:
                    rm->usButtonFlags |= RI_MOUSE_LEFT_BUTTON_DOWN;
                    ulRawButtons |= 0x00000001;
                    break;
        
                case WM_LBUTTONUP:
                    rm->usButtonFlags |= RI_MOUSE_LEFT_BUTTON_UP;
                    ulRawButtons &= ~0x00000001;
                    break;                
                    
                case WM_RBUTTONDOWN:
                    rm->usButtonFlags |= RI_MOUSE_RIGHT_BUTTON_DOWN;
                    ulRawButtons |= 0x00000002;
                    break;             
                    
                case WM_RBUTTONUP:
                    rm->usButtonFlags |= RI_MOUSE_RIGHT_BUTTON_UP;
                    ulRawButtons &= ~0x00000002;
                    break;                

                case WM_MBUTTONDOWN:
                    rm->usButtonFlags |= RI_MOUSE_MIDDLE_BUTTON_DOWN;
                    ulRawButtons |= 0x00000004;
                    break;             
                    
                case WM_MBUTTONUP:
                    rm->usButtonFlags |= RI_MOUSE_MIDDLE_BUTTON_UP;
                    ulRawButtons &= ~0x00000004;
                    break; 
                
                case WM_MOUSEWHEEL:
                    rm->usButtonFlags |= RI_MOUSE_WHEEL;
                    rm->usButtonData = HIWORD(hs->mouseData);
                    break;
					
				case WM_XBUTTONDOWN:
					if(HIWORD(hs->mouseData) == XBUTTON1)      rm->usButtonFlags |= RI_MOUSE_BUTTON_4_DOWN;
					else if(HIWORD(hs->mouseData) == XBUTTON2) rm->usButtonFlags |= RI_MOUSE_BUTTON_5_DOWN;
					ulRawButtons |= (1 << (HIWORD(hs->mouseData) + 2));
					break;
					
				case WM_XBUTTONUP:
					if(HIWORD(hs->mouseData) == XBUTTON1)      rm->usButtonFlags |= RI_MOUSE_BUTTON_4_UP;
					else if(HIWORD(hs->mouseData) == XBUTTON2) rm->usButtonFlags |= RI_MOUSE_BUTTON_5_UP;
					ulRawButtons &= ~(1 << (HIWORD(hs->mouseData) + 2));
					break;
            }
            rm->ulRawButtons = ulRawButtons;
                                    
            if(!mouseFirstRun) {
				AddRawInputToWindowList(ri, rawMouseInputWindowList);
            } else {
				mouseFirstRun = FALSE;
			}
        }
    }
    
    return CallNextHookEx(rawInputMouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK myLowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT * ks = (KBDLLHOOKSTRUCT*)lParam;    
    RAWKEYBOARD *rk=NULL;
    RAWINPUT *ri=NULL;
    
    if(nCode == HC_ACTION)
    {
        ri = (RAWINPUT*)HeapAlloc(hHeap, 0, sizeof(RAWINPUT));
        if(ri)
        {
            rk = &ri->data.keyboard;
            rk->MakeCode = ks->scanCode;
            rk->Flags = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ? RI_KEY_BREAK : RI_KEY_MAKE;
            rk->Reserved = 0;
            rk->VKey = ks->vkCode;
            rk->Message = wParam;
            rk->ExtraInformation = 0;
        
            ri->header.dwType = RIM_TYPEKEYBOARD;
            ri->header.dwSize = sizeof(RAWINPUT);
            ri->header.hDevice = KBD_DEVICE_HANDLE;
            ri->header.wParam = RIM_INPUT;
			
			AddRawInputToWindowList(ri, rawKeyboardInputWindowList);            
        }
    }
    
    return CallNextHookEx(rawInputKeyboardHook, nCode, wParam, lParam);
}

BOOL XP_RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) {
    int i;
	HWND hWnd;

    if(!pRawInputDevices || !uiNumDevices || !cbSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
	
    for(i=0; i<uiNumDevices; i++) {
		//Get the window handle if we need to
		hWnd = pRawInputDevices[i].hwndTarget;
		if(!hWnd) hWnd = GetFocus();
		if(!hWnd) hWnd = GetForegroundWindow(); //try this too
		if(!hWnd) {
			//if(DbgPrintf) DbgPrintf(DBG_WARN,  "user32: couldn't get HWND for RegisterRawInputDevices\n");
			return FALSE;
		}
	
        // Mouse input
        if(pRawInputDevices[i].usUsagePage==1 && pRawInputDevices[i].usUsage==2) { //Mouse should match this
			GetLock(rawMouseInputWindowList);
            if(!(pRawInputDevices[i].dwFlags & RIDEV_REMOVE)) {
				//Add			
				//if(DbgPrintf) DbgPrintf(DBG_TRACE, "user32: Adding a RAWINPUT mouse\n");
				
                if(pRawInputDevices[i].dwFlags & RIDEV_CAPTUREMOUSE)
                    SetCapture(hWnd);
				
				//Add the hWnd to the list
                if(!IsInListByVal(rawMouseInputWindowList, (DWORD)hWnd))
                    PushFrontByVal(rawMouseInputWindowList, (DWORD)hWnd);
                
				//If not getting low level data yet activate
				if(UseDirectInput) {
					RegisterDirectInputMouse(&pRawInputDevices[i], hWnd);
						//DbgPrintf(DBG_ERROR, S_RED "user32: Could not register DirectInput mouse\n");				
                } else if(!rawInputMouseHook) {
                    rawInputMouseHook = SetWindowsHookExA(WH_MOUSE_LL, myLowLevelMouseProc, GetModuleHandle("user32.dll"), 0);
					/*if(!rawInputMouseHook && DbgPrintf)
						DbgPrintf(DBG_ERROR, S_RED "user32: Could not SetWindowsHookEx WH_MOUSE_LL LastError=0x%X\n", GetLastError());*/
				}
				
				//Save this structure to return with GetRegisteredRawInputDevices
				b_registered_mouse = TRUE;
				memcpy(&rid_mouse, &pRawInputDevices[i], sizeof(RAWINPUTDEVICE));
            } else {
				//Remove
				//if(DbgPrintf) DbgPrintf(DBG_TRACE, "user32: Removing a RAWINPUT mouse\n");
				
                if(pRawInputDevices[i].hwndTarget) {
					//This actually is not allowed
                    RemoveNodeByVal(rawMouseInputWindowList, (DWORD)pRawInputDevices[i].hwndTarget);
                } else {
					//Remove dem all!
                    while(rawMouseInputWindowList->length)
                        PopFrontAndFree(rawMouseInputWindowList);
                }
                
                ReleaseCapture();
                
				if(rawMouseInputWindowList->length==0) {
					if(UseDirectInput) {
						DeRegisterDirectInputMouse();
					} else if(rawInputMouseHook) {
						UnhookWindowsHookEx(rawInputMouseHook);
						rawInputMouseHook = NULL;
					}
				}
				
				b_registered_mouse = FALSE;
            }
			ReleaseLock(rawMouseInputWindowList);
        }
          
        // Keyboard input
        if(pRawInputDevices[i].usUsagePage==1 && pRawInputDevices[i].usUsage==6) { //kbd should match this
			GetLock(rawKeyboardInputWindowList);
            if(!(pRawInputDevices[i].dwFlags & RIDEV_REMOVE)) {
				//Add			
				//if(DbgPrintf) DbgPrintf(DBG_TRACE, "user32: Adding a RAWINPUT keyboard\n");
				
                if(!IsInListByVal(rawKeyboardInputWindowList, (DWORD)hWnd))
                    PushFrontByVal(rawKeyboardInputWindowList, (DWORD)hWnd);
					
				/*if(UseDirectInput)
					if(DbgPrintf) DbgPrintf(DBG_WARN, S_YELLOW "user32: no DirectInput support for KBD yet, defaulting to Low Level Hook\n");*/
                
                if(!rawInputKeyboardHook)
                    rawInputKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, myLowLevelKeyboardProc, GetModuleHandle("user32.dll"), 0);
					
				//Save this structure to return with GetRegisteredRawInputDevices
				b_registered_keyboard = TRUE;
				memcpy(&rid_keyboard, &pRawInputDevices[i], sizeof(RAWINPUTDEVICE));
            } else {
				//Remove
				//if(DbgPrintf) DbgPrintf(DBG_TRACE, "user32: Removing a RAWINPUT keyboard\n");
				
                if(pRawInputDevices[i].hwndTarget) {
					//This actually is not allowed
                    RemoveNodeByVal(rawKeyboardInputWindowList, (DWORD)pRawInputDevices[i].hwndTarget);
                } else {
					//Remove dem all!
                    while(rawKeyboardInputWindowList->length)
                        PopFrontAndFree(rawKeyboardInputWindowList);
                }
                
                if(rawInputKeyboardHook && rawKeyboardInputWindowList->length==0) {
                    UnhookWindowsHookEx(rawInputKeyboardHook);
                    rawInputKeyboardHook = NULL;
                }
				
				b_registered_keyboard = FALSE;
            }
			ReleaseLock(rawKeyboardInputWindowList);
        }
    }
    
    return TRUE;
}









BOOL GetKeyboardDeviceInfo(KEYBOARD_ATTRIBUTES *attrib) {
	HANDLE hDevice;
	DWORD bytesReturned;

	//We really should have this synchronized somehow
	if(!DefineDosDevice(DDD_RAW_TARGET_PATH, INFO_DEVICE_NAME, "\\Device\\KeyboardClass0")) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to create Dos Device for kbd 0x%X\n", GetLastError());
		return FALSE;
	}
	
	hDevice = CreateFile("\\\\.\\" INFO_DEVICE_NAME,
		0,
		FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	
	if(hDevice == INVALID_HANDLE_VALUE) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to open Dos Device for kbd 0x%X\n", GetLastError());
		DefineDosDevice(DDD_REMOVE_DEFINITION, INFO_DEVICE_NAME, NULL);
		return FALSE;
	}
	
	if(!DeviceIoControl(
		hDevice,
		IOCTL_KEYBOARD_QUERY_ATTRIBUTES,
		NULL, 0,
		attrib, sizeof(KEYBOARD_ATTRIBUTES),
		&bytesReturned,
		NULL)) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: DevIOCtl failed to query the kbd 0x%X\n", GetLastError());
		CloseHandle(hDevice);
		DefineDosDevice(DDD_REMOVE_DEFINITION, INFO_DEVICE_NAME, NULL);
		return FALSE;
	}
	
	CloseHandle(hDevice);
	DefineDosDevice(DDD_REMOVE_DEFINITION, INFO_DEVICE_NAME, NULL);
	return TRUE;
}

BOOL GetMouseDeviceInfo(MOUSE_ATTRIBUTES *attrib) {
	HANDLE hDevice;
	DWORD bytesReturned;

	//We really should have this synchronized somehow
	if(!DefineDosDevice(DDD_RAW_TARGET_PATH, INFO_DEVICE_NAME, "\\Device\\PointerClass0")) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to create Dos Device for mouse 0x%X\n", GetLastError());
		return FALSE;
	}
	
	hDevice = CreateFile("\\\\.\\" INFO_DEVICE_NAME,
		0,
		FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	
	if(hDevice == INVALID_HANDLE_VALUE) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: Failed to open Dos Device for mouse 0x%X\n", GetLastError());
		DefineDosDevice(DDD_REMOVE_DEFINITION, INFO_DEVICE_NAME, NULL);
		return FALSE;
	}
	
	if(!DeviceIoControl(
		hDevice,
		IOCTL_MOUSE_QUERY_ATTRIBUTES,
		NULL, 0,
		attrib, sizeof(MOUSE_ATTRIBUTES),
		&bytesReturned,
		NULL)) {
		//if(DbgPrintf) DbgPrintf(DBG_ERROR, S_RED "user32: DevIOCtl failed to query the mouse 0x%X\n", GetLastError());
		CloseHandle(hDevice);
		DefineDosDevice(DDD_REMOVE_DEFINITION, INFO_DEVICE_NAME, NULL);
		return FALSE;
	}
	
	CloseHandle(hDevice);
	DefineDosDevice(DDD_REMOVE_DEFINITION, INFO_DEVICE_NAME, NULL);
	return TRUE;
}

UINT XP_GetRawInputDeviceInfoA(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize) {
	UINT size_needed;
	UINT len;
	RID_DEVICE_INFO *info;
	MOUSE_ATTRIBUTES mattrib;
	KEYBOARD_ATTRIBUTES kattrib;
	UINT size_wrote = 0;
	
	if(uiCommand == RIDI_PREPARSEDDATA) {
		//if(DbgPrintf) DbgPrintf(DBG_WARN, S_YELLOW "user32: GetRawInputDeviceInfo - Don't know what to do with RIDI_PREPARSEDDATA\n");
		return 0;
	}
	
	switch(uiCommand) {
		case RIDI_DEVICENAME:
			size_needed = MAX_DEVICE_NAME;
			break;
		case RIDI_DEVICEINFO:
			size_needed = sizeof(RID_DEVICE_INFO);
			break;
		default:
			//if(DbgPrintf) DbgPrintf(DBG_WARN, S_YELLOW "user32: GetRawInputDeviceInfo - Unknown uiCommand %X\n", uiCommand);
			return 0;
	}
	
	if(!pData) {
		*pcbSize = size_needed;
		return 0;
	}
	
	if(*pcbSize < size_needed) {
		*pcbSize = size_needed;
		return -1;
	}
	
	switch(uiCommand) {
		case RIDI_DEVICENAME:
			switch((DWORD)hDevice) {
				case MOUSE_DEVICE_HANDLE:
					len = strlen("Wrapper Mouse");
					RtlCopyMemory(pData,  "Wrapper Mouse", len*sizeof(CHAR));
					size_wrote = len * sizeof(CHAR);
					break;
				case KBD_DEVICE_HANDLE:
					len = strlen("Wrapper Keyboard");
					RtlCopyMemory(pData,  "Wrapper Keyboard", len*sizeof(CHAR));
					size_wrote = len * sizeof(CHAR);
					break;
				default:
					//if(DbgPrintf) DbgPrintf(DBG_WARN, S_YELLOW "user32: GetRawInputDeviceInfo - Unknown device %X\n", hDevice);
					return 0;
			}
			break;
		case RIDI_DEVICEINFO:
			info = (RID_DEVICE_INFO *)pData;
			info->cbSize = sizeof(RID_DEVICE_INFO);
			size_wrote = sizeof(RID_DEVICE_INFO);
			switch((DWORD)hDevice) {
				case MOUSE_DEVICE_HANDLE:
					info->dwType = RIM_TYPEMOUSE;
					if(GetMouseDeviceInfo(&mattrib)) {
						info->mouse.dwId = mattrib.MouseIdentifier;
						info->mouse.dwNumberOfButtons = mattrib.NumberOfButtons;
						info->mouse.dwSampleRate = mattrib.SampleRate;
						//info->mouse.fHasHorizontalWheel = FALSE;
					}
					break;
				case KBD_DEVICE_HANDLE:
					info->dwType = RIM_TYPEKEYBOARD;
					if(GetKeyboardDeviceInfo(&kattrib)) {
						info->keyboard.dwType    = kattrib.KeyboardIdentifier.Type;
						info->keyboard.dwSubType = kattrib.KeyboardIdentifier.Subtype;
						info->keyboard.dwKeyboardMode = kattrib.KeyboardMode;
						info->keyboard.dwNumberOfFunctionKeys = kattrib.NumberOfFunctionKeys;
						info->keyboard.dwNumberOfIndicators = kattrib.NumberOfIndicators;
						info->keyboard.dwNumberOfKeysTotal = kattrib.NumberOfKeysTotal;
					}
					break;
				default:
					//if(DbgPrintf) DbgPrintf(DBG_WARN, S_YELLOW "user32: GetRawInputDeviceInfo - Unknown device %X\n", hDevice);
					return 0;
			}
			break;
	}
	
	return size_wrote;
}

UINT XP_GetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize) {
	PRAWINPUTDEVICELIST d;
	
	if(!pRawInputDeviceList) {
		*puiNumDevices = 2;
		return 0;
	}
	
	if(*puiNumDevices < 2) {
		*puiNumDevices = 2;
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return (UINT)-1;
	}
	
	d = pRawInputDeviceList;
	d->hDevice = MOUSE_DEVICE_HANDLE;
	d->dwType  = RIM_TYPEMOUSE;
	
	d = (PRAWINPUTDEVICELIST)(((PBYTE)d) + cbSize);
	d->hDevice = KBD_DEVICE_HANDLE;
	d->dwType  = RIM_TYPEKEYBOARD;
	
	*puiNumDevices = 2;
	return 2;
}
