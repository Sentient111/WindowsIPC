#pragma once
#include "IPCshared.h"
#include <vector>

std::vector<int> hiddenPidList;

//This could be a class too but I prefer data oriented programming over oop
HANDLE filemappingHandle = INVALID_HANDLE_VALUE;
HANDLE mutexHandle = INVALID_HANDLE_VALUE;
SharedSection* mappedViewOfFile = nullptr;


void AquirePageLockRoutine()
{
	if (WaitForSingleObject(mutexHandle, MUTEX_WAIT_TIMEOUT))
	{
		printf("Failed to aquire mutex %X\n", GetLastError());
		return;
	}
}

void ReleasePageLockRoutine()
{
	ReleaseMutex(mutexHandle);
}


void UpdateListsRoutine()
{
	if (0 > mappedViewOfFile->newPacketsCount)
	{
		printf("No new packets\n");
		return;
	}

	SharedPacket* currPacket = (SharedPacket*)&mappedViewOfFile->packets;
	ULONG readPacketSize = 0;

	for (size_t i = 0; i < mappedViewOfFile->newPacketsCount; i++)
	{
		switch (currPacket->type)
		{
		case PACKET_TYPE_PID:
		{
			printf("Recieved new pid to blacklist %X\n", *(int*)&currPacket->data);
			hiddenPidList.push_back(*(int*)&currPacket->data);
			break;
		}

		default:
		{
			printf("Invalid packet type at idx %i\n", i);
			break;
		}
		}

		readPacketSize += currPacket->size;
		currPacket = (SharedPacket*)(((BYTE*)currPacket) + currPacket->size);
	}
}

bool InitHostIPC()
{
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	if (0x1000 > sysInfo.dwPageSize)
	{
		printf("Invalid system page size %X\n", sysInfo.dwPageSize);
		return false;
	}

	mutexHandle = CreateMutexA(NULL, NULL, "Fuckyou");
	if (IS_INVALID_HANDLE(mutexHandle))
	{
		printf("Failed to create mutex %X\n", GetLastError());
		return false;
	}

	filemappingHandle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, NULL, sysInfo.dwPageSize, "BigManBill");
	if (IS_INVALID_HANDLE(filemappingHandle))
	{
		printf("Failed to create filemapping %X\n", GetLastError());
		CloseHandle(mutexHandle);
		return false;
	}

	mappedViewOfFile = (SharedSection*)MapViewOfFile(filemappingHandle, FILE_MAP_ALL_ACCESS, NULL, NULL, sysInfo.dwPageSize);
	if (!mappedViewOfFile)
	{
		printf("Failed to map view of file %X\n", GetLastError());
		CloseHandle(mutexHandle);
		CloseHandle(filemappingHandle);
		return false;
	}

	//set handlers
	mappedViewOfFile->lockHandler = (UINT64)AquirePageLockRoutine;
	mappedViewOfFile->unlockHandler = (UINT64)ReleasePageLockRoutine;
	mappedViewOfFile->updateHandler = (UINT64)UpdateListsRoutine;
	mappedViewOfFile->ownerPid = GetCurrentProcessId();
	mappedViewOfFile->newPacketsCount = 0;

	return true;
}


void DestroyHostIPC()
{
	CloseHandle(mutexHandle);
	UnmapViewOfFile(mappedViewOfFile);
	CloseHandle(filemappingHandle);
}