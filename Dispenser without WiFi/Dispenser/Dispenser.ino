/* 
The chip sleeps by default in order to save power. There are 2 interrupt pins, pin 1 and pin 0 on the chip, which are used to wake the chip up.
Pin 1 is used by the IR sensor, and will interrupt the chip when it is low. 
- A comparator is used to convert the analog signal from the IR phototransistor to a digital signal, which is then used to wake the chip up.

Pin 0 is used by the button. A pull down resistor is attached to the button in order to wake up the chip when the button is pressed. 
- Button pressed -> pin 0 is pulled low -> chip wakes up
*/


#include <avr/sleep.h>
#include <SPI.h>
#include <LoRa.h>

const int motorPin = 6;
const int buttonPin = 2;
const int unitNum = 1;

const String companyId = "Fj7JOWRl8OFoUKNPzLPU";
const String dispenserId = "23456789";

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("setting up");
  pinMode(3, INPUT_PULLUP);
  pinMode(2, INPUT);
  pinMode(6, OUTPUT);

  // Frequency of the LoRa module is 433MHz
  if (!LoRa.begin(433E6))
  { 
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Needs to match the settings on the gateway
  LoRa.setSpreadingFactor(12);
  LoRa.setTxPower(20);
  LoRa.setCodingRate4(8);
}

void loop() {
  sleepFunction();
}

void sleepFunction()
{
  sleep_enable();
  LoRa.sleep();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // attach the interrupt on int0
  attachInterrupt(1, interruptFunction, LOW); // IR sensor to detect hand
  attachInterrupt(0, buttonInterrupt, LOW);   // Button to detect reset

  sleep_cpu();
  Serial.println("Woke up!");
}

// Dispensing hand sanitiser function
void interruptFunction()
{
  Serial.println("Hand detected!"); //A7 is the ir transistor

  // turn on the motor
  digitalWrite(motorPin, HIGH);

  // I used a for loop to create a delay as the chip's Real Time Clock(RTC) is put to sleep so delay() does not work.
  for (int i=0; i < 120; i++) {
    Serial.println(i);
  }
  digitalWrite(motorPin, LOW);

  Serial.println("Sending dispenser info");

  // Sending the LoRa packet to the gateway 
  // Packet Structure: companyId + ' ' + dispenserId + ' ' + "usage"/"reset"
  LoRa.beginPacket();
  LoRa.print(companyId);
  LoRa.print(" ");
  LoRa.print(dispenserId);
  LoRa.print(" ");
  LoRa.print("usage");
  LoRa.endPacket();
  Serial.println("Sent dispenser info");

  Serial.println(analogRead(A7));

  // White the hand is too near, run through the while loop so that the hand sanitiser will not be dispensed twice.
  while (analogRead(A7) > 900)
  {
    Serial.println("Hand is too near");
    Serial.println(analogRead(A7));
  }
  Serial.println("Going to sleep");

  // Put the chip back to sleep after dispensing
  sleep_disable();
  detachInterrupt(1);
  LoRa.sleep();
}

// Button pressed to reset function
void buttonInterrupt()
{
  Serial.println("Button was pressed!");
  Serial.println("Sending reset packet 1");
  Serial.println("Sending reset packet 2");

    // 2 packets are sent to gateway when resetting in order to increase the chance that the gateway will receive the packet (minimise risk of packet loss)
    LoRa.beginPacket();
    LoRa.print(companyId);
    LoRa.print(" ");
    LoRa.print(dispenserId);
    LoRa.print(" ");
    LoRa.print("reset");
    LoRa.endPacket();  
    for (int i=0; i < 50; i++) {
      Serial.println(i);
    }
    LoRa.beginPacket();
    LoRa.print(companyId);
    LoRa.print(" ");
    LoRa.print(dispenserId);
    LoRa.print(" ");
    LoRa.print("reset");
    LoRa.endPacket(); 

  Serial.println("Finished sending reset packet");

  // While button is pressed, don't do anything else
  while (digitalRead(2) == 0) {}

  // Put the chip back to sleep 
  sleep_disable();
  detachInterrupt(0);
  LoRa.sleep();
}
