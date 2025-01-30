#include "Compute.h"

/*|---------------------------------------------------------------------------------------|
    Functions Pre-Declarations
|-----------------------------------------------------------------------------------------|*/

void CONFIG_INTR(void);             // Configuration mode interrupt
void STORE_CONFIG(void);            // Store Configuration data in to Preference
void ASSIGN_CONFIG(void);           // Assigns Wi-Fi credentials
void RECONNECT(void);               // Reconnects to Wi-Fi if connection lost
void CONFIG_MODE(void);             // Enters configuration mode
void FW_UPDATE(void);         // Updates firmware
void FW_CHECK(void);         // Checks firmware version
void DEVICE_DETAILS_TO_CLOUD(void); // Sends device details to cloud
void DATA_TO_CLOUD(void);           // Sends data to cloud
void STATUS_TO_CLOUD(void);         // Sends status to cloud

/*|---------------------------------------------------------------------------------------|
    Variables & Definations
|-----------------------------------------------------------------------------------------|*/

char ssid;
char password;
int ssidLen;
int passLen;
int CONNECT_STATUS = 0;
bool intr_check = 0;
float Wi_Fi_Strength;
long int WIFI_CONNECT_time_now;

WiFiUDP ntpUDP; // Set up UDP client for NTP and initialize NTP client
WiFiClient espClient;
IPAddress myIP;
WebServer server(80); // Object of WebServer(HTTP port, 80 is default)

/*|---------------------------------------------------------------------------------------|
    Pin Defincation
|-----------------------------------------------------------------------------------------|*/

const int LED_2R = 10;
const int LED_2G = 11;
const int LED_2B = 12;

/* |---------------------------------------------------------------------------------------|
      Default Values and Server Details
  |---------------------------------------------------------------------------------------|*/
const char *Default_STA_SSID = "Kotleak"; // Station mode credentials
const char *Default_STA_PASS = "NeverStop2016";

const char *ap_ssid = "Kotleak_V1.0"; // Access Point Mode credentials
const char *ap_password = "12345678";

#define REMOTE_HOST "www.google.com" // Enter the host name to ping for checking the internet


/*|---------------------------------------------------------------------------------------|
       Intrupt - 1 -> Takes the device in to Configuration Mode
|-----------------------------------------------------------------------------------------|*/

void IRAM_ATTR CONFIG_INTR(void)
{
    intr_check = 1;
}

/*|---------------------------------------------------------------------------------------|
    RECONNECT -> Process to connet to Wi-Fi Network
|-----------------------------------------------------------------------------------------|*/

void RECONNECT()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        CONNECT_STATUS = 0;
        WIFI_CONNECT_time_now = millis();
        Serial.print("Connecting time = ");
        Serial.println(WIFI_CONNECT_time_now);
        Serial.print("CONNECT_STATUS = ");
        Serial.println(CONNECT_STATUS);
        WiFi.begin(Default_STA_SSID, Default_STA_PASS);

        while (WiFi.status() != WL_CONNECTED && millis() - WIFI_CONNECT_time_now <= 8000) // Try connecting for 8 Seconds
        {
            Serial.print("Time Lapsed = ");
            Serial.println((millis() - WIFI_CONNECT_time_now) / 1000);
            digitalWrite(RGB_R, LOW);
            digitalWrite(RGB_G, LOW);
            digitalWrite(RGB_B, HIGH);
            delay(500);
            digitalWrite(RGB_R, LOW);
            digitalWrite(RGB_G, LOW);
            digitalWrite(RGB_B, LOW);
            delay(500);
            Serial.print("..");

            if (intr_check == 1) // For configuration
            {
                intr_check = 0;
                Serial.println("Entering Wi-Fi configuration mode...");
                WiFiManager wm;
                wm.startConfigPortal("IoT Node", "Kotleak123");
            }
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            CONNECT_STATUS = 1;
            Serial.println("Wi-Fi Network Connected");
            Serial.print("CONNECT_STATUS = ");
            Serial.println(CONNECT_STATUS);
        }
        else
        {
            CONNECT_STATUS = 0;
            Serial.println("Wi-Fi Network [NOT] Connected ");
            Serial.print("CONNECT_STATUS = ");
            Serial.println(CONNECT_STATUS);
        }
    }
    else if (CONNECT_STATUS = 1)
    {
        Serial.println(F("Wi-Fi NETWORK: [CONNECTED] - Checking Internet Accessiability ?"));
        bool internet_check = Ping.ping(REMOTE_HOST, 2);
        // float avg_time_ms = Ping.averageTime();
        if ((internet_check))
        {
            CONNECT_STATUS = 2;
            Serial.print("CONNECT_STATUS = ");
            Serial.println(CONNECT_STATUS);
            digitalWrite(RGB_R, LOW);
            digitalWrite(RGB_G, HIGH);
            digitalWrite(RGB_B, LOW);
            Serial.println("CLOUD NETWORK: [CONNECTED] -> Internet Accessiable");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.println("NETWORK: [CONNECTED] -> Internet [NOT] Accessiable");
            CONNECT_STATUS = 1;
            Serial.print("CONNECT_STATUS = ");
            Serial.println(CONNECT_STATUS);
            digitalWrite(RGB_R, LOW);
            digitalWrite(RGB_G, LOW);
            digitalWrite(RGB_B, HIGH);
        }
    }
}

/*|---------------------------------------------------------------------------------------|
    FW_CHECK  -> Check for Latest Firmware on GitHib or Any other cloud
|-----------------------------------------------------------------------------------------|*/

void FW_CHECK()
{
    Serial.println("Cheching for Latest Firmware");
    HTTPClient http;
    http.begin(URL_FW_Ver);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        String LatestVer = http.getString();
        LatestVer.trim();
        Serial.printf("Latest Firmware version: %s\n", LatestVer.c_str());

        if (LatestVer != FirmwareVer)
        {
            Serial.println("New Firmware Available -> Starting update. . . ");
            FW_UPDATE();
        }
        else
        {
            Serial.println("Device Already have Latest Version.");
        }
    }
    else
    {
        Serial.printf("Failed to fetch firmware version: %d\n", httpCode);
    }
    http.end();
}

/*|---------------------------------------------------------------------------------------|
    FW_UPDATE  -> Download and Update the device with Latest Firmware
|-----------------------------------------------------------------------------------------|*/

void FW_UPDATE()
{
    WiFiClientSecure client;
    client.setInsecure(); // Use for debugging only; remove in production

    Serial.println("Checking for firmware update...");
    t_httpUpdate_return ret = httpUpdate.update(client, URL_FW_Bin);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("Update failed: %d %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("No updates available.");
        break;
    case HTTP_UPDATE_OK:
        Serial.println("Update successful ->  Rebooting the device");
        break;
    }
}