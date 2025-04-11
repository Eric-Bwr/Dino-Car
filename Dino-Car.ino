#include <CAN.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);
  if (!CAN.begin(1E6)) {
    while (1);
  }
}

void loop() {
  int packetSize = CAN.parsePacket();

  if (packetSize) {
    Serial.print("ID:0x");
    Serial.print(CAN.packetId(), HEX);
    Serial.print(",L:");
    Serial.print(packetSize);
    Serial.print(",D:");

    while (CAN.available()) {
      Serial.print("0x");
      Serial.print(CAN.read(), HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}
