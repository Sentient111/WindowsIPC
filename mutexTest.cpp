//process 1

#include "IPChost.h"

void ProcessExitRoutine()
{
	DestroyHostIPC();
}

int main()
{
	std::atexit(ProcessExitRoutine);

	InitHostIPC();
	//free to do other stuff
	system("pause");
}


//process 2

#include "IPCclient.h"

SharedPacket* CreatePidPacket(int pid)
{
	ULONG neededSize = sizeof(SharedPacket) - 1 + sizeof(pid);
	SharedPacket* packet = (SharedPacket*)VirtualAlloc(NULL, neededSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!packet)
	{
		printf("Failed to allocate %i bytes for pid packet %X\n", sizeof(SharedPacket) - 1 + sizeof(pid), GetLastError());
		return 0;
	}

	memcpy(&packet->data, &pid, sizeof(pid));

	packet->size = neededSize;
	packet->type = PACKET_TYPE_PID;
	return packet;
}


int main()
{
	OpenClientIpc();

	SharedPacket* packet = CreatePidPacket(0x1337);
	AddPacketsToRemoteList(packet, 1, packet->size);
	VirtualFree(packet, NULL, MEM_RELEASE);

	CloseClientIPC();
}
