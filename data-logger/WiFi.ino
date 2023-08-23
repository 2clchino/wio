int timeout=0;
void connectToWiFi() {
    Serial.println("Connecting to WiFi network: " + String(ssid));
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
        WiFi.begin(ssid, pass);
        
        timeout++;
        if (timeout > 9)
            break;
    }
    Serial.println("Connected to the WiFi network");
    Serial.print("IP Address: ");
    Serial.println (WiFi.localIP()); // prints out the device's IP address
    IPAddress localIP = WiFi.localIP();
    wio_id = localIP[3];
}
