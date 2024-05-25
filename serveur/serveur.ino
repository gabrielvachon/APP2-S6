// APP2 S6 - Remise : 2024/05/24
// William Pépin  - PEPW3101
// Gabriel Vachon - VACG3501
// Description : code de la station météo

#include <Wire.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

using namespace std;

BLEServer* pServer = NULL;
BLECharacteristic* pCharHUM = NULL;
BLECharacteristic* pCharTEMP    = NULL;
BLECharacteristic* pCharPRS     = NULL;
BLECharacteristic* pCharLUM     = NULL;
BLECharacteristic* pCharWS      = NULL;
BLECharacteristic* pCharWD      = NULL;
BLECharacteristic* pCharRain    = NULL;
BLEDescriptor *pDescr;
BLE2902 *pBLE2902;
BLE2902 *pBLE2902_2;
BLE2902 *pBLE2902_3;
BLE2902 *pBLE2902_4;
BLE2902 *pBLE2902_5;
BLE2902 *pBLE2902_6;
BLE2902 *pBLE2902_7;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// UUID definitions
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define HUM_UUID            "76ca544c-b8c2-404b-a88a-d6aa312d4d9d"
#define TEMP_UUID           "341019a0-c3d6-40ec-baee-aeb8127031c8"
#define PRS_UUID            "2a7f3b8d-3583-4e14-b770-79ddaf2c1674"
#define LUM_UUID            "004f0e25-5c29-4d34-ad9c-ca73bade9bdc"
#define WS_UUID             "a6ed8fd2-9c68-4c2a-8de5-ac0aa9b3446e"
#define WD_UUID             "e9a27706-1d6d-42ed-a2e2-0119d49a1b50"
#define RAIN_UUID           "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// Function declarations
double_t getLuminosity();
double_t getHumidity();
double_t getWindSpeed();
double_t getWindDirection();
double_t getRainAccumulation();
double_t getTemperature(bool getRaw=false);
double_t getPressure();
void countWindSpd();

// Pins
int pinLum      = 34;
int pinHum      = 16;
int pinWDir     = 35;
int pinWSpd     = 27;
int pinPlu      = 23;

int64_t reading = 0;

// Raw values
int32_t rawTemp = 0;
int32_t rawPrs  = 0;

// Coefficients DPS310
int64_t c00_c10 = 0;
int32_t c0_c1   = 0,
        c00     = 0,
        c10     = 0;
int16_t c0      = 0,
        c1      = 0,
        c20     = 0,
        c30     = 0,
        c01     = 0,
        c11     = 0,
        c21     = 0;

// Actual data
double_t compTemp = 0;
double_t compPrs  = 0;

int valPlu    = 0;
int nbWindSpd = 0;

// Oversampling factors
int kT = 3670016;
int kP = 3670016;

void setup() {
  // Set pins
  pinMode(pinLum,   INPUT);
  pinMode(pinHum,   INPUT);
  pinMode(pinWDir,  INPUT);
  pinMode(pinWSpd,  INPUT);
  pinMode(pinPlu,   INPUT);

  // Interrupts
  attachInterrupt(pinWSpd, countWindSpd, RISING);
  attachInterrupt(pinPlu, rainBeDrippin, RISING);
  
  // Begin I2C
  Wire.begin();

  // Begin serial
  Serial.begin(115200);

  Serial.println("Hello world!");
  
  delay(250);

  // Read reg 0x10 to 0x20
  for(byte i = 0x10; i < 0x20; i++)
  {
    Wire.beginTransmission(0x77);
    Wire.write(i);
    Wire.read();
    Wire.endTransmission();
    delay(100);
  }

  // Configurations for DPS310
  // PRS_CFG
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x06));
  Wire.write(byte(0b00000010));
  Wire.endTransmission();
  
  delay(100);

  // TMP_CFG
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x07));
  Wire.write(byte(0b10000010));
  Wire.endTransmission();
  
  delay(100);

  // CFG_REG
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x09));
  Wire.write(byte(0b00000000));
  Wire.endTransmission();
  
  delay(100);

    Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 40);

  // Create a BLE Characteristic
  pCharHUM = pService->createCharacteristic(
                      HUM_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharTEMP = pService->createCharacteristic(
                    TEMP_UUID,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
  pCharPRS  = pService->createCharacteristic(
                      PRS_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharLUM  = pService->createCharacteristic(
                      LUM_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharWS   = pService->createCharacteristic(
                      WS_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharWD   = pService->createCharacteristic(
                      WD_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharRain = pService->createCharacteristic(
                      RAIN_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );                 

  // Create a BLE Descriptor  
  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);

  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);

  pBLE2902_3 = new BLE2902();
  pBLE2902_3->setNotifications(true);

  pBLE2902_4 = new BLE2902();
  pBLE2902_4->setNotifications(true);

  pBLE2902_5 = new BLE2902();
  pBLE2902_5->setNotifications(true);

  pBLE2902_6 = new BLE2902();
  pBLE2902_6->setNotifications(true);

  pBLE2902_7 = new BLE2902();
  pBLE2902_7->setNotifications(true);

  pCharHUM  ->addDescriptor(pBLE2902);
  pCharTEMP ->addDescriptor(pBLE2902_2);
  pCharPRS  ->addDescriptor(pBLE2902_3);
  pCharLUM  ->addDescriptor(pBLE2902_4);
  pCharWS   ->addDescriptor(pBLE2902_5);
  pCharWD   ->addDescriptor(pBLE2902_6);
  pCharRain ->addDescriptor(pBLE2902_7);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // Get values
  double_t valLum = getLuminosity();
  double_t valHum = getHumidity();
  double_t  valWD  = getWindDirection();
  double_t  valRn  = getRainAccumulation();
  double_t valTmp = getTemperature();
  double_t valPrs = getPressure();
  double_t valWS  = getWindSpeed();

  Serial.print("Luminosity (Lux): ");
  Serial.println(valLum);

  Serial.print("Humidity (%): ");
  Serial.println(valHum);

  Serial.print("Wind direction (degree): ");
  Serial.println(valWD);

  Serial.print("Rain (mm): ");
  Serial.println(valRn);

  Serial.print("Temperature (Celsius): ");
  Serial.println(valTmp);

  Serial.print("Pressure (Pascal): ");
  Serial.println(valPrs);
  
  Serial.print("Wind speed (km/h): ");
  Serial.println(valWS);

  // BLE - Notify
  // Notify changed value
  if (deviceConnected) {
      pCharHUM  ->setValue(valHum);
      pCharPRS  ->setValue(valPrs);
      pCharLUM  ->setValue(valLum);
      pCharWS   ->setValue(valWS);
      pCharWD   ->setValue(valWD);
      pCharRain ->setValue(valRn);
      pCharTEMP ->setValue(valTmp);

      pCharHUM  ->notify();
      pCharTEMP ->notify();
      pCharPRS  ->notify();
      pCharLUM  ->notify();
      pCharWS   ->notify();
      pCharWD   ->notify();
      pCharRain ->notify();
  }
  // Disconnect
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // Connect
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }
}

double_t getLuminosity()
{
  // -----------------------------------------------------------------
  // Luminosity retrieval
  // -----------------------------------------------------------------
  double_t valLum = 0;
  return valLum = analogRead(pinLum)/24.57;
}

double_t getHumidity()
{
  // -----------------------------------------------------------------
  // Humidity retrieval (DHT11)
  // -----------------------------------------------------------------
  int i, j;
  int duree[42];
  unsigned long pulse;
  byte data[5];
  float humidite;
  float temperature;
  
  pinMode(pinHum, OUTPUT_OPEN_DRAIN);
  digitalWrite(pinHum, HIGH);
  delay(250);
  digitalWrite(pinHum, LOW);
  delay(20);
  digitalWrite(pinHum, HIGH);
  delayMicroseconds(40);
  pinMode(pinHum, INPUT_PULLUP);
  
  while (digitalRead(pinHum) == HIGH);
  i = 0;

  do {
        pulse = pulseIn(pinHum, HIGH);
        duree[i] = pulse;
        i++;
  } while (pulse != 0);
 
  if (i != 42) 
    Serial.printf(" Erreur timing \n"); 

  for (i=0; i<5; i++) {
    data[i] = 0;
    for (j = ((8*i)+1); j < ((8*i)+9); j++) {
      data[i] = data[i] * 2;
      if (duree[j] > 50) {
        data[i] = data[i] + 1;
      }
    }
  }

  if ( (data[0] + data[1] + data[2] + data[3]) != data[4] ) 
    Serial.println(" Erreur checksum");

  humidite = data[0] + (data[1] / 256.0);
  temperature = data [2] + (data[3] / 256.0);
  return humidite;
}

double_t getWindSpeed()
{
  // -----------------------------------------------------------------
  // Wind speed
  // -----------------------------------------------------------------
  int windSpd = nbWindSpd;
  nbWindSpd = 0;
  delay(1000);
  return windSpd * 2.4;
}

double_t getWindDirection()
{
  // -----------------------------------------------------------------
  // Wind direction
  // -----------------------------------------------------------------
  switch (analogRead(pinWDir)) {
    case 3880 ... 3920:
      return 0;
      break;
    case 3220 ... 3250:
      return 22.5;
      break;
    case 3520 ... 3550:
      return 45;
      break;
    case 2680 ... 2720:
      return 67.5;
      break;
    case 3030 ... 3060:
      return 90;
      break;
    case 1480 ... 1520:
      return 112.5;
      break;
    case 1700 ... 1750:
      return 135;
      break;
    case 175 ... 205:
      return 157.5;
      break;
    case 210 ... 240:
      return 180;
      break;
    case 100 ... 130:
      return 202.5;
      break;
    case 575 ... 610:
      return 225;
      break;
    case 340 ... 370:
      return 247.5;
      break;
    case 990 ... 1020:
      return 270;
      break;
    case 820 ... 850:
      return 292.5;
      break;
    case 2390 ... 2420:
      return 315;
      break;
    case 2270 ... 2300:
      return 337.5;
      break;
  }
}

double_t getRainAccumulation()
{
  // -----------------------------------------------------------------
  // Rain accumulation
  // -----------------------------------------------------------------
  return valPlu*0.2794;
}

double_t getTemperature(bool getRaw)
{
  // -----------------------------------------------------------------
  // Temperature retrieval
  // -----------------------------------------------------------------
  // Write 0x02 to reg 0x08
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x08));
  Wire.write(byte(0x02));
  Wire.endTransmission();
  
  delay(100);

  // Get raw 24-bit temperature in 2's complement
  Wire.beginTransmission(0x77);  
  Wire.write(byte(0x03));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 3);    // Read reg 0x03 to 0x05
  if (3 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (TMP_B2)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (TMP_B1)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive low byte (TMP_B0)
    rawTemp = reading;
  }

  delay(100);

  // Apply 2's complement to raw temperature
  if(rawTemp & 0x800000)
  {
    rawTemp |= 0xFF800000;
  }

  if(getRaw)
  {
    return (double_t)rawTemp;
  }

  // Get coefficients c0 and c1 (24 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x10));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 3);    // Read reg 0x10 to 0x12
  if (3 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c0)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c0/MSB c1)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive low byte (LSB c1)
    c0_c1 = reading;
  }

  delay(100);

  // Split c0 in c1 in their respective 12-bit signature
  c0 = (c0_c1 & 0x00FFF000) >> 12;
  c1 = (c0_c1 & 0x00000FFF);

  // Apply 2's complement for c0
  if(c0 & 0x0800)
  {
    c0 |= 0xF000;
  }

  // Apply 2's complement for c1
  if(c1 & 0x0800)
  {
    c1 |= 0xF000;
  }

  // Calculate raw scaled temperature according to oversampling rate
  double_t rawScaledTemp = (double_t)rawTemp/(double_t)kT;

  // Calculate actual temperature in celsius
  return c0*0.5+c1*rawScaledTemp;
}

double_t getPressure()
{
  // -----------------------------------------------------------------
  // Pressure retrieval
  // -----------------------------------------------------------------
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x08));
  Wire.write(byte(0x01));
  Wire.endTransmission();

  delay(100);

  Wire.beginTransmission(0x77);  
  Wire.write(byte(0x00));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 3);
  if (3 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (PRS_B2)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (PRS_B1)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive low byte (PRS_B0)
    rawPrs = reading;
  }

  delay(100);
  
  // Apply 2's complement to raw pressure
  if(rawPrs & 0x800000)
  {
    rawPrs |= 0xFF800000;
  }

  // Get coefficients c00 and c10 (40 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x13));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 5);    // Read reg 0x10 to 0x12
  if (5 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c00)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c00)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive low byte (XLSB c00/MSB c10)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive low byte (LSB c10)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive low byte (XLSB c10)
    c00_c10 = reading;
  }

  delay(100);

  // Split c00 and c10 in their respective 20-bit signature
  c00 = (c00_c10 & 0x000000FFFFF00000) >> 20;
  c10 = (c00_c10 & 0x00000000000FFFFF);

  // Apply 2's complement for c00
  if(c00 & 0x80000)
  {
    c00 |= 0xFFF80000;
  }
  
  // Apply 2's complement for c10
  if(c10 & 0x80000)
  {
    c10 |= 0xFFF80000;
  }

  // Get coefficient c01 (16 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x18));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 2);    // Read reg 0x18 to 0x19
  if (2 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c01)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c01)
    c01 = reading;
  }

  delay(100);

  // Get coefficient c11 (16 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x1A));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 2);    // Read reg 0x1A to 0x1B
  if (2 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c11)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c11)
    c11 = reading;
  }

  delay(100);

  // Get coefficient c20 (16 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x1C));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 2);    // Read reg 0x1C to 0x1D
  if (2 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c20)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c20)
    c20 = reading;
  }

  delay(100);

  // Get coefficient c21 (16 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x1E));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 2);    // Read reg 0x1E to 0x1F
  if (2 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c21)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c21)
    c21 = reading;
  }

  delay(100);

  // Get coefficient c30 (16 bits in total)
  Wire.beginTransmission(0x77);
  Wire.write(byte(0x20));
  Wire.endTransmission();
  Wire.requestFrom(0x77, 2);    // Read reg 0x20 to 0x21
  if (2 <= Wire.available()) {  // If three bytes were received
    reading = Wire.read();      // Receive high byte (MSB c30)
    reading = reading << 8;     // Shift byte by 8 bits
    reading |= Wire.read();     // Receive middle byte (LSB c30)
    c30 = reading;
  }

  delay(100);

  // Calculate raw scaled pressure according to oversampling rate
  double_t rawScaledPrs = (double_t)rawPrs/(double_t)kP;

  double_t rawScaledTemp = getTemperature(true)/(double_t)kT;

  // Calculate actual pressure in pascal
  return c00 + rawScaledPrs*(c10 + rawScaledPrs*(c20 + rawScaledPrs*c30)) + rawScaledTemp*c01 + rawScaledTemp*rawScaledPrs*(c11 + rawScaledPrs*c21);
}

void countWindSpd()
{
  // Interrupt function for wind speed
  nbWindSpd++;
}

void rainBeDrippin()
{
  // Interrupt function for rain
  valPlu++;
}