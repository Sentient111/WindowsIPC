#pragma once
#include "IPCshared.h"

SharedSection* sharedSec = nullptr;
HANDLE clientFilemappingHandle = INVALID_HANDLE_VALUE;
HANDLE sharedPageMutex = INVALID_HANDLE_VALUE;

bool RemoteCall(UINT64 routine)
{
	HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, false, sharedSec->ownerPid);
	if (IS_INVALID_HANDLE(procHandle))
	{
		printf("Failed to open handle to shared page owner process %X\n", GetLastError());
		return false;
	}

	HANDLE callThread = CreateRemoteThread(procHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)routine, NULL, NULL, NULL);

	if (!callThread)
	{
		printf("Failed to create remote thread %X\n", GetLastError());
		return false;
	}

	WaitForSingleObject(callThread, 1000);

	CloseHandle(procHandle);
	CloseHandle(callThread);
	return true;
}


bool AddPacketsToRemoteList(PVOID packets, ULONG packetCount, ULONG size)
{
	if (WaitForSingleObject(sharedPageMutex, MUTEX_WAIT_TIMEOUT))
	{
		printf("Failed to obtain shared page mutex %X\n", GetLastError());
		return false;
	}

	sharedSec->newPacketsCount = packetCount;
	memcpy(&sharedSec->packets, packets, size);
	RemoteCall(sharedSec->updateHandler);

	sharedSec->newPacketsCount = 0;
	memset(&sharedSec->packets, 0, size);

	ReleaseMutex(sharedPageMutex);
}

bool OpenClientIpc()
{
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	if (0x1000 > sysInfo.dwPageSize)
	{
		printf("Invalid system page size %X\n", sysInfo.dwPageSize);
		return false;
	}

	clientFilemappingHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, "BigManBill");
	if (IS_INVALID_HANDLE(clientFilemappingHandle))
	{
		printf("Failed to open handle to filemapping %X\n", GetLastError());
		return false;
	}

	sharedSec = (SharedSection*)MapViewOfFile(clientFilemappingHandle, FILE_MAP_ALL_ACCESS, NULL, NULL, sysInfo.dwPageSize);
	if (!sharedSec)
	{
		printf("Failed to map view of file %X\n", GetLastError());
		return false;
	}

	sharedPageMutex = OpenMutexA(SYNCHRONIZE, false, "BoberBoBBIBO");
	if (!sharedPageMutex)
	{
		printf("Failed to open mutex %X\n", GetLastError());
		return false;
	}

	return true;
}

void CloseClientIPC()
{
	UnmapViewOfFile(sharedSec);
	CloseHandle(clientFilemappingHandle);
}
