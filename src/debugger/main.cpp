#include "Debugger.hpp"

#include <windows.h>

#include <thread>
#include <chrono>
#include <d3d9.h>

HMODULE g_Handle;

namespace Debugger
{
    struct FakeWindow
    {
        FakeWindow( HMODULE owner )
        {
            handle = CreateWindowA( "BUTTON", "Temp Window", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, owner, NULL );
        }

        ~FakeWindow()
        {
            if ( handle )
            {
                DestroyWindow( handle );
            }
        }

        HWND handle;
    };

    DWORD WINAPI InitializeHook( __in  LPVOID lpParameter )
    {
        while ( GetModuleHandle( L"d3d9.dll" ) == 0 )
        {
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }

        FakeWindow window( g_Handle );

        IDirect3D9* directx = Direct3DCreate9( D3D_SDK_VERSION );
        if ( directx == nullptr )
            return 0;

        D3DPRESENT_PARAMETERS params;
        ZeroMemory( &params, sizeof( params ) );
        params.Windowed = TRUE;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.hDeviceWindow = window.handle;
        params.BackBufferFormat = D3DFMT_UNKNOWN;

        IDirect3DDevice9* device = nullptr;

        HRESULT result = directx->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &device );
        if ( result != D3D_OK )
        {
            directx->Release();
            return 0;
        }

        VTABLE_TYPE* vtable = reinterpret_cast< VTABLE_TYPE* >( device );
        vtable = reinterpret_cast< VTABLE_TYPE* >( vtable[ 0 ] );

        static Debugger s_debugger( vtable );

        device->Release();
        directx->Release();
        return 1;
    }
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD action, LPVOID /*reserved*/ )
{
    switch ( action )
    {
        case DLL_PROCESS_ATTACH:
        {
            g_Handle = hModule;

            DisableThreadLibraryCalls( g_Handle );
            CreateThread( 0, 0, Debugger::InitializeHook, 0, 0, 0 );
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
