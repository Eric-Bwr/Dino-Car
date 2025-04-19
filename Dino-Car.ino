#include <CAN.h>
#include <Servo.h>

Servo gearServo;
int gearServoPos = 90;
const int gearServoPin = 3;
unsigned long lastServoCommandTime = 0;
bool servoAttached = false;

unsigned long lastRequestTime = 0;
int currentPIDIndex = 0;

const uint8_t pidList[] = { 0x11, 0x0F, 0x04 };
const int numPIDs = sizeof(pidList);

const int batteryPin = A0;
const float R1 = 47000.0; // 47k ohm
const float R2 = 10000.0; // 10k ohm
const float scalingFactor = 0.934; // Cheap resistors

float throttle = -1;
float ambiTemp = -1;
float engineLoad = -1;
float batteryVoltage = -1;

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
  Serial.print(",A:"); Serial.print(ambiTemp);
  Serial.print(",V:"); Serial.println(batteryVoltage);
}

void readCAN() {
  if (!CAN.parsePacket()) return;
  uint8_t buf[8], i = 0;
  while (CAN.available() && i < 8) buf[i++] = CAN.read();

  if (CAN.packetId() == 0x7E8) handleOBDResponse(buf);
  else if (CAN.packetId() == 0x540) handleCustom540(buf);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  if (!CAN.begin(1E6)) while (1);
}

void loop() {
  int sensorValue = analogRead(batteryPin);
  float voltage = sensorValue * (5.0 / 1023.0);
  batteryVoltage = voltage * ((R1 + R2) / R2);
  batteryVoltage *= scalingFactor;
  if (millis() - lastRequestTime >= 5) {
    lastRequestTime = millis();
    sendPIDRequest();
  }
  readCAN();

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.startsWith("G:")) {
      int pos = input.substring(2).toInt();
      if (pos >= 0 && pos <= 180) {
        if (!servoAttached) {
          gearServo.attach(gearServoPin);
          servoAttached = true;
        }
        gearServo.write(pos);
        gearServoPos = pos;
        lastServoCommandTime = millis();
      }
    }
  }

  if (servoAttached && (millis() - lastServoCommandTime > 1000)) {
    gearServo.detach();
    servoAttached = false;
  }
}
