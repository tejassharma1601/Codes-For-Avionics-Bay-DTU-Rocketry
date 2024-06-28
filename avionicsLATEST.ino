#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BME680.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define RELAY_PIN 7
#define MOSFET_PIN 8  // Define the MOSFET control pin

Adafruit_BME680 bme; // Create BME680 object
File dataFile;       // Create file object for SD card
float max = 0.0;

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);  // Set MOSFET pin as output
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(MOSFET_PIN, LOW);  // Ensure MOSFET is initially off

  if (!bme.begin()) {
    Serial.println("Could not find BME680 sensor, check wiring!");
    while (1);
  }

  Serial.println("BME680 sensor found!");

  if (!SD.begin(4)) { // Corrected to pin 4 for most SD shields
    Serial.println("SD card initialization failed!");
    while (1);
  }

  Serial.println("SD card initialized.");

  // Initialize max with the first reading if available
  if (bme.performReading()) {
    max = bme.readAltitude(SEALEVELPRESSURE_HPA);
  }
}

void loop() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;         
  }

  float temperature = bme.temperature;
  float pressure = bme.pressure / 100.0; // Convert Pa to hPa
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("Altitude: ");
  Serial.print(altitude);
  Serial.println(" meters");

  // Log data to SD card
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print("Temperature: ");
    dataFile.print(temperature);
    dataFile.println(" °C");

    dataFile.print("Pressure: ");
    dataFile.print(pressure);
    dataFile.println(" hPa");

    dataFile.print("Altitude: ");
    dataFile.print(altitude);
    dataFile.println(" meters");

    if (altitude > max) {
      max = altitude;
    } else if (altitude < max - 2) {
      Serial.println("Cracked!!");
      dataFile.println("Cracked!!");
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(MOSFET_PIN, HIGH); // Turn on the MOSFET
    } else {
      digitalWrite(MOSFET_PIN, LOW);  // Ensure MOSFET is off if no crack detected
    }

    dataFile.close();
    Serial.println("Data logged to SD card.");
  } else {
    Serial.println("Error opening datalog.txt!");
  }

  delay(1000); // Wait for a second
}

