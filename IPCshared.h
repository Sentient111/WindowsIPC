#pragma once
#include <Windows.h>
#include <iostream>

#define IS_INVALID_HANDLE(handle) !handle || handle == INVALID_HANDLE_VALUE

#define PACKET_TYPE_PID 1

#define MUTEX_WAIT_TIMEOUT 1000

struct SharedPacket
{
	int type;
	DWORD size;
	BYTE data;
};

struct SharedSection
{
	UINT64 updateHandler;
	int ownerPid;
	DWORD newPacketsCount;
	DWORD freePacketSize;
	SharedPacket* packets;
};
