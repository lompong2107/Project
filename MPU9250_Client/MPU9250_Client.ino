#include <Wire.h>
#include "BLEDevice.h"
#include "BLEScan.h"
#include "MPU9250.h"

MPU9250 mpu;
int count=1, chk=0;
#define _size 20
int x[_size], aX[_size], aY[_size];
float DirecX=0, DirecY=0;
String DirectionX = "0", DirectionY = "0";

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
  }
};

bool connectToServer() {
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      pClient->disconnect();
      return false;
    }
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      pClient->disconnect();
      return false;
    }
    connected = true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup()
{
  Serial.begin(115200);
  Wire.begin(17, 16); // กำหนดขาในการอ่านข้อมูลจาก MPU9250
  delay(2000);
  mpu.setup();
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop()
{
  mpu.update();
  float AcX = (int)1000 * mpu.getAcc(0);
  float AcY = (int)1000 * mpu.getAcc(1);
  float AcZ = (int)1000 * mpu.getAcc(2);
//  Serial.printf("%.2f, %.2f, %.2f, \n", AcX/100, AcY/100, AcZ/100);
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
        doConnect = false;
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
  }
  if (connected) {
      std::string value = pRemoteCharacteristic->readValue(); // รับค่าจาก ESP32 ฝั่ง Server
      if ((AcX/100 < -2 || AcX/100 > 2) || (AcY/100 < -2 || AcY/100 > 2) || chk == 1) {
//        Serial.printf("%.2f, %.2f, %.2f, \n", AcX/100, AcY/100, AcZ/100);
        chk = 1;
        if (count <= 15) {
          x[count-1] = count;
          aX[count-1] = (int)AcX/100;
          aY[count-1] = (int)AcY/100;
          count ++;
        } else {
          DirecX = regression(x, aX); // ส่งข้อมูล Array ไปคำนวณในฟังก์ชั่น และเก็บค่า
          DirecY = regression(x, aY); // ส่งข้อมูล Array ไปคำนวณในฟังก์ชั่น และเก็บค่า
          if (DirecX > 0.01){         // ตรวจสอบค่าหลังจากการคำนวณ
            DirectionX = "Left";
          } else {
            DirectionX = "Right";
          }
          if (DirecY < -0.01){
            DirectionY = "Up";
          } else {
            DirectionY = "Down";
          }
          chk = 0;
          count = 1;
        }
      }
//      Serial.println(String(DirectionX) + "/" + String(DirectionY) + "/");
      String text = String(value.c_str()) + String(DirectionX) + "/" + String(DirectionY) + "/";
      Serial.println(text); // ส่งค่าข้อมูลไปยัง BLE HM-10
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  DirectionX = "0";
  DirectionY = "0";
  delay(20);
}

float regression(int x[_size], int y[_size]){
  float r2, xy[_size], x2[_size], y2[_size];
  float sumX=0, sumY=0, sumXY=0, sumX2=0, sumY2=0, byx=0;
  for (int i=0; i<_size; i++) {
    x2[i] = x[i] * x[i];
    y2[i] = y[i] * y[i];
    xy[i] = x[i] * y[i];
    sumX = sumX + x[i];
    sumY = sumY + y[i];
    sumXY = sumXY + xy[i];
    sumX2 = sumX2 + x2[i];
    sumY2 = sumY2 + y2[i];
  }
  byx = ((_size * sumXY) - (sumX * sumY)) / ((_size * sumX2) - (sumX * sumX));
  return byx;
}
