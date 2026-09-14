call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /O2 /MD /LD jna_main.cpp /Fe:network_monitor.dll iphlpapi.lib ws2_32.lib psapi.lib
