                                                                                                    #include <HX711_ADC.h>
#include <SPI.h>
#include <SD.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Pins
const int HX711_dout = 4; // MCU > HX711 dout pin
const int HX711_sck = 5;  // MCU > HX711 sck pin
const int sdCardCsPin = 10; // SD card chip select pin

// HX711 constructor
HX711_ADC LoadCell(HX711_dout, HX711_sck);
File dataFile;

const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {
  Serial.begin(57600); 
  delay(10);
  Serial.println(F("Starting..."));

  LoadCell.begin();
  unsigned long stabilizingtime = 2000; // Stabilizing time for HX711
  boolean _tare = true; // Set to false if you don't want tare performed
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println(F("Timeout, check MCU > HX711 wiring and pin designations"));
    while (1);
  } else {
    LoadCell.setCalFactor(2000.0); // Initial calibration value for 200 kg load cell
    Serial.println(F("Startup is complete"));
  }

  while (!LoadCell.update());
  calibrate(); // Start calibration procedure

  // Initialize SD card
  if (!SD.begin(sdCardCsPin)) {
    Serial.println(F("Initialization of SD card failed!"));
    while (1); // Stop here
  }
  Serial.println(F("SD card initialized."));

  // Create or open the file
  dataFile = SD.open("loadcell.csv", FILE_WRITE);
  if (!dataFile) {
    Serial.println(F("Error opening file"));
    while (1); // Stop here
  }
  Serial.println(F("File opened successfully."));

  // Write the header to the CSV file
  if (dataFile) {
    dataFile.println("Timestamp,Load Cell Reading");
    dataFile.flush(); // Ensure header is written to the file
  } else {
    Serial.println(F("Error opening file for writing."));
  }
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 100; // Adjust value to log data every 100 ms

  // Check for new data/start next conversion
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from the dataset
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();

      // Write reading to SD card
      if (dataFile) {
        dataFile.print(millis());
        dataFile.print(",");
        dataFile.println(i);
        dataFile.flush(); // Ensure data is written to the file
      } else {
        Serial.println(F("Error writing to file."));
      }

      newDataReady = 0;
      t = millis();
    }
  }

  // Receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); // Tare
    else if (inByte == 'r') calibrate(); // Re-calibrate
    else if (inByte == 'c') changeSavedCalFactor(); // Edit calibration value manually
  }

  // Check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println(F("Tare complete"));
  }
}

void calibrate() {
  Serial.println(F("***"));
  Serial.println(F("Start calibration:"));
  Serial.println(F("Place the load cell on a level stable surface."));
  Serial.println(F("Remove any load applied to the load cell."));
  Serial.println(F("Send 't' from serial monitor to set the tare offset."));

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println(F("Tare complete"));
      _resume = true;
    }
  }

  Serial.println(F("Now, place your known mass on the load cell."));
  Serial.println(F("Then send the weight of this mass (i.e., 100.0) from serial monitor."));

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print(F("Known mass is: "));
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); // Refresh the dataset to be sure the known mass is measured correctly
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); // Get the new calibration value

  Serial.print(F("New calibration value has been set to: "));
  Serial.print(newCalibrationValue);
  Serial.println(F(", use this as calibration value (calFactor) in your project sketch."));
  Serial.print(F("Save this value to EEPROM address "));
  Serial.print(calVal_eepromAdress);
  Serial.println(F("? y/n"));

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print(F("Value "));
        Serial.print(newCalibrationValue);
        Serial.print(F(" saved to EEPROM address: "));
        Serial.println(calVal_eepromAdress);
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println(F("Value not saved to EEPROM"));
        _resume = true;
      }
    }
  }

  Serial.println(F("End calibration"));
  Serial.println(F("***"));
  Serial.println(F("To re-calibrate, send 'r' from serial monitor."));
  Serial.println(F("For manual edit of the calibration value, send 'c' from serial monitor."));
  Serial.println(F("***"));
}

void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println(F("***"));
  Serial.print(F("Current value is: "));
  Serial.println(oldCalibrationValue);
  Serial.println(F("Now, send the new value from serial monitor, i.e., 696.0"));
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print(F("New calibration value is: "));
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print(F("Save this value to EEPROM address "));
  Serial.print(calVal_eepromAdress);
  Serial.println(F("? y/n"));
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print(F("Value "));
        Serial.print(newCalibrationValue);
        Serial.print(F(" saved to EEPROM address: "));
        Serial.println(calVal_eepromAdress);
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println(F("Value not saved to EEPROM"));
        _resume = true;
      }
    }
  }
  Serial.println(F("End change calibration value"));
  Serial.println(F("***"));
}
