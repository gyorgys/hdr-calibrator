#include <windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <dinput.h>
#include <string>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitD3D(HWND hwnd);
bool InitHDR();
bool InitDirectInput(HWND hwnd);
void CleanupD3D();
void CleanupDirectInput();
void Render();
void UpdateInput();

// Global variables
HWND g_hwnd = nullptr;
ComPtr<ID3D11Device> g_device;
ComPtr<ID3D11DeviceContext> g_context;
ComPtr<IDXGISwapChain1> g_swapChain;
ComPtr<ID3D11RenderTargetView> g_renderTargetView;
ComPtr<ID3D11VertexShader> g_vertexShader;
ComPtr<ID3D11PixelShader> g_pixelShader;
ComPtr<ID3D11InputLayout> g_inputLayout;
ComPtr<ID3D11Buffer> g_vertexBuffer;

// DirectInput
ComPtr<IDirectInput8> g_directInput;
ComPtr<IDirectInputDevice8> g_keyboard;
ComPtr<IDirectInputDevice8> g_mouse;

// HDR settings
bool g_hdrEnabled = false;
float g_maxNits = 1000.0f; // Target HDR brightness in nits

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"HDRCalibratorClass";

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create window
    g_hwnd = CreateWindowEx(
        0,
        L"HDRCalibratorClass",
        L"HDR Calibrator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1920, 1080,
        NULL, NULL, hInstance, NULL
    );

    if (g_hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Initialize D3D11
    if (!InitD3D(g_hwnd))
    {
        MessageBox(NULL, L"D3D11 Initialization Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Initialize HDR
    if (!InitHDR())
    {
        MessageBox(NULL, L"HDR Initialization Failed (HDR may not be available)", L"Warning", MB_ICONWARNING | MB_OK);
    }

    // Initialize DirectInput
    if (!InitDirectInput(g_hwnd))
    {
        MessageBox(NULL, L"DirectInput Initialization Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        CleanupD3D();
        return 0;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // Main message loop
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            UpdateInput();
            Render();
        }
    }

    CleanupDirectInput();
    CleanupD3D();

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool InitD3D(HWND hwnd)
{
    HRESULT hr;

    // Get window dimensions
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Create device and context
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL featureLevel;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &g_device,
        &featureLevel,
        &g_context
    );

    if (FAILED(hr))
        return false;

    // Get DXGI factory
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = g_device.As(&dxgiDevice);
    if (FAILED(hr))
        return false;

    ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr))
        return false;

    ComPtr<IDXGIFactory2> dxgiFactory;
    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr))
        return false;

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM; // HDR10 format
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    hr = dxgiFactory->CreateSwapChainForHwnd(
        g_device.Get(),
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &g_swapChain
    );

    if (FAILED(hr))
        return false;

    // Create render target view
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
        return false;

    hr = g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &g_renderTargetView);
    if (FAILED(hr))
        return false;

    g_context->OMSetRenderTargets(1, g_renderTargetView.GetAddressOf(), nullptr);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)width;
    viewport.Height = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    g_context->RSSetViewports(1, &viewport);

    return true;
}

bool InitHDR()
{
    HRESULT hr;

    // Query for DXGI 1.6 swap chain
    ComPtr<IDXGISwapChain4> swapChain4;
    hr = g_swapChain.As(&swapChain4);
    if (FAILED(hr))
        return false;

    // Set HDR10 color space
    UINT colorSpaceSupport = 0;
    hr = swapChain4->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport);
    
    if (SUCCEEDED(hr) && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
    {
        hr = swapChain4->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        if (SUCCEEDED(hr))
        {
            g_hdrEnabled = true;

            // Set HDR metadata
            DXGI_HDR_METADATA_HDR10 hdrMetadata = {};
            hdrMetadata.RedPrimary[0] = 34000;   // DCI-P3 red primary in 0.00002 increments
            hdrMetadata.RedPrimary[1] = 16000;
            hdrMetadata.GreenPrimary[0] = 13250;
            hdrMetadata.GreenPrimary[1] = 34500;
            hdrMetadata.BluePrimary[0] = 7500;
            hdrMetadata.BluePrimary[1] = 3000;
            hdrMetadata.WhitePoint[0] = 15635;
            hdrMetadata.WhitePoint[1] = 16450;
            hdrMetadata.MaxMasteringLuminance = static_cast<UINT>(g_maxNits * 10000);
            hdrMetadata.MinMasteringLuminance = 1; // 0.0001 nits
            hdrMetadata.MaxContentLightLevel = static_cast<UINT16>(g_maxNits);
            hdrMetadata.MaxFrameAverageLightLevel = static_cast<UINT16>(g_maxNits * 0.5f);

            swapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(hdrMetadata), &hdrMetadata);
            
            return true;
        }
    }

    return false;
}

bool InitDirectInput(HWND hwnd)
{
    HRESULT hr;

    // Create DirectInput interface
    hr = DirectInput8Create(
        GetModuleHandle(NULL),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)&g_directInput,
        NULL
    );

    if (FAILED(hr))
        return false;

    // Create keyboard device
    hr = g_directInput->CreateDevice(GUID_SysKeyboard, &g_keyboard, NULL);
    if (FAILED(hr))
        return false;

    hr = g_keyboard->SetDataFormat(&c_dfDIKeyboard);
    if (FAILED(hr))
        return false;

    hr = g_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr))
        return false;

    g_keyboard->Acquire();

    // Create mouse device
    hr = g_directInput->CreateDevice(GUID_SysMouse, &g_mouse, NULL);
    if (FAILED(hr))
        return false;

    hr = g_mouse->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hr))
        return false;

    hr = g_mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr))
        return false;

    g_mouse->Acquire();

    return true;
}

void UpdateInput()
{
    if (g_keyboard)
    {
        BYTE keyState[256];
        HRESULT hr = g_keyboard->GetDeviceState(sizeof(keyState), (LPVOID)&keyState);

        if (FAILED(hr))
        {
            // Device lost, try to reacquire
            g_keyboard->Acquire();
        }
        else
        {
            // Check for specific keys (example: increase/decrease nits with +/-)
            if (keyState[DIK_EQUALS] & 0x80) // + key
            {
                g_maxNits += 10.0f;
                if (g_maxNits > 10000.0f)
                    g_maxNits = 10000.0f;
            }
            if (keyState[DIK_MINUS] & 0x80) // - key
            {
                g_maxNits -= 10.0f;
                if (g_maxNits < 80.0f)
                    g_maxNits = 80.0f;
            }
        }
    }

    if (g_mouse)
    {
        DIMOUSESTATE mouseState;
        HRESULT hr = g_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouseState);

        if (FAILED(hr))
        {
            g_mouse->Acquire();
        }
        // Mouse state can be used for calibration UI interaction
    }
}

void Render()
{
    if (!g_context || !g_swapChain)
        return;

    // Clear to a specific nits value
    // For HDR10, we need to apply PQ (ST.2084) curve to convert linear nits to display values
    // This is typically done in the pixel shader
    
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_context->ClearRenderTargetView(g_renderTargetView.Get(), clearColor);

    // Draw calls would go here
    // Shaders will handle nits-to-PQ conversion

    g_swapChain->Present(1, 0);
}

void CleanupDirectInput()
{
    if (g_keyboard)
    {
        g_keyboard->Unacquire();
        g_keyboard.Reset();
    }

    if (g_mouse)
    {
        g_mouse->Unacquire();
        g_mouse.Reset();
    }

    g_directInput.Reset();
}

void CleanupD3D()
{
    g_vertexBuffer.Reset();
    g_inputLayout.Reset();
    g_pixelShader.Reset();
    g_vertexShader.Reset();
    g_renderTargetView.Reset();
    g_swapChain.Reset();
    g_context.Reset();
    g_device.Reset();
}
