// APP2 S6 - Remise : 2024/05/24
// William Pépin  - PEPW3101
// Gabriel Vachon - VACG3501
// Description : code du client

#include "BLEDevice.h"
//#include "BLEScan.h"

// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID_hum("76ca544c-b8c2-404b-a88a-d6aa312d4d9d");
static BLEUUID charUUID_temp("341019a0-c3d6-40ec-baee-aeb8127031c8");
static BLEUUID charUUID_prs("2a7f3b8d-3583-4e14-b770-79ddaf2c1674");
static BLEUUID charUUID_lum("004f0e25-5c29-4d34-ad9c-ca73bade9bdc");
static BLEUUID charUUID_ws("a6ed8fd2-9c68-4c2a-8de5-ac0aa9b3446e");
static BLEUUID charUUID_wd("e9a27706-1d6d-42ed-a2e2-0119d49a1b50");
static BLEUUID charUUID_rain("1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e");
//static BLEUUID    charUUID_2("1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e");

// Some variables to keep track on device connected
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

// Define pointer for the BLE connection
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pCharHUM;
BLERemoteCharacteristic* pCharTEMP;
BLERemoteCharacteristic* pCharPRS;
BLERemoteCharacteristic* pCharLUM;
BLERemoteCharacteristic* pCharWS;
BLERemoteCharacteristic* pCharWD;
BLERemoteCharacteristic* pCharRain;
//BLERemoteCharacteristic* pRemoteChar_2;

// Callback function for Notify function
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                           uint8_t* pData,
                           size_t length,
                           bool isNotify) {
  //if(pBLERemoteCharacteristic->getUUID().toString() == charUUID_hum.toString()) {
  std::string uuid = pBLERemoteCharacteristic->getUUID().toString();
  
  Serial.print("Station - ");
  
  if (uuid == charUUID_hum.toString()) {
    Serial.print("Humidité (%) : ");
  }
  else if (uuid == charUUID_temp.toString()) {
    Serial.print("Température (Celcius) : ");
  }
  else if (uuid == charUUID_lum.toString()) {
    Serial.print("Luminosité (Lux) : ");
  }
  else if (uuid == charUUID_prs.toString()) {
    Serial.print("Pression (Pascal) : ");
  }
  else if (uuid == charUUID_rain.toString()) {
    Serial.print("Pluie (mm) : ");
  }
  else if (uuid == charUUID_wd.toString()) {
    Serial.print("Direction du vent (Deg) : ");
  }
  else if (uuid == charUUID_ws.toString()) {
    Serial.print("Vitesse du vent (km/h) : ");
  }

  double_t value;
  memcpy(&value, pData, length);

  Serial.println(value);
  //}
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// Function that is run whenever the server is connected
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  connected = true;
  pCharHUM = pRemoteService->getCharacteristic(charUUID_hum);
  pCharTEMP = pRemoteService->getCharacteristic(charUUID_temp);
  pCharPRS = pRemoteService->getCharacteristic(charUUID_prs);
  pCharLUM = pRemoteService->getCharacteristic(charUUID_lum);
  pCharWS = pRemoteService->getCharacteristic(charUUID_ws);
  pCharWD = pRemoteService->getCharacteristic(charUUID_wd);
  pCharRain = pRemoteService->getCharacteristic(charUUID_rain);
  if (connectCharacteristic(pRemoteService, pCharHUM) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pCharTEMP) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pCharPRS) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pCharLUM) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pCharWS) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pCharWD) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pCharRain) == false)
    connected = false;

  if (connected == false) {
    pClient->disconnect();
    Serial.println("At least one characteristic UUID not found");
    return false;
  }
  registerNotify(pRemoteService, pCharHUM);
  registerNotify(pRemoteService, pCharTEMP);
  registerNotify(pRemoteService, pCharPRS);
  registerNotify(pRemoteService, pCharLUM);
  registerNotify(pRemoteService, pCharWS);
  registerNotify(pRemoteService, pCharWD);
  registerNotify(pRemoteService, pCharRain);
  return true;
}

// Function to chech Characteristic
bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  if (l_BLERemoteChar == nullptr) {
    Serial.print("Failed to find one of the characteristics");
    Serial.print(l_BLERemoteChar->getUUID().toString().c_str());
    return false;
  }
  Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));
  return true;
}

void registerNotify(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  if (l_BLERemoteChar->canNotify()) {
    Serial.println(l_BLERemoteChar->canNotify());
    l_BLERemoteChar->registerForNotify(notifyCallback);
  }
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  //Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}  // End of setup.

void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // std::string rxValue = pRemoteChar_2->readValue();
    // Serial.print("Characteristic 2 (readValue): ");
    // Serial.println(rxValue.c_str());

    // String txValue = "String with random value from client: " + String(-random(1000));
    // Serial.println("Characteristic 2 (writeValue): " + txValue);

    // // Set the characteristic's value to be the array of bytes that is actually a string.
    // pRemoteChar_2->writeValue(txValue.c_str(), txValue.length());

  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  // In this example "delay" is used to delay with one second. This is of course a very basic
  // implementation to keep things simple. I recommend to use millis() for any production code
  delay(1000);
}