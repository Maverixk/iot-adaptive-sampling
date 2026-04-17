#include "secrets.h"
#include "network.h"
#include "sampler.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <RadioLib.h>

// WiFi and MQTT client objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// RadioLib variables
uint64_t joinEUI = LORA_JOIN_EUI;
uint64_t devEUI  = LORA_DEV_EUI;
uint8_t appKey[] = LORA_APP_KEY;

// Radio object
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);

// LoRaWAN node object
LoRaWANNode node(&radio, &EU868);

void wifiTask(void *pvParameters) {
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
    uint32_t wifiPacketCounter = 0;

    for (;;) {
        // WiFi task waits undefinetely until sampler.cpp does not send something on the queue
        if (xQueueReceive(mqttWifiQueue, &receivedAvg, portMAX_DELAY) == pdTRUE) {
            
            // Check MQTT connection (auto-reconnects if needed)
            if (!mqttClient.connected()) {
                Serial.println("[MQTT] Connecting to broker...");
                if (mqttClient.connect("esp32_adaptive_sampling", TB_TOKEN, "")) {
                    Serial.println("[MQTT] Connected!");
                } else {
                    Serial.print("[MQTT] Failed, rc=");
                    Serial.println(mqttClient.state());
                }
            }

            if (mqttClient.connected()) {
                wifiPacketCounter++;

                char jsonPayload[96];
                snprintf(jsonPayload, sizeof(jsonPayload), 
                         "{\"signal_average\": %.2f, \"packet_id\": %u}", 
                         receivedAvg, wifiPacketCounter);
                
                mqttClient.publish("v1/devices/me/telemetry", jsonPayload);                
                Serial.printf("[MQTT] Sent Packet %u | AVG: %.2f\n", wifiPacketCounter, receivedAvg);
            }
        }
        
        // Keeps broker connection alive
        mqttClient.loop(); 
    }
}

void loraTask(void *pvParameters){
    Serial.println("[LoRa] Initializing SX1262...");
    
    // Initialize physical chip
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Radio error: %d\n", state);
        vTaskDelete(NULL); // Kills the task if the radio object is broken
    }

    state = node.beginOTAA(joinEUI, devEUI, appKey, appKey);

    // Send request to TTN and wait for reply
    Serial.println("[LoRa] Connecting to TTN (Join OTAA)...");
    state = node.activateOTAA();

    if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
        Serial.println("[LoRa] Successfully connected to TTN!");
    } else {
        Serial.printf("[LoRa] Join failed, error: %d\n", state);
    }

    float receivedLoraAvg = 0.0f;
    uint16_t loraPacketCounter = 0;

    for (;;) {
        if (xQueueReceive(mqttLoraQueue, &receivedLoraAvg, portMAX_DELAY) == pdTRUE) {

            loraPacketCounter++;
            
            // Multiply for 10 (e.g. 1920.5 becomes 19205) to save 1 decimal.
            // We put it in an uint16_t (takes exactly 2 bytes).
            uint16_t dataToSend = (uint16_t)(receivedLoraAvg * 10);
            
            uint8_t payload[4]; 
            
            payload[0] = (uint8_t)(dataToSend >> 8);   // MSB
            payload[1] = (uint8_t)(dataToSend & 0xFF); // LSB
            
            payload[2] = (uint8_t)(loraPacketCounter >> 8);   // MSB
            payload[3] = (uint8_t)(loraPacketCounter & 0xFF); // LSB
            
            // Send 4 bytes to TTN
            int state = node.sendReceive(payload, sizeof(payload));
            
            if(state == RADIOLIB_ERR_NONE){
                Serial.printf("[LoRa] Sent Packet %u | AVG: %.2f -> Bytes: %02X %02X %02X %02X\n", 
                              loraPacketCounter, receivedLoraAvg, payload[0], payload[1], payload[2], payload[3]);
            }
            else{
                Serial.printf("[LoRa] Errore invio payload: %d\n", state);
            }
        }
    }
}
