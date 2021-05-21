## General Info about Hand Sanitiser System
- The purpose is to solve the problem of cleaning staff not knowing when the refills of their hand sanitiser dispensers are running out.
- This system aims to alert the users when their dispensers are running out.
- The app will send alert notifications to users to alert them (more information about the hand sanitiser can be found in the hand sanitiser app folder README)
- The app and the firmware for the dispensers and gateway are connected by the Hand Sanitiser API (more info can be found in that folder)
- There are 2 versions of this system: 
    1) Dispenser with WiFi (this requires the dispenser to be connected to WiFi and plugged into a power supply)
    2) Dispenser without WiFi (this requires the gateway to be connected to WiFi and power supply, but the dispenser is battery operated and does not require a WiFi connection)
- The buffers in the Gateway and Dispenser with WiFi function as circular queues, with the data to be sent being queued and when the data is sent by the API successfully, the data is dequeued (removed from the queue)
- Queues are a form of data structure that follow the First In First Out principle

---------------------------------------------------------------------------------

Dispenser without WiFi
- The gateway and dispenser communicate via LoRa, a long range radio protocol.
- Packets are only sent from the dispenser to the gateway (dispenser functions as a beacon, beaming information to the gateway, but the gateway does not reply)
- There are 2 files inside this folder (Gateway and Dispenser)
- LoRa packet is a string, consisting of 3 segments of information, each separated by a space. The first segment is the companyId, second segment is the dispenserId, third is the type of usage.

1) Dispenser
    - A voltage of 3.3V-3.6V is used as that is the required voltage for the MOSFET to work properly (MOSFET is unreliable when GPIO voltage is too low).
    - The dispenser is programmed using an FTDI adapter that is connected to the RX, TX, DTR pins of the PCB. While programming the chip, the chip needs to be powered by the 3.3V and ground of the FTDI.
    
    ~ Electronics
    - Uses a custom PCB with ATMEGA328P chip, runs on 4 AA batteries
    - The input voltage (6V) is converted to 3.3V-3.6V via a DC-DC converter or a LDO regulator 
    - A voltage of 3.3V-3.6V is used as that is the required voltage for the MOSFET to work properly (MOSFET is unreliable when GPIO voltage is too low).
    - The DC-DC converter or LDO will output 3.3V-3.6V into the chip and LoRa module. The maximum GPIO voltage of the chip to be the same as the input voltage of the chip
    - IR sensor:
        a) IR led: Connected to resistor in series to ground and the 3.3V-3.6V supply from the DC-DC converter
        b) IR phototransistor: Connected to a high resistance pull up resistor. Voltage in between resistor and phototransistor is connected to an analog pin of the ATMEGA as well as a pin on the comparator
    - Motor pins:
        a) Positive end is connected to the AA batteries
        b) Negative end is connected to the drain of the motor
    - MOSFET pins: 
        a) Gate leg is connected to GPIO pin of ATMEGA
        b) Drain is connected to the negative pin of the motor
        c) Source is connected to ground
    
    ~ Dispensing procedure
    - When hand is placed below the IR sensor, the voltage across the IR phototransistor will vary. This produces an analog voltage signal that is converted to a digital signal by the comparator.
    - A 500k ohm potentiometer is connected to the comparator to produce a reference voltage which the incoming voltage of the IR phototransistor will compare to. A low signal will be produced if the voltage is lower than the reference voltage, signalling that a hand was detected by the IR sensor.
    - The low signal from the comparator will go to interrupt pin 1 of ATMEGA chip, waking it up from deep sleep and dispensing hand sanitiser and send the LoRa packet before going back to sleep.
    - LoRa packet: companyId + ' ' + dispenserId + ' ' + 'usage'
    - In order to dispense hand sanitiser, the motorPin of the ATMEGA chip will be on for about 150ms, this turns on the MOSFET that is connected to the motor. 
    - After dispensing, the ATMEGA chip will go back to sleep
    
    ~ Resetting procedure
    - When the reset button is pressed, a low signal will be sent to pin 0 of the ATMEGA chip, waking it up.
    - The chip will then send 2 LoRa packets to the gateway, informing it that the dispenser has been reset.
    - LoRa packet: companyId + ' ' + dispenserId + ' ' + 'reset'
    - When the button is released, the chip will go back to sleep.
    
 2) Gateway
    - The gateway uses an ESP32 board as the microcontroller
    - It consists only of an ESP32 connected to a LoRa module.
    - Connected via a USB cable to a wall socket to power the gateway
    - ESP32 has 2 cores. In my code, both cores are running simultaneously, with 1 core handling the receiving of LoRa packets and the other core handling the sending of APIs to the server.
    - Both cores need to be used as the API takes about 2-4s to complete and this can block code execution as Arduino code runs synchronously. By splitting the different tasks into different cores, the ESP32 can both receive packets and send APIs concurrently
    
    ~ Upon receiving a LoRa packet 
    - Store the company ID, dispenser ID and type of usage (usage/reset) into the buffer, which is a 2D array.
    - Position of the counter is stored in a variable, and will increase every time a new packet is received
    
    ~ Procedure of sending APIs to the server
    - Using the company ID, dispenser ID and type of usage, an API is sent to the server to update it that the dispenser has been used/reset.
    - If the API fails, the chip will attempt to send 3 more times before giving up and moving on to the next packet.
    - If the API request is successful, the element in the array will be cleared, allowing data to be written into the same spot.
    
---------------------------------------------------------------------------------

Dispenser with WiFi
- Uses ESP32 board
- The dispenser will be connected to WiFi and plugged into a wall power supply.
- Both of the cores on the ESP32 are used simultaneously, 1 for detecting hand near the IR sensor and another for sending APIs to the server.
- It is similar to the system without WiFi, just without the LoRa part. 
- There is no PCB yet, just an ESP32, resistors, MOSFET and motor mounted on a donut board.
- As the dispenser and company IDs are unique to the dispenser, the buffer array is only 1 dimensional, instead of 2 dimensional like in the gateway above. (dispenserId and companyId are stored as global variables in the code)

~ Upon detecting a hand
- The string 'usage' will be inserted into the buffer array. 
- The counter that stores the position of the next available slot in the buffer will increase.

~ Upon reset button being pressed
- The string 'reset' will be inserted into the buffer array. 
- The counter that stores the position of the next available slot in the buffer will increase.

~ Procedure of sending APIs to the server
- When there is a packet of data to be sent, the API will send the data with the companyId, dispenserId and the type of usage.
- API counter, which stores the position of the data of the last sent API, will increase.
