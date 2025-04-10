#include <ACAN2515.h>
#include <SPI.h>

static const byte MCP2515_CS = 10;
static const byte MCP2515_INT = 2;
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  const uint32_t QUARTZ_FREQUENCY = 16 * 1000 * 1000;
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 500 * 1000);  // CAN-Bitrate: 500 kbit/s

  const uint16_t errorCode = can.begin(settings, [] {
    can.isr();
  });

  if (errorCode == 0) {
    Serial.println("CAN initialisiert!");
  } else {
    Serial.print("Fehler bei CAN-Initialisierung: ");
    Serial.println(errorCode, HEX);
  }
}

void loop() {
  CANMessage message;

  if (can.receive(message)) {
    Serial.print("ID: ");
    Serial.print(message.id, HEX);
    Serial.print(", Length: ");
    Serial.print(message.len);
    Serial.print(", Data: ");

    for (uint8_t i = 0; i < message.len; i++) {
      Serial.print(message.data[i], HEX);
      if (i < message.len - 1) Serial.print(" ");
    }
    Serial.println();
  }
}
