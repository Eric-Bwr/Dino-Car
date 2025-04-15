#include <CAN.h>

unsigned long lastRequestTime = 0;
int currentPIDIndex = 0;

const uint8_t pidList[] = { 0x11, 0x0F, 0x04 };
const int numPIDs = sizeof(pidList);

float throttle = -1;
float ambiTemp = -1;
float engineLoad = -1;

void sendPIDRequest() {
  uint8_t frame[8] = { 0x02, 0x01, pidList[currentPIDIndex], 0, 0, 0, 0, 0 };
  CAN.beginPacket(0x7DF);
  CAN.write(frame, 8);
  CAN.endPacket();
  currentPIDIndex = (currentPIDIndex + 1) % numPIDs;
}

void handleOBDResponse(uint8_t* res) {
  if (res[1] != 0x41) return;

  switch (res[2]) {
    case 0x11: throttle = res[3] * 100.0 / 255.0; break;
    case 0x0F: ambiTemp = res[3] - 40; break;
    case 0x04: engineLoad = res[3] * 100.0 / 255.0; break;
  }
}

void handleCustom540(uint8_t* data) {
  if (data[0] != 0x02) return;

  uint8_t gear = data[3] & 0x0F;
  uint16_t rpm = (data[1] << 8) | data[2];
  float temp = ((data[6] << 8) | data[7]) / 10.0;

  Serial.print("G:"); Serial.print(gear);
  Serial.print(",R:"); Serial.print(rpm);
  Serial.print(",T:"); Serial.print(temp);
  Serial.print(",Th:"); Serial.print(throttle);
  Serial.print(",L:"); Serial.print(engineLoad);
  Serial.print(",A:"); Serial.println(ambiTemp);
}

void readCAN() {
  if (!CAN.parsePacket()) return;
  uint8_t buf[8], i = 0;
  while (CAN.available() && i < 8) buf[i++] = CAN.read();

  if (CAN.packetId() == 0x7E8) handleOBDResponse(buf);
  else if (CAN.packetId() == 0x540) handleCustom540(buf);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  if (!CAN.begin(1E6)) while (1);
}

void loop() {
  if (millis() - lastRequestTime >= 50) {
    lastRequestTime = millis();
    sendPIDRequest();
  }
  readCAN();
}
