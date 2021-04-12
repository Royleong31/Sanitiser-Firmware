#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>

//define the pins used by the transceiver module        FOR ESP32 ONLY
#define ss 5
#define rst 14
#define dio0 2

String LoRaData;
int time1, time2;
bool dispensed = false;
// const char* ssid = "Roy's Wifi";
// const char* password = "qpoe1242";
 
//const char* ssid = "3logytech_#03_2.4GHz";
//const char* ssid = "3logytech2.4";
const char* ssid = "TP-Link_3logytech";
const char* password = "3logytech1928";

char jsonOutput[128];
int buffer[1000][2];
int counter;
int APIcounter = 0;
int packetCounter = 0;
int unitNum; //Controls which unit the api request is sent to


TaskHandle_t Task1;
TaskHandle_t Task2;

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Receiver");
 
  //setup LoRa transceiver module       FOR ESP32 ONLY
  LoRa.setPins(ss, rst, dio0);
  
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSpreadingFactor(12);
//  LoRa.setSignalBandwidth(41.7E3);
  LoRa.setCodingRate4(8);

  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
//  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}



void Task1code( void * pvParameters ){
  for(;;){
   if (WiFi.status() == WL_CONNECTED) {
      if (buffer[APIcounter][0] > 0) {
       time1 = millis();
        HTTPClient client;
        int unitNum = buffer[APIcounter][0];

        if (buffer[APIcounter][1] == 2) {
          client.begin("https://aqueous-hamlet-30482.herokuapp.com/data/" + String(unitNum));
        } else if (buffer[APIcounter][1] == 1) {
          client.begin("https://aqueous-hamlet-30482.herokuapp.com/reset/" + String(unitNum));
        }
   
        int httpCode = client.PATCH(jsonOutput);
         time2 = millis();
         Serial.println("Time that posting API needs to execute: " + String(time2-time1));
        
        if (httpCode == 200) {
          String payload = client.getString();
          Serial.println("\nStatuscode: " + String(httpCode));
          Serial.println(payload);
  
          client.end();
          buffer[APIcounter][0] = buffer[APIcounter][1] = 0;

          if (APIcounter < 999) {
            APIcounter++;
          } else {
            APIcounter = 0;
          }
          
        } else {
          Serial.println("Error on HTTP request");
          Serial.println("Error Code " + String(httpCode));
        }
      } 
    }
      else {
        Serial.println("Connection lost"); 
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        delay(3000);
       if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Device reconnected to WiFi");
       }
       }
      delay(100);
    } 
  } 

void Task2code( void * pvParameters ){
  for(;;){

 //try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      //received a packet
      Serial.print("Received packet ");
  
      //read packet
      while (LoRa.available()) {
        LoRaData = LoRa.readString();
        Serial.print(LoRaData);
  
        // add the LoraData to the buffer array
      }
  
      //print RSSI of packet
      int rssi = LoRa.packetRssi();
      Serial.print(" with RSSI ");
      Serial.println(rssi);
  
      int dispensing;
      int loraLength = LoRaData.length();
      int unit = LoRaData.substring(0, loraLength - 1).toInt();

      if (LoRaData[loraLength - 1] == '1') {
        dispensing = 2;                                       // dispensing == 2 means that it has been dispensed
      } else {      
      dispensing = 1;                                         // dispensing == 1 means that is has been reset
      }
  
  //    ADD THE DATA TO THE BUFFER
      if (buffer[packetCounter][0] == 0) {
        buffer[packetCounter][0] = unit;
        buffer[packetCounter][1] = dispensing;
      }
  
      for (int i=0; i<10; i++) {
        Serial.print(buffer[i][0]);
        Serial.print("     ");
        Serial.println(buffer[i][1]);
      }

      if (packetCounter < 999) {
        packetCounter++;
      } else {
        packetCounter = 0;
      }
    }
  }
}

void loop() {
   vTaskDelete(NULL);
}










//#include <SPI.h>
//#include <LoRa.h>
//
////define the pins used by the transceiver module        FOR ESP32 ONLY
//#define ss 5
//#define rst 14
//#define dio0 2
//
//void setup() {
//  //initialize Serial Monitor
//  Serial.begin(9600);
//  while (!Serial);
//  Serial.println("LoRa Receiver");
// 
//  //setup LoRa transceiver module       FOR ESP32 ONLY
//  LoRa.setPins(ss, rst, dio0);
//  
//  //replace the LoRa.begin(---E-) argument with your location's frequency 
//  //433E6 for Asia
//  //866E6 for Europe
//  //915E6 for North America
//  while (!LoRa.begin(433E6)) {
//    Serial.println(".");
//    delay(500);
//  }
//   // Change sync word (0xF3) to match the receiver
//  // The sync word assures you don't get LoRa messages from other LoRa transceivers
//  // ranges from 0-0xFF
////  LoRa.setSyncWord(0xF3);
//  Serial.println("LoRa Initializing OK!");
//}
//
//void loop() {
//  // try to parse packet
//  int packetSize = LoRa.parsePacket();
//  if (packetSize) {
//    // received a packet
//    Serial.print("Received packet '");
//
//    // read packet
//    while (LoRa.available()) {
//      String LoRaData = LoRa.readString();
//      Serial.print(LoRaData); 
//    }
//
//    // print RSSI of packet
//    Serial.print("' with RSSI ");
//    Serial.println(LoRa.packetRssi());
//  }
//}
