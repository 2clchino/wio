#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

File myFile;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  myFile = SD.open("token.txt", FILE_READ);
  if (myFile) {
    Serial.println("token.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening token.txt");
    Serial.println("create token.txt");
    myFile = SD.open("token.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      Serial.println("Writing to token.txt...");
      myFile.println("token is here");
      // close the file:
      myFile.close();
      Serial.println("done.");
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening token.txt");
    }
    myFile = SD.open("token.txt", FILE_READ);
    if (myFile) {
      Serial.println("token.txt:");

      // read from the file until there's nothing else in it:
      while (myFile.available()) {
        Serial.write(myFile.read());
      }
      // close the file:
      myFile.close();
    }
  }
}

void loop() {
  // nothing happens after setup
}
