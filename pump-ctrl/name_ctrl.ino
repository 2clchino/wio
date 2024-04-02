void handleGetName() {
  String _name = get_name(wio_id);
  server.send(200, "text/plain", _name);
}

void handleSetName() {
  if (server.hasArg("name")) {
    String newName = server.arg("name");
    set_name(newName.c_str());
    server.send(200, "text/plain", "Name set successfully");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void set_name(const char *name) {
  File dataFile = SD.open("name.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(name);
    dataFile.close();
    Serial.println("Name saved successfully.");
  } else {
    Serial.println("Error opening name.txt for writing.");
  }
  String names = String(get_name(0));
  processWioData(names);
}

void processWioData(String state) {
    DynamicJsonDocument doc(jsonCapacity);
    DeserializationError error = deserializeJson(doc, state);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        tft.println("Web can't find this wio");
        return;
    }

    JsonObject wio = doc["wio"];
    wio_ip = wio["ip"].as<String>(); // "10.1.2.12"
    wio_name = wio["name"].as<String>(); // "Pump 0"
    wio_number = wio["number"]; // 1
}

String get_name(int num) {
  File dataFile = SD.open("name.txt");
  String wio_data = "";
  if (dataFile) {
    while (dataFile.available()) {
      wio_data += (char)dataFile.read();
    }
    dataFile.close();
  } else {
    Serial.println("name.txt not found. Returning default name.");
    wio_data = "Wio " + String(num); // default
  }

  return wio_data;
}
