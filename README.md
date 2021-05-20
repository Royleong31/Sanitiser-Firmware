General Info about Hand Sanitiser System
- The purpose is to solve the problem of cleaning staff not knowing when the refills of their hand sanitiser dispensers are running out.
- This system aims to alert the users when their dispensers are running out.
- The app will send alert notifications to users to alert them (more information about the hand sanitiser can be found in the hand sanitiser app folder README)
- There are 2 versions of this system: 
    1) Dispenser with WiFi (this requires the dispenser to be connected to WiFi and plugged into a power supply)
    2) Dispenser without WiFi (this requires the gateway to be connected to WiFi and power supply, but the dispenser is battery operated and does not require a WiFi connection)


Dispenser without WiFi
- The gateway and dispenser communicate via LoRa, a long range radio protocol.
- Packets are only sent from the dispenser to the gateway (dispenser functions as a beacon, beaming information to the gateway, but the gateway does not reply)
- There are 2 files inside this folder (Gateway and Dispenser)

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
    - The low signal from the comparator will go to interrupt pin 1 of ATMEGA chip, waking it up from deep sleep and dispensing hand sanitiser and send the LoRa signal before going back to sleep.
    - In order to dispense hand sanitiser, the motorPin of the ATMEGA chip will be on for about 150ms, this turns on the MOSFET that is connected to the motor. 
    - After dispensing, the ATMEGA chip will go back to sleep
    
    ~ 
    
        
