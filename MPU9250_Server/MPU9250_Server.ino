#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include "MPU9250.h"

MPU9250 mpu;
int count=1, chk=0;
#define _size 15
int x[_size], aX[_size], aY[_size];
float DirecX=0, DirecY=0;
String DirectionX = "0",DirectionY = "0";

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyServerCallbacks: public BLEServerCallbacks {
   void onConnect(BLEServer* pServer) {
     deviceConnected = true;
   };

   void onDisconnect(BLEServer* pServer) {
     deviceConnected = false;
   }
};

void setup()
{
  Serial.begin(115200);
  Wire.begin(17, 16);
  delay(2000);
  mpu.setup();
  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  mpu.update();
  float AcX = (int)1000 * mpu.getAcc(0);
  float AcY = (int)1000 * mpu.getAcc(1);
  float AcZ = (int)1000 * mpu.getAcc(2);
  //Serial.printf("%.2f, %.2f, %.2f, \n", AcX/100, AcY/100, AcZ/100);
  if (deviceConnected) {
    if ((AcX/100 < 1 || AcX/100 > 5) || (AcY/100 < -2 || AcY/100 > 2) || chk == 1) {
    chk = 1;
    if (count <= 15) {
      x[count-1] = count;
      aX[count-1] = (int)AcX/100;
      aY[count-1] = (int)AcY/100;
      count ++;
      //Serial.printf("%.2f, %.2f, %.2f, \n", AcX/100, AcY/100, AcZ/100);
    } else {
      DirecX = regression(x, aX);
      DirecY = regression(x, aY);
      if (DirecX > 0.01){
        DirectionX = "Left";
      } else {
        DirectionX = "Right";
      }
      if (DirecY < -0.01){
        DirectionY = "Up";
      } else {
        DirectionY = "Down";
      }
      Serial.println(DirectionX+"/"+DirectionY+"/");
      chk = 0;
      count = 1;
    }
  }
    char send_Data[30];
    snprintf(send_Data, sizeof(send_Data), "%s/%s/", DirectionX, DirectionY); //ใส่สตริงให้ตัวแปร ตามรูปแบบที่กำหนด
    pCharacteristic->setValue(send_Data); 
    pCharacteristic->notify(); //  เมื่อมีการเปลี่ยนแปลงข้อมูลจะส่งข้อมูลที่เปลี่ยนแปลงมาให้ปลายทางด้วยในที่นี้คือ ESP32 ฝั่ง Client
  }
  //Serial.println(DirectionX+"/"+DirectionY+"/");
  DirectionX = "0";
  DirectionY = "0";
  delay(40);
}
// การคำนวณ Linear Regression
float regression(int x[_size], int y[_size]){
  float r2, xy[_size], x2[_size], y2[_size];
  float sumX=0, sumY=0, sumXY=0, sumX2=0, sumY2=0, bxy=0, byx=0;
  for (int i=0; i<_size; i++) {
    x2[i] = x[i] * x[i]; // x กำลังสอง
    y2[i] = y[i] * y[i]; // y กำลังสอง
    xy[i] = x[i] * y[i]; // x คูณ y
    sumX = sumX + x[i];  // ผลบวกของ x
    sumY = sumY + y[i];  // ผลบวกของ y
    sumXY = sumXY + xy[i]; // ผลบวกของ x คูณ y
    sumX2 = sumX2 + x2[i]; // ผลบวกของ x กำลังสอง
    sumY2 = sumY2 + y2[i]; // ผลบวกของ y กำลังสอง
  }
  // การคำนวณหาค่า
  byx = ((_size * sumXY) - (sumX * sumY)) / ((_size * sumX2) - (sumX * sumX));
  return byx;
}
