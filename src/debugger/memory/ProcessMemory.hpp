#ifndef ProcessMemory_hpp__
#define ProcessMemory_hpp__

#include <windows.h>
#include <stdexcept>

namespace Debugger
{
    class ProcessMemory
    {
    public:
        ProcessMemory( HANDLE hProcess )
            : m_hProcess( hProcess )
        {

        }

        template< typename T >
        T Read( uintptr_t offset )
        {
            T buffer;

            ReadInto( buffer, offset );

            return buffer;
        }

        template< typename T >
        void ReadInto( T & buffer, uintptr_t offset )
        {
            SIZE_T bytesRead = 0;
            BOOL status = ReadProcessMemory( m_hProcess, reinterpret_cast< const void * >( offset ), &buffer, sizeof( T ), &bytesRead );
            if ( bytesRead != sizeof( T ) || status == FALSE )
            {
                throw std::runtime_error( "Failed to read memory!" );
            }
        }

        uintptr_t GetFunctionFromVtable( uintptr_t offset, size_t functionIndex )
        {
            return Read<uintptr_t>( Read<uintptr_t>( offset ) + functionIndex * 4 );
        }

    private:
        HANDLE m_hProcess;
    };
}

#endif // ProcessMemory_hpp__
