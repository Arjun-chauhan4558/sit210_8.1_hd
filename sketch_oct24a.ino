
#include <ArduinoBLE.h>
#include <Wire.h>
#include <BH1750.h>
#include <Servo.h>

const int bathroomLED = 2;
const int hallwayLED = 3;
const int fanPin = 9;
const float darkLevel = 50.0;

BH1750 lightSensor;
Servo fan;

BLEService lightingService("19B10000-E8F2-537E-4F6C-D104768A1214");

BLEStringCharacteristic commandChar(
  "19B10001-E8F2-537E-4F6C-D104768A1214",
  BLEWrite,
  30
);

void setup() {
  Serial.begin(9600);

  pinMode(bathroomLED, OUTPUT);
  pinMode(hallwayLED, OUTPUT);

  Wire.begin();
  lightSensor.begin();

  fan.attach(fanPin);
  fan.write(0);

  if (!BLE.begin()) {
    Serial.println("BLE failed to start");
    while (true);
  }

  BLE.setLocalName("LightingNano");
  BLE.setAdvertisedService(lightingService);
  lightingService.addCharacteristic(commandChar);
  BLE.addService(lightingService);
  BLE.advertise();

  Serial.println("Arduino ready");
  Serial.println("Waiting for voice commands...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.println("Raspberry Pi connected");

    while (central.connected()) {
      if (commandChar.written()) {
        String command = commandChar.value();

        float lightLevel = lightSensor.readLightLevel();

        Serial.print("Command received: ");
        Serial.println(command);

        Serial.print("Light level: ");
        Serial.print(lightLevel);
        Serial.println(" lux");

        if (command == "BATHROOM") {
          if (lightLevel < darkLevel) {
            digitalWrite(bathroomLED, HIGH);
            Serial.println("Bathroom LED: ON (dark)");
          } else {
            digitalWrite(bathroomLED, LOW);
            Serial.println("Bathroom LED: OFF (bright)");
          }

        } else if (command == "HALLWAY") {
          if (lightLevel < darkLevel) {
            digitalWrite(hallwayLED, HIGH);
            Serial.println("Hallway LED: ON (dark)");
          } else {
            digitalWrite(hallwayLED, LOW);
            Serial.println("Hallway LED: OFF (bright)");
          }

        } else if (command == "LIGHTS_OFF") {
          digitalWrite(bathroomLED, LOW);
          digitalWrite(hallwayLED, LOW);
          Serial.println("Both LEDs OFF");

        } else if (command == "FAN_ON") {
          fan.write(90);
          Serial.println("Fan ON");

        } else if (command == "FAN_OFF") {
          fan.write(0);
          Serial.println("Fan OFF");
        }
      }
    }

    Serial.println("Raspberry Pi disconnected");
  }
}
