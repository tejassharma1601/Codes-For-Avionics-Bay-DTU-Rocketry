#include <Wire.h>
#include <Adafruit_BME680.h>

// Pin for MOSFET control
const int MOSFET_PIN = 6;

// Create object for the BME680 sensor
Adafruit_BME680 bme;

// Define sea level pressure for altitude calculation
#define SEALEVELPRESSURE_HPA (1013.25)

// Variables to track altitude and cracking state
float maxAltitude = 0;

void setup() {
  pinMode(MOSFET_PIN, OUTPUT); // Set MOSFET control pin as output
  digitalWrite(MOSFET_PIN, LOW); // Ensure the MOSFET is initially off
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  
  // Initialize BME680
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1) { delay(10); }
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms

  Serial.println(F("BME680 sensor initialized"));
  Serial.println("Temperature_BME680,Pressure,Humidity,GasResistance,Altitude");
}

void loop() {
  // Read BME680 data
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  // Print sensor data in CSV format with headers
  Serial.print(bme.temperature);
  Serial.print(",");
  Serial.print(bme.pressure / 100.0);
  Serial.print(",");
  Serial.print(bme.humidity);
  Serial.print(",");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.print(",");
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  Serial.print(altitude);
  Serial.println();

  // Check altitude and control MOSFET
  if (altitude > maxAltitude) {
    maxAltitude = altitude;
  } else if (altitude < maxAltitude - 2) {
    Serial.println("Cracked!!");
    digitalWrite(MOSFET_PIN, HIGH); // Turn on the MOSFET
  } else {
    digitalWrite(MOSFET_PIN, LOW);  // Ensure MOSFET is off if no crack detected
  }

  delay(1000); // Adjust delay as needed
}