#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h> 
#include <HTTPClient.h>
//#include <HttpClient.h>
#include <ArduinoJson.h>

//define the pins used by the transceiver module        FOR ESP32 ONLY
#define ss 5
#define rst 14
#define dio0 2

// const char* ssid = "Roy's Wifi";
// const char* password = "qpoe1242";
 
//const char* ssid = "3logytech_#03_2.4GHz";
//const char* ssid = "3logytech2.4";

const char* ssid = "TP-Link_3logytech";
const char* password = "3logytech1928";

String buffer[1000][3];
int APIcounter = 0;
int packetCounter = 0;

TaskHandle_t Task1;
TaskHandle_t Task2;

void resetBuffer() {
  buffer[APIcounter][0] = buffer[APIcounter][1] = buffer[APIcounter][2] = "";

  if (APIcounter < 999) {
    APIcounter++;
  } else {
    APIcounter = 0;
  }
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Receiver");
 
  //setup LoRa transceiver module       FOR ESP32 ONLY
  LoRa.setPins(ss, rst, dio0);
  
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    (500);
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
                    Task1code, 
                    "Task1",
                    10000,      
                    NULL,      
                    1,          
                    &Task1,      
                    0);                   
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   
                    "Task2",     
                    10000,       
                    NULL,        
                    1,           
                    &Task2,      
                    1);          
    delay(500); 
}



void Task1code( void * pvParameters ){
  int numOfTimesTried = 0;
  while (true) {
   if (WiFi.status() == WL_CONNECTED) {
      if (buffer[APIcounter][0] != "" && numOfTimesTried < 4) {
        Serial.println("Starting api send");
        const int apiStart = millis();
        HTTPClient client;
        char jsonOutput[128];
        String userId = buffer[APIcounter][0];
        String dispenserId = buffer[APIcounter][1];
        String typeOfUsage = buffer[APIcounter][2];

        if (typeOfUsage == "usage") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + userId + '/' + dispenserId + "/usage");
//          client.begin("https://v2.jokeapi.dev/joke/Any");
        } else if (typeOfUsage == "reset") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + userId + '/' + dispenserId + "/reset");
        }
   
        int httpCode = client.PATCH(jsonOutput);
//         int httpCode = client.GET();
         Serial.println("Time that posting API needs to execute: " + String(millis() - apiStart));
        
        if (httpCode == 200) {
          String payload = client.getString();
          Serial.println("\nStatuscode: " + String(httpCode));
          Serial.println(payload);
          client.end();

          resetBuffer();
          
          
        } else {
          numOfTimesTried++;
          Serial.println("Error on HTTP request");
          Serial.println("Error Code " + String(httpCode));
        } 
      } else if (numOfTimesTried >= 4) {
        numOfTimesTried = 0;
        resetBuffer();
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
  while (true) {
    String LoRaData;
    String currentPacketData[3] = {"", "", ""};

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

      int arrayCounter = 0;
      
      for (int i=0; i<LoRaData.length(); i++) {
        if (LoRaData[i] != ' ') {
          currentPacketData[arrayCounter] += LoRaData[i];
        } else {
          arrayCounter++;
        }

      }


      for (int i=0; i<3; i++) {
        Serial.print(currentPacketData[i]);
        Serial.println("|");
      }
  
  //    ADD THE DATA TO THE BUFFER
      if (buffer[packetCounter][0] == "") {
        buffer[packetCounter][0] = currentPacketData[0];      // userId
        buffer[packetCounter][1] = currentPacketData[1];      // dispenserId
        buffer[packetCounter][2] = currentPacketData[2];      // usage/reset
      }
  
//      for (int i=0; i<10; i++) {
//        Serial.print("Buffer row" + String(i) + "      ");
//        Serial.print(buffer[i][0]);
//        Serial.print("     ");
//        Serial.print(buffer[i][1]);
//        Serial.print("     ");
//        Serial.println(buffer[i][2]);
//      }

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
