# vehicle-telematics
In this project, I use an ESP32 micro to read vehicle gps data. The data is stored and served to a frontend using a Flask based API. 

The ESP32 uses an MCP2515 CAN module to communicate with the vehicle over OBD2. It uses a GPS module to read gps location. It uses an SD card to store that information until it is able to connect to WiFi and upload the data to the DB using the API. The ESP32 code is found under the ESP32 folder. 

The API is written in Python Flask and hosted on a raspberry pi on the local network. The API takes in the data and stores stores the data under /[VIN]/[starting datetime]. The metadata is stored in a postgres DB that is also hosted on the raspberry pi. 

The Frontend was built locally to visualize the trips. 

can module: https://www.amazon.com/HiLetgo-MCP2515-TJA1050-Receiver-Arduino/dp/B01D0WSEWU 

GPS module: https://www.amazon.com/Navigation-Positioning-Microcontroller-Compatible-Sensitivity/dp/B084MK8BS2

SD module: https://www.amazon.com/HiLetgo-Adater-Interface-Conversion-Arduino/dp/B07BJ2P6X6

