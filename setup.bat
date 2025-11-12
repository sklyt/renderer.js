@echo off
echo === raylib-napi Windows setup ===

where vcpkg >nul 2>&1
if %ERRORLEVEL% EQU 0 (
  echo vcpkg found, installing raylib via vcpkg (x64)...
  vcpkg install raylib:x64-windows
  echo Make sure vcpkg integrate install or your VS toolchain is configured to see vcpkg libs.
) else (
  echo vcpkg not found.
  echo Please install vcpkg and run: vcpkg install raylib:x64-windows
  echo Or install raylib manually and update binding.gyp to point to raylib.lib
)

echo Installing npm deps...
npm install

echo Building the native addon...
call npm run build

echo Done. Run: node index.js
pause
