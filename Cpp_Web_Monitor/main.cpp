#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "httplib.h"
#include <iostream>
#include <fstream>
#include <sstream>

extern "C" const char* get_network_data_json_c();

std::string read_html_file(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) return "<h1>Error: Cannot open " + filepath + "</h1>";
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
}

int main() {
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = read_html_file("templates/index.html");
        res.set_content(html, "text/html; charset=utf-8");
    });

    svr.Get("/api/network-data", [](const httplib::Request&, httplib::Response& res) {
        const char* json_data = get_network_data_json_c();
        res.set_content(json_data, "application/json; charset=utf-8");
    });

    std::cout << "Starting C++ Web Server on http://localhost:8080..." << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}