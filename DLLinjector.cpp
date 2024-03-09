#include <Windows.h>
#include <iostream>
#include <string>
#include <psapi.h>
#include <VersionHelpers.h>
#include <atlstr.h>
#include <tlhelp32.h>

#define CREATE_THREAD_ACCESS (PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ)

//---- EDIT THIS --------------------------------------------------------------------------------------------
//Change this to the process you want to inject the DLL into
std::string DLL_PATH = "C:\\MyDLL.dll";

//Change this to the process you want to inject the DLL into
std::string targetProcessName = "somethingsomething.exe";

//-----------------------------------------------------------------------------------------------------------

BOOL InjectDLL(DWORD processID, const char* dllPATH)
{
    HANDLE processHND = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);

    //Calcuates the length of the dll path string
    SIZE_T dllLEN = strlen(dllPATH) + 1;

    //Allocates memory of dllLEN in the target process and gets the pointer to it
    LPVOID p_dllPATH = VirtualAllocEx(processHND, 0, dllLEN, MEM_COMMIT, PAGE_READWRITE);

    // Writes the path in the memory just allocated
    WriteProcessMemory(processHND, p_dllPATH, (LPVOID)dllPATH, dllLEN, 0);

    // Creates a thread within the target process that calls LoadLibraryA() with argument a pointer to the string containing the dll path
    // effectively loading the DLL in the target process
    HANDLE threadHND = CreateRemoteThread(processHND, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), p_dllPATH, 0, 0);

    // Waits for the thread to finish
    WaitForSingleObject(threadHND, INFINITE);

    std::cout << "[Debug]: DLL path allocated at: " << std::hex << p_dllPATH << std::endl;

    // Free the memory allocated for our dll path
    VirtualFreeEx(processHND, p_dllPATH, dllLEN, MEM_RELEASE);

    return 0;
}

int main() {

    //The target process ID
    DWORD targetPID;

    //Creates a handle to a snapshot with the list of the running processes
    HANDLE snapshotHND = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    //Entry to check, we use this to scroll through the list of processes
    PROCESSENTRY32W entry;
    
    entry.dwSize = sizeof entry;

    //Sets entry to the first process in the list
    if (!Process32FirstW(snapshotHND, &entry))
    {
        return 0; //if this fails we exit
    }

    do {
        //Get the name of the process set in entry and convert it to a string
        std::wstring wsEXE(entry.szExeFile);
        std::string WSEXEconverted(wsEXE.begin(), wsEXE.end());

        //Checks if the process name in entry matches against the process we are interested in 
        if (WSEXEconverted == targetProcessName) {

            std::cout << "[Debug]: (" << WSEXEconverted << ") -- matched -> (" << targetProcessName << ")" << std::endl;
            
            //Sets the target process ID to the process ID of the entry that matched
            targetPID = entry.th32ProcessID;
           
            std::cout << "[Debug]: PID: " << targetPID << std::endl;

            //Checks OS compatibility
            if (IsWindowsXPOrGreater()) {

                //Injects the DLL
                InjectDLL(targetPID, DLL_PATH.c_str());
            }
            else {
                std::cout << "[Error]: OS not supported" << std::endl;
                return 0;
            }
            break;
        }

    //Scrolls to the next process and sets entry to that
    } while (Process32NextW(snapshotHND, &entry));

    return 0;
}
