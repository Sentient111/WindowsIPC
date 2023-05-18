# WindowsIPC
Windows IPC using remote threads and filemappings

Simple inter process communication project for Windows. Uses a filemapping to share data and a mutex to lock access to the page so it's threadsave.
To avoid a while true thread in the host process the start of the filemapping is used to give information about the host process and the address of a update routine.
A client process can then simply create a remote thread in the host process with the update routine as start address to notify the host to update its data.
