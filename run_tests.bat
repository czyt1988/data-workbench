@echo off
set PYTHONHOME=C:\Program Files\Python311
set PATH=C:\src\Qt\data-workbench\bin_Debug_qt6.7.3_MSVC_x64\bin;C:\src\Qt\data-workbench\build\Desktop_Qt_6_7_3_MSVC2019_64bit-Debug\bin;D:\Qt\6.7.3\msvc2019_64\bin;%PATH%
C:\src\Qt\data-workbench\build\Desktop_Qt_6_7_3_MSVC2019_64bit-Debug\src\tst\DAPyWorkFlow\DAPyWorkFlowTests.exe -v2 -o C:\src\Qt\data-workbench\test_output.txt,txt -o C:\src\Qt\data-workbench\test_output.log,txt
echo EXIT=%ERRORLEVEL%