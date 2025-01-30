#include "pin.h"
/*|---------------------------------------------------------------------------------------|
    Functions Pre-Declarations
|-----------------------------------------------------------------------------------------|*/

void BATT_SOC(void);         // Sends status to cloud

/*|---------------------------------------------------------------------------------------|
    Battery SoC 
|-----------------------------------------------------------------------------------------|*/

void BATT_SOC()
 {
  static esp_adc_cal_characteristics_t *adc_chars = nullptr;

  if (!adc_chars) {
    // Configure ADC width and attenuation
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_12); // 0-3.3V range

    // Characterize the ADC
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    Serial.println("Battery Voltage Measurement Initialized");
  }

  // Read ADC value
  uint32_t adc_reading = adc1_get_raw(ADC_CHANNEL);

  // Convert ADC reading to voltage in mV
  uint32_t adc_voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

  // Calculate battery voltage
  float battery_voltage = (adc_voltage / 1000.0) * VOLTAGE_DIVIDER_RATIO;

  // Calculate State of Charge (SoC)
  
  if (battery_voltage >= BATTERY_MAX_VOLTAGE) {
    soc = 100.0; // Fully charged
  } else if (battery_voltage <= BATTERY_MIN_VOLTAGE) {
    soc = 0.0; // Fully discharged
  } else {
    // Linear mapping
    soc = ((battery_voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0;
  }

  // Print results
  Serial.print("ADC Voltage: ");
  Serial.print(adc_voltage / 1000.0, 3);
  Serial.print(" V, Battery Voltage: ");
  Serial.print(battery_voltage, 3);
  Serial.print(" V, SoC: ");
  Serial.print(soc, 2);
  Serial.println(" %");
  delay(100);
}
