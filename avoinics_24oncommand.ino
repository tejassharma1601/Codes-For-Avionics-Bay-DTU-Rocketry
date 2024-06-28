#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// Pin for MOSFET control
const int MOSFET_PIN = 8;

// Create objects for the sensors
Adafruit_MPU6050 mpu;
Adafruit_BME680 bme;

// Define sea level pressure for altitude calculation
#define SEALEVELPRESSURE_HPA (1013.25)

void setup() {
  pinMode(MOSFET_PIN, OUTPUT); // Set MOSFET control pin as output
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  
  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

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

  Serial.println(F("All sensors initialized"));
  Serial.println("Acceleration_X,Acceleration_Y,Acceleration_Z,Gyro_X,Gyro_Y,Gyro_Z,Temperature_MPU,Temperature_BME680,Pressure,Humidity,GasResistance,Altitude");
}

void loop() {
  // Read MPU6050 data
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  // Read BME680 data
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  // Print sensor data in CSV format with headers
  Serial.print(a.acceleration.x);
  Serial.print(",");
  Serial.print(a.acceleration.y);
  Serial.print(",");
  Serial.print(a.acceleration.z);
  Serial.print(",");
  Serial.print(g.gyro.x);
  Serial.print(",");
  Serial.print(g.gyro.y);
  Serial.print(",");
  Serial.print(g.gyro.z);
  Serial.print(",");
  Serial.print(temp_mpu.temperature);
  Serial.print(",");
  Serial.print(bme.temperature);
  Serial.print(",");
  Serial.print(bme.pressure / 100.0);
  Serial.print(",");
  Serial.print(bme.humidity);
  Serial.print(",");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.print(",");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println();

  // Serial control for MOSFET
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == '1') {
      digitalWrite(MOSFET_PIN, HIGH); // Turn on the MOSFET
      Serial.println("MOSFET turned ON.");
    } else if (command == '0') {
      digitalWrite(MOSFET_PIN, LOW); // Turn off the MOSFET
      Serial.println("MOSFET turned OFF.");
    } else {
      Serial.println("Invalid command. Type '1' to turn on, '0' to turn off.");
    }
  }

  delay(1000); // Adjust delay as needed
}