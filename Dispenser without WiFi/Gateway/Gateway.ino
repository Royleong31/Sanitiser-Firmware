#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>

//define the pins used by the transceiver module        FOR ESP32 ONLY
#define ss 5
#define rst 14
#define dio0 2
 
// Insert WiFi name and password
const char* ssid = "";
const char* password = "";

// 2D array that stores LoRa packets from dispensers
// Each 'row' (the one with [1000]) stores the information from each packet
// Each 'column' (the one with [3]) stores the information from each packet
// As each packet has 3 types of information (companyId, dispenserId, typeOfUsage), this allows the system to store data from 1000 packets
// The buffer functions as a queue, with incoming requests starting from the back of the queue
String buffer[1000][3];

// APIcounter stores the position of the current API request
int APIcounter = 0;

// packetCounter stores the position of the spot that will be used for the incoming LoRa packet
int packetCounter = 0;

// APIcounter will 'follow' packetCounter as the packetCounter adds data into the buffer(as data is received from dispensers) and APIcounter removes data from the buffer(as API requests are made successfully)

TaskHandle_t Task1;
TaskHandle_t Task2;

  // Restarts the counter when the counter reaches the end of the queue
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

  // Setting the spreading factor and coding rate of LoRa helps to increase the range (look at <LoRa.h> library docs on github for more details)
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(8);

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
  // Details are similar to the Dispenser with WiFi file
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
        } else if (typeOfUsage == "reset") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + userId + '/' + dispenserId + "/reset");
        }
   
        int httpCode = client.PATCH(jsonOutput);
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
    // Variables to store the data from incoming LoRa packets
    String LoRaData;
    // Array of 3 strings as the LoRa packet has 3 types of info (companyId, dispenserId, typeOfUsage)
    String currentPacketData[3] = {"", "", ""};

    //try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      //received a packet
      Serial.print("Received packet ");
  
      //read packet
      while (LoRa.available()) {
        // add the LoraData to the buffer array
        LoRaData = LoRa.readString();
        Serial.print(LoRaData);
      }
  
      //print RSSI of packet
      int rssi = LoRa.packetRssi();
      Serial.print(" with RSSI ");
      Serial.println(rssi);

      int arrayCounter = 0;
      
      for (int i=0; i<LoRaData.length(); i++) {
        // Write the data into the currentPacketData array        
        if (LoRaData[i] != ' ') {
          currentPacketData[arrayCounter] += LoRaData[i];

        // Each component in the LoRaData is separated by a space(' '), so this helps to seperate the data into the 3 elements in currentPacketData
        } else {
          arrayCounter++;
        }
      }

      // Prints out the data from the incoming packet
      for (int i=0; i<3; i++) {
        Serial.print(currentPacketData[i]);
        Serial.println("|");
      }
  
  //    ADD THE DATA TO THE BUFFER
      if (buffer[packetCounter][0] == "") {
        buffer[packetCounter][0] = currentPacketData[0];      // companyId
        buffer[packetCounter][1] = currentPacketData[1];      // dispenserId
        buffer[packetCounter][2] = currentPacketData[2];      // usage/reset
      }

      // Restarts the counter when the counter reaches the end of the queue
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
