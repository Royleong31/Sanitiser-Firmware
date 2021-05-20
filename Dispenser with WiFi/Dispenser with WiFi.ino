const int motorPin = 19;
const int irSensor = 34;
const int buttonPin = 17;

const String companyId = "Fj7JOWRl8OFoUKNPzLPU";
const String dispenserId = "23456789";

#include <WiFi.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>
 
// Insert WiFi name and password
const char* ssid = ""; 
const char* password = "";

// This buffer stores reset/usage data regarding the dispenser.
// It functions as a queue data structure, with the API of the oldest usage sent before the data of newer usages. (First in, first out principle)
String buffer[1000];

// APIcounter stores the position of the current API request
int APIcounter = 0;
// packetCounter stores the position of the spot that will be used for the incoming LoRa packet
int packetCounter = 0;

TaskHandle_t Task1;
TaskHandle_t Task2;

// Function is to reset the buffer after it is used
void resetBuffer() {
  buffer[APIcounter] = "";

  if (APIcounter < 999) {
    APIcounter++;
  } else {
    APIcounter = 0;
  }
}

// Insert info regarding the type of usage (reset/usage) into the buffer array
void setBuffer(String typeOfUsage) {
  if (buffer[packetCounter] == "") {
    buffer[packetCounter] = typeOfUsage;
  }

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

  // This indicates the number of times the api request has tried and failed. If it exceeds 4, stop trying to send the request, and instead skip to the next request
  int numOfTimesTried = 0;
  while (true) {
  //  If connected to wifi
   if (WiFi.status() == WL_CONNECTED) {
      // If there is an api request in the queue, and the same request has not failed for more than 4 times,
      if (buffer[APIcounter] != "" && numOfTimesTried < 4) {
        Serial.println("Starting api send");

        // Count the amount of time it takes to send the api request
        const int apiStart = millis();
        HTTPClient client;
        char jsonOutput[128];
        String typeOfUsage = buffer[APIcounter];

        // 2 types of API request, usage(when dispenser was used), and reset(when dispenser was reset with the use of the button)
        if (typeOfUsage == "usage") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + companyId + "/" + dispenserId + "/usage");
        } else if (typeOfUsage == "reset") {
          client.begin("https://us-central1-hand-sanitiser-c33d1.cloudfunctions.net/usage/" + companyId + "/" + dispenserId + "/reset");
        }
   
        int httpCode = client.PATCH(jsonOutput); // Request type is patch (look at API documentation)
         Serial.println("Time that posting API needs to execute: " + String(millis() - apiStart));
        
        // If API request is successful
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
        
        // If the API request has failed for more than 4 times, move on to the next request
      } else if (numOfTimesTried >= 4) {
        numOfTimesTried = 0;
        resetBuffer();
      }

    } else {
        // If connection is lost, attempt to reconnect to network      
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
    // When reset button is pressed, the value of digitalRead(buttonPin) == 0
    if (digitalRead(buttonPin) == 0) {
      setBuffer("reset");
      Serial.println("Reset button was pressed");
      while(digitalRead(buttonPin) == 0) {}
      delay(500);
    }

    delay(100);

    // Turn on the motor to dispense the hand sanitiser solution
    if (analogRead(irSensor) < 3200) {
      digitalWrite(motorPin, HIGH);
      delay(300);
      digitalWrite(motorPin, LOW);

      setBuffer("usage");

      // If hand is too close, dont dispense again (this is to prevent double dispensing)
      while(analogRead(irSensor) < 3200) {
        Serial.println("Hand is too close");
        Serial.println(analogRead(irSensor));
        delay(100);
      }
      delay(800);
    }
    else {
      digitalWrite(motorPin, LOW);
    }
  }
}

void loop() {
   vTaskDelete(NULL);
}
