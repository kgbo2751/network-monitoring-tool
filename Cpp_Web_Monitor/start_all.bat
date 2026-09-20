@echo off
echo Starting C++ Web Server on port 8080...
start cmd /k "cpp_web_server.exe"

echo Starting Nginx on port 80...
cd nginx
start nginx.exe

echo All systems running! Open http://localhost in your browser.
pause
