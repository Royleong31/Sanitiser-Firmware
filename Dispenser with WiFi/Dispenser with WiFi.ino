const int motorPin = 19;
const int irSensor = 34;
const int buttonPin = 17;

const String companyId = "Fj7JOWRl8OFoUKNPzLPU";
const String dispenserId = "23456789";

#include <WiFi.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Roy's Wifi";
const char* password = "qpoe1242";
 
//const char* ssid = "3logytech_#03_2.4GHz";
//const char* ssid = "3logytech2.4";

// const char* ssid = "TP-Link_3logytech";
// const char* password = "3logytech1928";

String buffer[1000];
int APIcounter = 0;
int packetCounter = 0;

TaskHandle_t Task1;
TaskHandle_t Task2;

void resetBuffer() {
  buffer[APIcounter] = "";

  if (APIcounter < 999) {
    APIcounter++;
  } else {
    APIcounter = 0;
  }
}

void setBuffer(String typeOfUsage) {
  if (buffer[packetCounter] == "") {
    buffer[packetCounter] = typeOfUsage;  // usage/reset
  }

  // for (int i=0; i<10; i++) {
  //   Serial.println("Buffer row: " + String(i) + "      Value: " + buffer[i]);
  // }

  if (packetCounter < 999) {
    packetCounter++;
  } else {
    packetCounter = 0;
  }
}

void setup() {
  //initialize Serial Monitor and pins
  Serial.begin(9600);
  pinMode(motorPin, OUTPUT);
  pinMode(irSensor, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // CONNECT TO WIFI
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
      if (buffer[APIcounter] != "" && numOfTimesTried < 4) {
        Serial.println("Starting api send");
        const int apiStart = millis();
        HTTPClient client;
        char jsonOutput[128];
        String typeOfUsage = buffer[APIcounter];

        if (typeOfUsage == "usage") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + companyId + "/" + dispenserId + "/usage");
        //  client.begin("https://v2.jokeapi.dev/joke/Any");
        } else if (typeOfUsage == "reset") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + companyId + "/" + dispenserId + "/reset");
          // client.begin("https://api.kanye.rest/");
        }
   
        int httpCode = client.PATCH(jsonOutput);
        // int httpCode = client.GET();
         Serial.println("Time that posting API needs to execute: " + String(millis() - apiStart));
        
        if (httpCode == 200) {
          String payload = client.getString();
          Serial.println("\nStatuscode: " + String(httpCode));
          Serial.println(payload);
          client.end();
          numOfTimesTried = 0;
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

void Task2code( void * pvParameters ) {
  while (true) {
    // Serial.println("Sensor Reading: " + String(analogRead(irSensor)));
    // Serial.println("Button Reading: " + String(digitalRead(buttonPin)));
    // delay(1000);

    // Button is pressed when digitalRead(buttonPin) == 0
    if (digitalRead(buttonPin) == 0) {
      setBuffer("reset");
      Serial.println("Reset button was pressed");
      while(digitalRead(buttonPin) == 0) {}
      delay(500);
    }

    delay(100);
    if (analogRead(irSensor) < 3000) {
      digitalWrite(motorPin, HIGH);
      delay(300);
      digitalWrite(motorPin, LOW);

      setBuffer("usage");

      while(analogRead(irSensor) < 2800) {
        Serial.println("Hand is too close");
        Serial.println(analogRead(irSensor));
        delay(100);
      }
    }
    else {
      digitalWrite(motorPin, LOW);
    }
  }
}

void loop() {
   vTaskDelete(NULL);
}
