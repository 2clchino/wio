void handleGetName() {
  String name = get_name(wio_id);
  server.send(200, "text/plain", name);
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
}

String get_name(int num) {
  File dataFile = SD.open("name.txt");
  wio_name = "";
  if (dataFile) {
    while (dataFile.available()) {
      wio_name += (char)dataFile.read();
    }
    dataFile.close();
  } else {
    Serial.println("name.txt not found. Returning default name.");
    wio_name = "Wio " + String(num); // default
  }

  return wio_name;
}
