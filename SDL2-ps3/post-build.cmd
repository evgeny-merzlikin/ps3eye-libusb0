:: copy release dlls
xcopy /Y "build\Win32 Release\ps3eye-lib.dll" "dist\Win32\"
xcopy /Y "build\x64 Release\ps3eye-lib.dll" "dist\x64\"
:: copy include headers
xcopy /Y "include\*.h" "dist\Win32\include\"
xcopy /Y "include\*.h" "dist\x64\include\"
:: copy linker libs
xcopy /Y "build\Win32 Release\ps3eye-lib.lib" "dist\Win32\lib\"
xcopy /Y "build\x64 Release\ps3eye-lib.lib" "dist\x64\lib\"
:: copy vc runtime dlls
xcopy /Y "vc-redist-dlls\Win32 Release\*.dll" "dist\Win32\"
xcopy /Y "vc-redist-dlls\x64 Release\*.dll" "dist\x64\"