# Windows MCS Server

BLE GATT Server that exposes Windows media information using the Media Control Service (MCS) protocol.

## Features
- ✅ Reads current media info from Windows (Spotify, YouTube, etc.)
- ✅ Exposes via BLE GATT (MCS Service)
- ✅ Works with ESP32 MCS Client
- ✅ No serial/WiFi required - pure BLE

## Requirements
- Windows 10 version 2004 (build 19041) or later
- Bluetooth LE adapter
- .NET 6.0 Runtime (or use self-contained build)

## Building

### Option 1: Using Visual Studio
1. Open `WindowsMCSServer.csproj` in Visual Studio
2. Build → Publish

### Option 2: Command Line
```bash
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -o ./bin/Release