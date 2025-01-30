#include "Commute.h"

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(240); // Max CPU frequency

    // WiFiManager wm;
    // wm.autoConnect("Kotleak_Connect", "Kotleak123");        // AP Mode Credentials.
    // CONFIG_TimeStamp(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER); // Gather Time stamp of Last configuration

    pinMode(CHG_STS_PIN, INPUT_PULLUP); // Configure battery measurement pin
    pinMode(BATT_PIN, INPUT);
    pinMode(CONFIG_PIN, INPUT_PULLUP);

    Serial.print("Current H/W Version : ");
    Serial.println(HardwareVer);
    Serial.print("Current F/W Version : ");
    Serial.println(FirmwareVer);
    FW_CHECK();

    if (!SD.begin(SD_CS_PIN)) // SD CARD Initialisation
    {
        Serial.println("SD card initialization -> FAILED");
        while (true)
            ;
    }
    Serial.println("SD card initialization -> SUCCESS.");

    if (intr_check == 1)
    {
        intr_check = 0;
        Serial.println("Entering Wi-Fi configuration mode...");
        WiFiManager wm;
        wm.startConfigPortal("Kotleak_Connect", "Kotleak123");
    }
}

void loop()
{
    // RECONNECT();
    WiFiManager wm;
    wm.autoConnect("", "Kotleak123");

    BATT_SOC();
    delay(60000);
}