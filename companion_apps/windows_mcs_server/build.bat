@echo off
echo Building Windows MCS Server...
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -o ./bin/Release
echo.
echo ✓ Build complete! 
echo Executable: bin\Release\WindowsMCSServer.exe
pause