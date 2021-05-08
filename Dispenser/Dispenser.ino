#include <avr/sleep.h>
#include <SPI.h>
#include <LoRa.h>

const int motorPin = 6;
const int buttonPin = 2;
const int unitNum = 1;
const String userId = "qRS23CdKI7QNwPp1LdtA2JaTYpI2";
const String dispenserId = "qwerty";

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("setting up");
  pinMode(3, INPUT_PULLUP);
  pinMode(2, INPUT);
  pinMode(6, OUTPUT);

  if (!LoRa.begin(433E6))
  { // or 915E6, the MHz speed of yout module
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  LoRa.setSpreadingFactor(12);
  LoRa.setTxPower(20);
  LoRa.setCodingRate4(8);
}

void loop()
{
  sleepFunction();
}

void sleepFunction()
{
  sleep_enable();
  LoRa.sleep();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // attach the interrupt on int0
  attachInterrupt(1, interruptFunction, LOW);
  attachInterrupt(0, buttonInterrupt, LOW);

  sleep_cpu();
  Serial.println("Woke up!");
}

void interruptFunction()
{
  Serial.println("Hand detected!"); //A7 is the ir transistor

  digitalWrite(motorPin, HIGH);
  for (int i=0; i < 120; i++) {
    Serial.println(i);
  }
  digitalWrite(motorPin, LOW);

  Serial.println("Sending dispenser info");
  LoRa.beginPacket();
  LoRa.print(userId);
  LoRa.print(" ");
  LoRa.print(dispenserId);
  LoRa.print(" ");
  LoRa.print("usage");
  LoRa.endPacket();
  Serial.println("Sent dispenser info");

  Serial.println(analogRead(A7));
  while (analogRead(A7) > 900)
  {
    Serial.println("Hand is too near");
    Serial.println(analogRead(A7));
  }
  Serial.println("Going to sleep");

  sleep_disable();
  detachInterrupt(1);
  LoRa.sleep();
}

void buttonInterrupt()
{
  Serial.println("Button was pressed!");
  Serial.println("Sending reset packet 1");
  Serial.println("Sending reset packet 2");

    LoRa.beginPacket();
    LoRa.print(userId);
    LoRa.print(" ");
    LoRa.print(dispenserId);
    LoRa.print(" ");
    LoRa.print("reset");
    LoRa.endPacket();  
    for (int i=0; i < 50; i++) {
      Serial.println(i);
    }
    LoRa.beginPacket();
    LoRa.print(userId);
    LoRa.print(" ");
    LoRa.print(dispenserId);
    LoRa.print(" ");
    LoRa.print("reset");
    LoRa.endPacket(); 

  Serial.println("Finished sending reset packet");

  while (digitalRead(2) == 0) {}

  sleep_disable();
  detachInterrupt(0);
  LoRa.sleep();
}
