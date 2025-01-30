#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";

WebServer server(80);
bool processRunning = false;
unsigned long startTime, endTime;
int numFiles, repeatCount;
String baseFileName;

HardwareSerial mySerial(1);  // Hardware serial for Quectel EG25 on TX=40, RX=41

void setup() {
    Serial.begin(115200);
    mySerial.begin(115200, SERIAL_8N1, 17, 16);  

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](){
        server.send(200, "text/html", R"rawliteral(
        <!DOCTYPE html>
        <html>
        <body>
            <h2>ESP32 File Upload</h2>
            <label>Number of files: </label>
            <input type="number" id="numFiles"><br><br>
            <label>Repeat count: </label>
            <input type="number" id="repeatCount"><br><br>
            <label>File base name: </label>
            <input type="text" id="baseFileName"><br><br>
            <button onclick="startProcess()">Start</button>
            <h3 id="status">Idle</h3>
            <script>
                function startProcess() {
                    fetch("/start?numFiles=" + document.getElementById("numFiles").value +
                          "&repeatCount=" + document.getElementById("repeatCount").value +
                          "&baseFileName=" + document.getElementById("baseFileName").value);
                    document.getElementById("status").innerText = "Processing...";
                }
                async function updateStatus() {
                    let response = await fetch("/status");
                    let text = await response.text();
                    document.getElementById("status").innerText = text;
                    setTimeout(updateStatus, 1000);
                }
                updateStatus();
            </script>
        </body>
        </html>
        )rawliteral");
    });

    server.on("/start", HTTP_GET, [](){
        if (server.hasArg("numFiles") && server.hasArg("repeatCount") && server.hasArg("baseFileName")) {
            numFiles = server.arg("numFiles").toInt();
            repeatCount = server.arg("repeatCount").toInt();
            baseFileName = server.arg("baseFileName");
            processRunning = true;
            startTime = millis();
            processFiles();
        }
        server.send(200, "text/plain", "Started");
    });

    server.on("/status", HTTP_GET, [](){
        if (processRunning) {
            server.send(200, "text/plain", "Processing...");
        } else {
            unsigned long totalTime = (endTime - startTime) / 1000;
            server.send(200, "text/plain", "Finished: " + String(numFiles) + " files, repeated " + String(repeatCount) + " times. Total time: " + String(totalTime) + " seconds.");
        }
    });

    Serial.println("Starting web server...");
    server.begin();
    Serial.println("Server started!");
}

void loop() {
    server.handleClient();
}

void sendATCommandAndWait(String command, String expectedResponse) {
    while (true) {
        Serial.println("Sent: " + command);
        mySerial.println(command);
        delay(1000);

        while (mySerial.available()) {
            String response = mySerial.readStringUntil('\n');
            response.trim();
            Serial.println("Received: " + response);

            if (response == expectedResponse) return;  
        }
        Serial.println("Retrying command: " + command);
        delay(1000);
    }
}

void sendATCommand(String command) {
    Serial.println("Sent: " + command);
    mySerial.println(command);
    delay(500);
}

void processFiles() {
    for (int i = 0; i < repeatCount; i++) {
        sendATCommandAndWait("AT+QPRTPARA=1", "OK");
        sendATCommandAndWait("AT+QHTTPCFG=\"contextid\",1", "OK");
        sendATCommandAndWait("AT+QIDEACT=1", "OK");
        sendATCommandAndWait("AT+QICSGP=1,1,\"airtelgprs.com\",\"\",\"\",1", "OK");
        sendATCommandAndWait("AT+QIACT=1", "OK");

        for (int j = 1; j <= numFiles; j++) {
            while (true) {  // Loop until file upload succeeds
                String fileNumber = (j < 10 ? "0" : "") + String(j);
                String fileName = "wav" + fileNumber + ".wav";
                String url = "https://hvr6s1ekgb.execute-api.ap-south-1.amazonaws.com/dev/kotleak-samp/" + baseFileName + fileNumber + ".wav";

                sendATCommandAndWait("AT+QHTTPCFG=\"contenttype\",2", "OK");
                sendATCommandAndWait("AT+QHTTPURL=" + String(url.length()) + ",80", "CONNECT");
                sendATCommandAndWait(url, "OK");

                sendATCommand("AT+QHTTPPUTFILE=\"SD:" + fileName + "\",1000");

                bool success = false;
                unsigned long startTime = millis();
                while (millis() - startTime < 100000) {
                    if (mySerial.available()) {
                        String response = mySerial.readStringUntil('\n');
                        response.trim();
                        Serial.println("Received: " + response);

                        if (response.startsWith("+QHTTPPUTFILE: 0,200,0")) {
                            success = true;
                            break;
                        } else if (response == "RDY") {  
                            Serial.println("Unexpected RDY response, restarting from AT+QHTTPURL...");
                            break;  
                        }
                    }
                }
                if (success) break;  // Exit loop if upload succeeded
            }
        }
        delay(20000);
    }
    endTime = millis();
    processRunning = false;
}
