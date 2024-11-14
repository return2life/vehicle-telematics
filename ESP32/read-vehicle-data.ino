#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <FS.h>
#include <SD.h>
#include <mcp2515.h>

const int buttonPin = 22; // GPIO22 pin connected to button
int buttonState = 0;
const int maxChunkSize = 1024; // Number of bytes to upload at a time

const char* ssid = "";
const char* password = "";

// Flask server endpoint
const char* serverUrl = "raspberrypi.local:5000/upload";  // Change this to your server's IP or URL

// GPS and SD card setup
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
bool isHome = true;
bool returnedHome = false;
// Define your home location
const double homeLat = ;  //  latitude for "home"
const double homeLng = ; //  longitude for "home"
// Function to calculate distance between two GPS coordinates
double distanceBetween(double lat1, double lon1, double lat2, double lon2) {
  return TinyGPSPlus::distanceBetween(lat1, lon1, lat2, lon2);
}

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

String dataFileName;      // Variable to hold the new file name
bool uploadSuccess = true;
String vin = "12345"; // change to your VIN or add section to read VIN from vehicle

// Create two SPI objects for VSPI and HSPI
SPIClass *vspi = NULL;
SPIClass *hspi = NULL;

// MCP2515 pins (VSPI)
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCLK 18
#define VSPI_SS   5  // Chip Select for MCP2515

// SD card pins (HSPI)
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_SS   15  // Chip Select for SD card

struct MCP2515 mcp2515(VSPI_SS);  // MCP2515 CAN object

unsigned long lastGPSLogTime = 0; // To store the last log time
const unsigned long GPSlogInterval = 1000; // Log interval (1 second)

unsigned long lastCANLogTime = 0; // To store the last log time
const unsigned long CANlogInterval = 950; // Log interval (1 second)

// info for can
struct can_frame canMsg;
struct can_frame sendMsg;
struct can_frame pressureMsg;
uint16_t obd2_requests[5] = {0x0D, 0x04, 0x10, 0x0C, 0x11}; 

void setup() {

  sendMsg.can_id  = 0x7DF;
  sendMsg.can_dlc = 8;
  sendMsg.data[0] = 0x02;
  sendMsg.data[1] = 0x01;
  sendMsg.data[2] = 0x0D;
  sendMsg.data[3] = 0x55;
  sendMsg.data[4] = 0x55;
  sendMsg.data[5] = 0x55;
  sendMsg.data[6] = 0x55;
  sendMsg.data[7] = 0x55;

  // Initialize serial communication for ESP32
  Serial.begin(115200);


  // Initialize MCP2515 CAN controller on VSPI bus
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  Serial.println("MCP2515 initialized on VSPI");

  // Initialize GPS communication
  ss.begin(GPSBaud);
  // Initialize button
  pinMode(buttonPin, INPUT);

// Initialize HSPI for SD card
  hspi = new SPIClass(HSPI);
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);  // SCLK, MISO, MOSI, SS

  // Initialize SD card on HSPI bus
  if (!SD.begin(HSPI_SS, *hspi)) {
    Serial.println("Card Mount Failed");
    return;
  }
  Serial.println("SD Card initialized on HSPI");

  dataFileName = "/carData.txt";
  Serial.print("Data will be saved to: ");
  Serial.println(dataFileName);

  // Create/open the new file
  if (!SD.exists(dataFileName)) {
    File dataFile = SD.open(dataFileName, FILE_WRITE);
    if (dataFile) {
      dataFile.print("&"); // indicates a new trip
      dataFile.print("\n");
      dataFile.close();
    } else {
      Serial.println("Error opening file on SD card!");
    }
  } else {
    File dataFile = SD.open(dataFileName, FILE_APPEND);
    if (dataFile) {
      dataFile.print("&"); // indicates a new trip
      dataFile.print("\n");
      dataFile.close();
    } else {
      Serial.println("Error opening file on SD card!");
    }
  }
  delay(5000);
}

void loop() {
  // Check if GPS data is available and process it
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  unsigned long currentTime = millis();
  if (currentTime - lastGPSLogTime >= GPSlogInterval) {
    lastGPSLogTime = currentTime;  // Update the last log time

    // If a valid location is available, print it and save to SD card
    if (gps.location.isValid() && gps.time.isValid()) {
      double latitude = gps.location.lat();
      double longitude = gps.location.lng();
      double altitude = gps.altitude.meters();

      // Print GPS data to Serial Monitor
      Serial.println(latitude, 6);
      Serial.println(longitude, 6);
      Serial.println(altitude);
      Serial.print("Time: ");
      if (gps.time.hour() < 10) Serial.print(F("0"));
      Serial.print(gps.time.hour());
      Serial.print(F(":"));
      if (gps.time.minute() < 10) Serial.print(F("0"));
      Serial.print(gps.time.minute());
      Serial.print(F(":"));
      if (gps.time.second() < 10) Serial.print(F("0"));
      Serial.print(gps.time.second());
      Serial.print(F("."));
      if (gps.time.centisecond() < 10) Serial.print(F("0"));
      Serial.print(gps.time.centisecond());
      Serial.println("");

      // Log GPS data to the SD card
      File dataFile = SD.open("/" + dataFileName, FILE_APPEND);
      if (dataFile) {
        dataFile.print(gps.date.year());
        dataFile.print(F("-"));
        if (gps.date.month()< 10) dataFile.print(F("0"));
        dataFile.print(gps.date.month());
        dataFile.print(F("-"));
        if (gps.date.day()< 10) dataFile.print(F("0"));
        dataFile.print(gps.date.day());
        dataFile.print(F(" "));
        if (gps.time.hour() < 10) dataFile.print(F("0"));
        dataFile.print(gps.time.hour());
        dataFile.print(F(":"));
        if (gps.time.minute() < 10) dataFile.print(F("0"));
        dataFile.print(gps.time.minute());
        dataFile.print(F(":"));
        if (gps.time.second() < 10) dataFile.print(F("0"));
        dataFile.print(gps.time.second());
        dataFile.print(",");
        dataFile.print(latitude, 6);
        dataFile.print(",");
        dataFile.print(longitude, 6);
        dataFile.print(",");
        dataFile.print(altitude);
        dataFile.close();
      } else {
        Serial.println("Error writing to SD card!");
      }

      for (int i = 0; i < sizeof(obd2_requests)/2; i++) {
        sendCAN(obd2_requests[i]);
        delay(10);  // Delay between messages
        readCAN(obd2_requests[i]);
      }

      dataFile = SD.open("/" + dataFileName, FILE_APPEND);
      dataFile.print("\n");
      dataFile.close();

      // Calculate the distance from the current location to "home"
      double distanceToHome = distanceBetween(latitude, longitude, homeLat, homeLng);
      Serial.println("Distance to home:");
      Serial.println(distanceToHome);
      if (distanceToHome < 50 && isHome == false) {
        isHome = true;
        returnedHome = true;
        Serial.println("Changing is home to true.");
      } 
      else if (distanceToHome > 50) {
        isHome = false;
        returnedHome = false;
        Serial.println("changing isHome to false.");
      } 
      if ( returnedHome == true ) {
        connectToWiFi();
        uploadFileToServer(dataFileName);
      }

    } // if gps is valid


    

    // Check to see if the button is pressed
    buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH) {
      connectToWiFi();
      uploadFileToServer(dataFileName);
    }

  } // close every minute

}

void sendCAN(uint16_t pid) {
  canMsg.can_id  = 0x7DF;  // OBD2 request ID
  canMsg.can_dlc = 8;      // Data length (always 8 for OBD2 requests)
  canMsg.data[0] = 0x02;   // Number of additional data bytes
  canMsg.data[1] = 0x01;   // Service ID: 01 (Show current data)
  canMsg.data[2] = pid; // Upper byte of PID
  canMsg.data[3] = 0x00;   // Unused
  canMsg.data[4] = 0x00;   // Unused
  canMsg.data[5] = 0x00;   // Unused
  canMsg.data[6] = 0x00;   // Unused
  canMsg.data[7] = 0x00;   // Unused
  
  mcp2515.sendMessage(&canMsg);
  Serial.print("Sent OBD2 Request: 0x");
  Serial.println(pid, HEX);

}
void readCAN(uint16_t pid) {
    File dataFile = SD.open("/" + dataFileName, FILE_APPEND);
    dataFile.print(",");
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      if (pid == canMsg.data[2]) {
        if (pid == 0x0D) { // vehicle speed
          dataFile.print((int)canMsg.data[3]);
        } else if (pid == 0x04) { // engine load
          dataFile.print((int)canMsg.data[3]/2.55); 
        } else if (pid == 0x10) { // mass airflow
          dataFile.print(((int)canMsg.data[3]*256+(int)canMsg.data[4])/100);
        } else if (pid == 0x0C) { // engine speed
          dataFile.print(((int)canMsg.data[3]*256+(int)canMsg.data[4])/4);
        } else if (pid == 0x11) { // throttle pos
          dataFile.print(((int)canMsg.data[3]*100)/255);
        }
        dataFile.close();

        for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
          Serial.print(canMsg.data[i],HEX);
          Serial.print(" | ");
        }
      }
    } 
}

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Already connected to WiFi.");
  }
}

void uploadFileToServer(String filename) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file");
    return;
  }

  String sessionID = "";
  String json = "{\"session_id\":";
  bool isNewSession = false;
  bool newTrip = true;
  String data = "";

  while (file.available()) {
    char c = file.read();

    if (c == '&' || !file.available() || (c == '\n' && (data.length() > maxChunkSize))) {  // Detect new session marker
      if (data != "") {  // Close previous session JSON object if needed
        json += "\"" + sessionID + "\"" + ",\"newTrip\":" + newTrip + ",\"VIN\":\"" + vin + "\"" + ",\"data\":\"" + data + "\"}";
        uploadSuccess = true;
        sendJsonChunk(json); // send data
        json = "{\"session_id\":";
        newTrip = false;
      }
      if (c == '&') {
        isNewSession = true;
        sessionID = "";
        newTrip = true;
      }
      data = "";
    } else if (isNewSession && c != ',') {
      if (c != '\n') {
        sessionID += c;  // Build session ID
        data += c;
      }
    } else if (isNewSession && c == ',') {
      data += c;
      isNewSession = false;
    } else if (c == '\n') {
      if (data != "") {
        data += "\\n";
      }
    }
    else {
      data += c;
    }
    //Serial.println(data);
  }
  file.close();
  if (uploadSuccess == true) {
      // Delete the file after reading
      if (SD.remove(filename)) {
        Serial.println("\nFile deleted successfully!");
        esp_deep_sleep_start();
      } else {
        Serial.println("\nFailed to delete file!");
      }
  }
}

// Function to send a chunk of JSON data
void sendJsonChunk(String jsonChunk) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");  // Set content type
    // Send POST request with JSON data chunk
    int httpResponseCode = http.POST(jsonChunk);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Server response: " + response);
      if (httpResponseCode != 200) {
        uploadSuccess = false;
      }
    } else {
      Serial.printf("Failed to upload chunk, error: %s\n", http.errorToString(httpResponseCode).c_str());
      uploadSuccess = false;
    }
    http.end();  // Free resources
  } else {
    Serial.println("WiFi not connected.");
  }
}

