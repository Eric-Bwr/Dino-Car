#include <ACAN2515.h>
#include <SPI.h>

static const byte MCP2515_CS = 10;  // CS pin
static const byte MCP2515_INT = 2;  // INT pin
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  const uint32_t QUARTZ_FREQUENCY = 16 * 1000 * 1000;  // 16 MHz crystal
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 500 * 1000);  // 500 kbit/s

  const uint16_t errorCode = can.begin(settings, [] { can.isr(); });

  if (errorCode == 0) {
    Serial.println("CAN initialized successfully!");
  } else {
    Serial.print("CAN initialization error: 0x");
    Serial.println(errorCode, HEX);
  }
}

void loop() {
  CANMessage message;

  if (can.receive(message)) {
    Serial.print(message.id, HEX);  // CAN ID
    Serial.print(",");
    Serial.print(message.len);      // Data length
    Serial.print(",");

    for (uint8_t i = 0; i < message.len; i++) {
      Serial.print(message.data[i], HEX);
      if (i < message.len - 1) Serial.print(",");
    }
    Serial.println();
  }
}
