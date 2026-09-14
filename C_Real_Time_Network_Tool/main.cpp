#define _WIN32_WINNT 0x0600
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <netioapi.h>
#include <psapi.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma warning(disable:4996)

namespace py = pybind11;

py::dict get_full_network_data() {
    py::dict data;

    py::list interfaces;
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = 0;
    GetIfTable(NULL, &dwSize, FALSE);
    pIfTable = (MIB_IFTABLE*)malloc(dwSize);
    if (pIfTable) {
        if (GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                MIB_IFROW row = pIfTable->table[i];
                py::dict iface;
                iface["name"] = std::string((char*)row.bDescr);
                iface["status"] = (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL) ? "Operational" : 
                                  (row.dwOperStatus == IF_OPER_STATUS_DISCONNECTED) ? "Disconnected" : "Other";
                iface["speed_mbps"] = row.dwSpeed / 1000000.0;
                interfaces.append(iface);
            }
        }
        free(pIfTable);
    }
    data["interfaces"] = interfaces;

    std::string ipv4 = "", subnet = "", gateway = "", mac = "", dns = "";
    DWORD bufLen = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO* adapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
    if (GetAdaptersInfo(adapterInfo, &bufLen) == ERROR_BUFFER_OVERFLOW) {
        free(adapterInfo);
        adapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
    }
    if (GetAdaptersInfo(adapterInfo, &bufLen) == NO_ERROR) {
        IP_ADAPTER_INFO* adapter = adapterInfo;
        while (adapter) {
            if (strcmp(adapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0 &&
                strcmp(adapter->GatewayList.IpAddress.String, "0.0.0.0") != 0) {
                ipv4 = adapter->IpAddressList.IpAddress.String;
                subnet = adapter->IpAddressList.IpMask.String;
                gateway = adapter->GatewayList.IpAddress.String;
                char macBuf[32];
                sprintf(macBuf, "%02X-%02X-%02X-%02X-%02X-%02X",
                    adapter->Address[0], adapter->Address[1], adapter->Address[2],
                    adapter->Address[3], adapter->Address[4], adapter->Address[5]);
                mac = macBuf;
                break;
            }
            adapter = adapter->Next;
        }
    }
    free(adapterInfo);

    FIXED_INFO* fixedInfo = (FIXED_INFO*)malloc(sizeof(FIXED_INFO));
    ULONG fixedInfoLen = sizeof(FIXED_INFO);
    if (GetNetworkParams(fixedInfo, &fixedInfoLen) == ERROR_BUFFER_OVERFLOW) {
        free(fixedInfo);
        fixedInfo = (FIXED_INFO*)malloc(fixedInfoLen);
    }
    if (GetNetworkParams(fixedInfo, &fixedInfoLen) == NO_ERROR) {
        dns = fixedInfo->DnsServerList.IpAddress.String;
    }
    free(fixedInfo);

    data["ipv4"] = ipv4;
    data["subnet"] = subnet;
    data["gateway"] = gateway;
    data["mac"] = mac;
    data["dns"] = dns;

    std::string downloadStr = "0 KB/s", uploadStr = "0 KB/s";
    pIfTable = (MIB_IFTABLE*)malloc(sizeof(MIB_IFTABLE));
    dwSize = sizeof(MIB_IFTABLE);
    if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (MIB_IFTABLE*)malloc(dwSize);
    }
    if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
        DWORD bestIndex = 0;
        DWORD maxTraffic = 0;
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            MIB_IFROW row = pIfTable->table[i];
            if (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL &&
                (strstr((char*)row.bDescr, "Wi-Fi") || strstr((char*)row.bDescr, "Ethernet")) &&
                !strstr((char*)row.bDescr, "VirtualBox") && !strstr((char*)row.bDescr, "VMware")) {
                if (row.dwInOctets > maxTraffic) {
                    maxTraffic = row.dwInOctets;
                    bestIndex = i;
                }
            }
        }
        
        MIB_IFROW row = pIfTable->table[bestIndex];
        DWORD rxOld = row.dwInOctets;
        DWORD txOld = row.dwOutOctets;
        Sleep(3000);
        if (GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            row = pIfTable->table[bestIndex];
            DWORD rxNew = row.dwInOctets;
            DWORD txNew = row.dwOutOctets;
            DWORD downloadKBps = (rxNew > rxOld) ? (rxNew - rxOld) / 1024 : 0;
            DWORD uploadKBps = (txNew > txOld) ? (txNew - txOld) / 1024 : 0;
            char dBuf[32], uBuf[32];
            sprintf(dBuf, "%lu KB/s", downloadKBps);
            sprintf(uBuf, "%lu KB/s", uploadKBps);
            downloadStr = dBuf;
            uploadStr = uBuf;
        }
    }
    if (pIfTable) free(pIfTable);

    data["download"] = downloadStr;
    data["upload"] = uploadStr;

    std::string pingStr = "Ping Error";
    FILE* fp;
    char buffer[256];
    int total = 0, min = 9999, max = 0;
    for (int i = 0; i < 3; i++) {
        fp = _popen("ping -n 1 8.8.8.8", "r");
        if (fp) {
            int found = 0;
            while (fgets(buffer, sizeof(buffer), fp)) {
                char* timePtr = strstr(buffer, "time=");
                if (!timePtr) timePtr = strstr(buffer, "시간=");
                if (timePtr) {
                    int time;
                    if (sscanf(timePtr, "time=%d", &time) == 1 || sscanf(timePtr, "시간=%d", &time) == 1) {
                        total += time;
                        if (time < min) min = time;
                        if (time > max) max = time;
                        found = 1;
                    }
                }
            }
            _pclose(fp);
            if (found) Sleep(300);
        }
    }
    if (min != 9999) {
        char pBuf[128];
        sprintf(pBuf, "Min=%dms, Max=%dms, Avg=%dms", min, max, total / 3);
        pingStr = pBuf;
    }
    data["ping"] = pingStr;

    ULONG inPackets = 0, outPackets = 0, inErrors = 0, outErrors = 0;
    double lossRate = 0.0;
    pIfTable = (MIB_IFTABLE*)malloc(sizeof(MIB_IFTABLE));
    dwSize = sizeof(MIB_IFTABLE);
    if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (MIB_IFTABLE*)malloc(dwSize);
    }
    if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
        DWORD bestIndex = 0;
        DWORD maxTraffic = 0;
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            MIB_IFROW row = pIfTable->table[i];
            if (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL &&
                (strstr((char*)row.bDescr, "Wi-Fi") || strstr((char*)row.bDescr, "Ethernet")) &&
                !strstr((char*)row.bDescr, "VirtualBox") && !strstr((char*)row.bDescr, "VMware")) {
                if (row.dwInOctets > maxTraffic) {
                    maxTraffic = row.dwInOctets;
                    bestIndex = i;
                }
            }
        }
        
        MIB_IFROW row = pIfTable->table[bestIndex];
        inPackets = row.dwInUcastPkts;
        outPackets = row.dwOutUcastPkts;
        inErrors = row.dwInErrors;
        outErrors = row.dwOutErrors;
        ULONG totalPkts = row.dwInUcastPkts + row.dwOutUcastPkts;
        ULONG totalErrors = row.dwInErrors + row.dwOutErrors;
        if (totalPkts > 0) lossRate = ((double)totalErrors / totalPkts) * 100.0;
    }
    if (pIfTable) free(pIfTable);
    
    data["inPackets"] = inPackets;
    data["outPackets"] = outPackets;
    data["inErrors"] = inErrors;
    data["outErrors"] = outErrors;
    data["lossRate"] = lossRate;

    DWORD tcpCount = 0;
    PMIB_TCPTABLE tcpTable;
    DWORD tcpSize = 0;
    GetTcpTable(NULL, &tcpSize, TRUE);
    tcpTable = (PMIB_TCPTABLE)malloc(tcpSize);
    if (tcpTable && GetTcpTable(tcpTable, &tcpSize, TRUE) == NO_ERROR) {
        tcpCount = tcpTable->dwNumEntries;
    }
    if (tcpTable) free(tcpTable);
    data["tcpCount"] = tcpCount;

    DWORD udpCount = 0;
    PMIB_UDPTABLE udpTable;
    DWORD udpSize = 0;
    GetUdpTable(NULL, &udpSize, TRUE);
    udpTable = (PMIB_UDPTABLE)malloc(udpSize);
    if (udpTable && GetUdpTable(udpTable, &udpSize, TRUE) == NO_ERROR) {
        udpCount = udpTable->dwNumEntries;
    }
    if (udpTable) free(udpTable);
    data["udpCount"] = udpCount;

    py::list processes;
    DWORD extSize = 0;
    GetExtendedTcpTable(NULL, &extSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    PMIB_TCPTABLE_OWNER_PID tableOld = (PMIB_TCPTABLE_OWNER_PID)malloc(extSize);
    if (tableOld) GetExtendedTcpTable(tableOld, &extSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);

    Sleep(3000);

    PMIB_TCPTABLE_OWNER_PID tableNew = (PMIB_TCPTABLE_OWNER_PID)malloc(extSize);
    if (tableNew && GetExtendedTcpTable(tableNew, &extSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        for (DWORD i = 0; i < tableNew->dwNumEntries; i++) {
            DWORD pid = tableNew->table[i].dwOwningPid;
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            char procName[MAX_PATH] = "Unknown";
            if (hProc) {
                DWORD size = MAX_PATH;
                QueryFullProcessImageNameA(hProc, 0, procName, &size);
                CloseHandle(hProc);
            }
            if (strcmp(procName, "Unknown") != 0) {
                py::dict proc;
                proc["pid"] = pid;
                proc["name"] = std::string(procName);
                processes.append(proc);
            }
        }
    }
    if (tableOld) free(tableOld);
    if (tableNew) free(tableNew);

    data["processes"] = processes;

    return data;
}

PYBIND11_MODULE(network_monitor, m) {
    m.def("get_data", &get_full_network_data);
}