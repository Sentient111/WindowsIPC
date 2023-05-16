#pragma once
#include "IPCshared.h"

SharedSection* sharedSec = nullptr;
HANDLE clientFilemappingHandle = INVALID_HANDLE_VALUE;

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

	if (WaitForSingleObject(callThread, MUTEX_WAIT_TIMEOUT / 2))
	{
		printf("Failed to execute routine %X\n", GetLastError());
		CloseHandle(procHandle);
		TerminateThread(callThread, 0);
		return false;
	}

	CloseHandle(procHandle);
	CloseHandle(callThread);
	return true;
}


bool AddPacketsToRemoteList(PVOID packets, ULONG packetCount, ULONG size)
{
	if (!RemoteCall(sharedSec->lockHandler))
	{
		printf("Failed to aquire lock for page\n");
		return false;
	}

	sharedSec->newPacketsCount = packetCount;
	memcpy(&sharedSec->packets, packets, size);
	RemoteCall(sharedSec->updateHandler);

	sharedSec->newPacketsCount = 0;
	memset(&sharedSec->packets, 0, size);

	if (!RemoteCall(sharedSec->unlockHandler))
	{
		printf("Failed to release lock for page\n");
		return false;
	}
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

	//write to shared mem
	MEMORY_BASIC_INFORMATION memInfo = { 0 };
	VirtualQuery(sharedSec, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

	return true;
}

void CloseClientIPC()
{
	UnmapViewOfFile(sharedSec);
	CloseHandle(clientFilemappingHandle);
}