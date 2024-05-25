// APP2 S6 - Remise : 2024/05/24
// William Pépin  - PEPW3101
// Gabriel Vachon - VACG3501
// Description : UART master

#include <HardwareSerial.h>

HardwareSerial ESPSerial(2);

#define RxPin 16
#define TxPin 17
#define BAUDRATE 9600
#define SER_BUF_SIZE 1024

void setup() {
  // Serial communication with the ESP32
  ESPSerial.setRxBufferSize(SER_BUF_SIZE);
  ESPSerial.begin(BAUDRATE, SERIAL_8N1, RxPin, TxPin);

  // Serial communication with the computer
  Serial.begin(115200);

  // Wait for initialization
  delay(1000);
}

void loop() {
  // Master Asks for data
  ESPSerial.println("Je veux les données");

  // Master Waits for data
  while (ESPSerial.available()) {
    String response = ESPSerial.readStringUntil('\n');
    Serial.println(response);
  }
  delay(1000);
}