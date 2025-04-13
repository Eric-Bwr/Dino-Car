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

  if (packetSize && CAN.packetId() == 0x540) {
    uint8_t data[8];
    int index = 0;

    while (CAN.available()) {
      data[index++] = CAN.read();
    }

    if (data[0] == 0x02) {
      uint16_t engine_rpm = (data[1] << 8) | data[2];
      uint8_t gear = data[3] & 0x0F;
      uint16_t coolant_temp = (data[6] << 8) | data[7];

      Serial.print("G:");
      Serial.print(gear);
      Serial.print(",R:");
      Serial.print(engine_rpm);
      Serial.print(",T:");
      Serial.print(coolant_temp / 10.0);
      Serial.println();
    }
  }
}
