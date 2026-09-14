package com.network.monitor;

import com.sun.jna.Library;
import com.sun.jna.Native;

public interface JnaLibrary extends Library {
    // Load the DLL. Note that JNA automatically appends ".dll" on Windows.
    JnaLibrary INSTANCE = Native.load("network_monitor", JnaLibrary.class);

    // This matches the C++ signature: extern "C" __declspec(dllexport) const char* get_network_data_json_c()
    String get_network_data_json_c();
}
