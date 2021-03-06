#define WIN32_LEAN_AND_MEAN 
#include <windows.h> 

#include <tlhelp32.h> 
#include <shlwapi.h> 
#include <conio.h> 
#include <stdio.h> 
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#define CREATE_THREAD_ACCESS (PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ) 

bool InjectIntoProcess( DWORD processId, const char * dllName )
{
    HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, processId );
    if ( !hProcess )
        return false;

    LPVOID procedure = ( LPVOID )GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" );
    LPVOID procedureArgs = ( LPVOID )VirtualAllocEx( hProcess, NULL, strlen( dllName ), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

    WriteProcessMemory( hProcess, procedureArgs, dllName, strlen( dllName ), NULL );
    CreateRemoteThread( hProcess, NULL, NULL, ( LPTHREAD_START_ROUTINE )procedure, procedureArgs, NULL, NULL );

    CloseHandle( hProcess );
    return true;
}

DWORD GetProcessIdByName( const char * processName )
{
    HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( snapshot == INVALID_HANDLE_VALUE )
        return 0;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof( PROCESSENTRY32 );

    BOOL status = Process32First( snapshot, &entry );
    while ( status == TRUE )
    {
        if ( StrStrI( entry.szExeFile, processName ) )
            return entry.th32ProcessID;

        status = Process32Next( snapshot, &entry );
    }

    return 0;
}

std::string GetFullLibraryPath( const char * dllName )
{
    auto length = GetFullPathName( dllName, 0, nullptr, nullptr );

    std::string fullPath;
    fullPath.resize( length );

    GetFullPathName( dllName, length, const_cast< char * >( fullPath.c_str() ), nullptr );

    return fullPath;
}

int main( int argc, char * argv[] )
{
    if ( argc != 3 )
    {
        std::cout << "Process requires 2 arguments: \n";
        std::cout << "* process name that will have DLL injected\n";
        std::cout << "* path to DLL that will be injected into process\n";
        std::cout << "\n\nExample: Injector.exe Wow.exe Debugger.dll\n\n";

        system( "pause" );
        return -1;
    }

    const char* PROCESS_NAME = argv[ 1 ];
    const char* LIBRARY_NAME = argv[ 2 ];

    DWORD processId = GetProcessIdByName( PROCESS_NAME );
    while ( processId == 0 )
    {
        std::cout << "Process: " << PROCESS_NAME << ", not found. Retrying in 1s.\n";
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    }

    std::string fullPath = GetFullLibraryPath( LIBRARY_NAME );
    if ( !InjectIntoProcess( processId, fullPath.c_str() ) )
    {
        std::cout << "[ERROR] Failed to inject DLL: " << fullPath << " into: " << PROCESS_NAME << "\n";
        system( "pause" );
        return -1;
    }

    std::cout << "DLL: " << fullPath << " properly injected into: " << PROCESS_NAME << "\n";
    return 0;
}
