#include "secrets.h"
#include "sampler.h"
#include <WiFi.h>
#include <PubSubClient.h> 

// WiFi and MQTT client objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void networkTask(void *pvParameters) {
    // Connect to WiFi
    Serial.print("[WiFi] Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected!");

    // Setup MQTT Broker
    mqttClient.setServer(MQTT_SERVER, 1883);

    float receivedAvg = 0.0f; 

    for (;;) {
        // Network task wait undefinetely until sampler.cpp does not send something on the queue
        if (xQueueReceive(mqttQueue, &receivedAvg, portMAX_DELAY) == pdTRUE) {
            
            // Check MQTT connection (auto-reconnects if needed)
            if (!mqttClient.connected()) {
                Serial.println("[MQTT] Connecting to broker...");
                if (mqttClient.connect("ESP32_Adaptive_Sampler", TB_TOKEN, NULL)) {
                    Serial.println("[MQTT] Connected!");
                } else {
                    Serial.print("[MQTT] Failed, rc=");
                    Serial.println(mqttClient.state());
                }
            }

            if (mqttClient.connected()) {
                char jsonPayload[64];
                snprintf(jsonPayload, sizeof(jsonPayload), 
                         "{\"signal_average\": %.2f}", 
                         receivedAvg);
                
                // Publish via MQTT (Uses "v1/devices/me/telemetry" on ThingsBoard)
                mqttClient.publish("v1/devices/me/telemetry", jsonPayload);                
                Serial.printf("[MQTT] Sent AVG: %.2f\n", receivedAvg);
            }
        }
        
        // Keeps broker connection alive
        mqttClient.loop(); 
    }
}