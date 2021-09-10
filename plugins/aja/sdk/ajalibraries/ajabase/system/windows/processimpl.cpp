/* SPDX-License-Identifier: MIT */
/**
	@file		windows/processimpl.cpp
	@brief		Implements the AJAProcessImpl class on the Windows platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/windows/processimpl.h"
#include "ajabase/common/timer.h"


// class AJAProcessImpl

AJAProcessImpl::AJAProcessImpl()
{
}


AJAProcessImpl::~AJAProcessImpl()
{
}

uint64_t
AJAProcessImpl::GetPid()
{
	return GetCurrentProcessId();
}

bool
AJAProcessImpl::IsValid(uint64_t pid)
{
	HANDLE pH = OpenProcess(SYNCHRONIZE, false, (DWORD)pid);
	if(INVALID_HANDLE_VALUE != pH && NULL != pH)
	{
		DWORD retValue = WaitForSingleObject(pH, 0);
		CloseHandle(pH);
		return retValue == WAIT_TIMEOUT;
	}
	return false;
}

static HWND hFoundWindow = NULL;

BOOL CALLBACK findFunc(HWND hWnd, LPARAM lParam)
{
	bool bDone = false;
	TCHAR* pTail = (TCHAR*)lParam;
	TCHAR Title[500];
	GetWindowText(hWnd, Title, NUMELMS(Title));

	if(strlen(Title) < strlen(pTail))
		return TRUE;
	TCHAR* pTitle = Title + strlen(Title) - strlen(pTail);
	if(pTitle < Title)
		return TRUE;		// Don't do the compare, because it would overflow
	int32_t len = min((int32_t)strlen(pTitle), (int32_t)strlen(pTail));
	if(!_strnicmp(pTitle, pTail, len))
	{
		hFoundWindow = hWnd;
		return FALSE;		// Found it, we're done
	}
	return TRUE;			// Keep going
}


bool
AJAProcessImpl::Activate(const char* pWindow)
{
	// handle "ends in" by looking for * at beginning of string
	const char* pName = pWindow;
	bool bEnum = false;
	if(*pName == '*')
	{
		pName++;
		bEnum = true;
	}
	HWND hWindow = NULL;
	bool bFound = false;
	if(!bEnum)
	{
		hWindow = FindWindow(NULL, pWindow);
		bFound = hWindow ? true : false;
	}
	else
	{
		hFoundWindow = NULL;
		bFound = !EnumWindows(findFunc, (LPARAM)pName);
		hWindow = hFoundWindow;
	}
	if(!bFound)
		return false;

	return Activate((uint64_t)hWindow);
}

bool
AJAProcessImpl::Activate(uint64_t handle)
{
	HWND hWnd = (HWND)handle;
	if(!hWnd)
		return false;
	if(!IsWindow(hWnd))
		return false;

	DWORD pid = 0;
	DWORD oldThread = GetWindowThreadProcessId(hWnd, &pid);
	if(!oldThread)
		return false;

	DWORD currentThread = GetCurrentThreadId();

	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd, &wp);

	if(SW_MINIMIZE == wp.showCmd || SW_SHOWMINIMIZED == wp.showCmd)
		ShowWindow(hWnd, SW_RESTORE);
	else
	{
		AttachThreadInput(oldThread, currentThread, TRUE);
		SetActiveWindow(hWnd);
		SetForegroundWindow(hWnd);
		BringWindowToTop(hWnd);
		SetFocus(hWnd);
		AttachThreadInput(oldThread, currentThread, FALSE);
	}
	RECT r;
	GetClientRect(hWnd, &r);
	InvalidateRect(hWnd, &r, true);
	UpdateWindow(hWnd);
	return true;
}
