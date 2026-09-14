package com.network.monitor;

import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.ResponseBody;

@Controller
public class NetworkController {

    // Serves the Thymeleaf HTML template
    @GetMapping("/")
    public String index() {
        return "index";
    }

    // Serves the JSON data from C++ DLL via JNA
    @GetMapping("/api/network-data")
    @ResponseBody
    public ResponseEntity<String> getNetworkData() {
        try {
            String jsonData = JnaLibrary.INSTANCE.get_network_data_json_c();
            return ResponseEntity.ok()
                    .header("Content-Type", "application/json")
                    .body(jsonData);
        } catch (Exception e) {
            e.printStackTrace();
            return ResponseEntity.internalServerError().body("{\"error\": \"Failed to load native data.\"}");
        }
    }
}
