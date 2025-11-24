# HDR Calibrator

A simple DirectX 11 application for HDR10 calibration that runs on Windows and Linux/Proton. This application allows direct control over pixel nit values for precise HDR display calibration.

## Features

- **HDR10 Support**: Full HDR10 implementation with PQ (ST.2084) encoding
- **Direct Nits Control**: Set exact nit values for pixels (0-10000 nits range)
- **DirectInput Support**: Keyboard and mouse input handling
- **Cross-Platform**: Runs natively on Windows and via Proton on Linux
- **R10G10B10A2 Format**: Uses proper HDR10 color format

## Requirements

### Windows
- Windows 10/11 with HDR-capable display
- DirectX 11 compatible GPU
- Visual Studio 2019 or later (for building)
- CMake 3.15 or later

### Linux (Proton)
- Linux distribution with Wine/Proton support
- MinGW-w64 cross-compiler (`x86_64-w64-mingw32-g++`)
- CMake 3.15 or later

## Building

### Windows (Native Build)

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Linux (Cross-compile for Proton)

Install MinGW-w64:
```bash
# Ubuntu/Debian
sudo apt-get install mingw-w64 cmake

# Arch Linux
sudo pacman -S mingw-w64-gcc cmake
```

Build:
```bash
mkdir build
cd build
cmake ..
make
```

## Running

### Windows
Simply run the executable:
```bash
./build/Release/hdr-calibrator.exe
```

### Linux (via Proton/Wine)
```bash
wine ./build/hdr-calibrator.exe
# Or through Steam's Proton
```

## Usage

- **ESC**: Exit application
- **+/=**: Increase target nits by 10
- **-**: Decrease target nits by 10

## HDR10 Details

This application uses:
- **Color Space**: DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 (HDR10 / Rec.2020 with PQ)
- **Format**: DXGI_FORMAT_R10G10B10A2_UNORM (10-bit per channel)
- **PQ Curve**: ST.2084 Perceptual Quantizer for nits-to-display conversion
- **Nits Range**: 0-10000 nits (adjustable via metadata)

## Code Structure

- `main.cpp`: Main application loop, D3D11 initialization, HDR setup, DirectInput
- `hdr_utils.h`: HDR utility functions for nits/PQ conversions
- `shaders/vertex_shader.hlsl`: Vertex shader (passes through nits values)
- `shaders/pixel_shader.hlsl`: Pixel shader (converts nits to PQ-encoded output)

## How Nits Control Works

1. Define colors in linear nits (e.g., 100 nits, 1000 nits)
2. Pass nits values to shaders as vertex colors
3. Pixel shader applies PQ (ST.2084) curve to convert to display values
4. Output to R10G10B10A2 swap chain in HDR10 color space

Example:
```cpp
HDR::RGBNits color = HDR::RGBNits::FromGray(500.0f); // 500 nits gray
// Shader automatically converts to proper PQ values
```

## License

MIT License - See LICENSE file for details