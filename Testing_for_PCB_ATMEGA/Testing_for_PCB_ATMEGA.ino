#include <avr/sleep.h>
#include <SPI.h>
#include <LoRa.h>

const int ledPin = 3;
const int buttonPin = 2;
const int unitNum = 1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("setting up");
  pinMode(3, INPUT_PULLUP);
  pinMode(2, INPUT);
  pinMode(6, OUTPUT);
 

  if (!LoRa.begin(433E6)) { // or 915E6, the MHz speed of yout module
    Serial.println("Starting LoRa failed!");
    while (1);
  } 

  LoRa.setSpreadingFactor(12);
  LoRa.setTxPower(20);
  LoRa.setCodingRate4(8);
}

void loop() {
  sleepFunction();
}

void sleepFunction() {
  sleep_enable();
  LoRa.sleep();
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // attach the interrupt on int0 
  attachInterrupt(1, interruptFunction, LOW );
  attachInterrupt(0, buttonInterrupt, LOW);
  
 
  sleep_cpu();
  Serial.println("Woke up!");
}

void interruptFunction() {
  Serial.println("Hand detected!");     //A7 is the ir transistor

  int i = 0;
  while(i < 100) {
    digitalWrite(6, HIGH);
    i++;
    Serial.println(i);
  }
  digitalWrite(6, LOW);


  LoRa.beginPacket();  
  LoRa.print(unitNum);
  LoRa.print(1);
  Serial.println("Sending dispensing info");
  LoRa.endPacket();  

  while (analogRead(A7) > 800) {Serial.print(".");}
  Serial.println("Going to sleep");
  
  sleep_disable();
  detachInterrupt(1);
  LoRa.sleep();
}

void buttonInterrupt() {
  Serial.println("Button was pressed!");
  Serial.println("Sending reset packet 1");
  Serial.println("Sending reset packet 2");
//  int i = 0;
//  while(i < 100) {
//    Serial.println(i);
//    digitalWrite(6, HIGH);
//    i++;
//  }
//  digitalWrite(6, LOW);
  
  LoRa.beginPacket();  
  LoRa.print(unitNum);
  LoRa.print(0);
  LoRa.endPacket();  
  delay(10);
  LoRa.beginPacket();  
  LoRa.print(unitNum);
  LoRa.print(0);
  LoRa.endPacket();
  
  while (digitalRead(2) == 0) {}
  
  sleep_disable();
  detachInterrupt(0);
  LoRa.sleep();

}
