#include <Arduino.h>

// Use Serial1 for ESP8266 communication on the Arduino Mega
#define ESP8266 Serial1
#define BAUD_RATE 115200

#define PING_ADDR "www.google.com"

#define IP_ADDR "192.168.6.100"
#define DEFAULT_GATEWAY "192.168.6.1"
#define SUBNET_MASK "255.255.255.0"
#define TCP_SERVER_PORT 333

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 8080

// Function to send AT commands and wait for a valid response
String send_command(const String& command, unsigned long timeout, bool debug = false) {
  ESP8266.println(command);  // Send AT command to check ESP8266 connection
  unsigned long startTime = millis();
  bool completeResponse = false;
  String response = "";

  while (millis() - startTime < timeout) {
    if (ESP8266.available()) {
      char c = ESP8266.read();
      response += c;

      if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1 || response.indexOf("busy") != -1) {
        completeResponse = true;
      }
    }
  }

  // debugging
  if(debug) {
    Serial.println("[DEBUG] command: ");
    Serial.println(command);
    Serial.println("[DEBUG] response: ");
    Serial.println(response);
    Serial.print("[DEBUG] input buffer:");
    Serial.println(ESP8266.available());
  }

  if(!completeResponse) {
    Serial.println("[WARNING] incomplete response from ESP8266.");
  }

  Serial.println("[COMPLETED] " + command);
  return response;
}

// Function to ping an internet address
void ping_internet(int total_count, unsigned long timeout, bool debug = false) {
  int count = 0;
  
  for(int i = 0; i < total_count; i++) {
    String response = send_command("AT+PING=\"" + String(PING_ADDR) + "\"", timeout, debug);
    if (response.indexOf("OK") != -1) {
      count++;
    }
  }

  if(debug) {
    Serial.print("[INFO] Pinged ");
    Serial.print(count);
    Serial.print("/");
    Serial.println(total_count);
  }
  
  if(count == total_count){
    Serial.println("[SUCCESS] Internet connection is good.");
  }else if (count > 0){
    Serial.println("[WARNING] Internet connection is unstable.");
  }else{
    Serial.println("[ERROR] No internet connection.");
  }
}

// Function to set up the ESP8266
void setupESP8266() {
  Serial.println("[INFO] Configuring ESP8266...");

  Serial.println("[INFO] Resetting ESP8266...");
  send_command("AT+RST", 6500, false);
  Serial.println("[INFO] Turning on station mode on ESP8266");
  send_command("AT+CWMODE_CUR=1", 2000, false);

  // access to wifi
  String ssid = "no internet";
  String password = "humamhumam09";
  send_command("AT+CWSAP_CUR=\"" + ssid + "\",\"" + password + "\",5,3", 1000, false);

  // check ip and mac address
  send_command("AT+CIFSR", 1500, false);

  // check internet connection
  ping_internet(1, 2000);
  send_command("AT+CWMODE_CUR?", 1000, false);

  // Serial.println("[INFO] Setting up TCP server...");
  // send_command("AT+CIPAP_CUR=\"" + String(IP_ADDR) + "\",\"" + String(DEFAULT_GATEWAY) + "\",\"" + String(SUBNET_MASK) + "\"", 1000);
  // send_command("AT+CIPMUX=1", 1000);
  // send_command("AT+CIPSERVER=1," + String(TCP_SERVER_PORT), 1000);

  Serial.println("[INFO] ESP8266 setup complete.");
}

void handle_requests() {
  String status = send_command("AT+CIPSTATUS", 1000, false);
  if (status.indexOf("STATUS:2") != -1) {
    Serial.println("[INFO] ESP8266 is connected to a Wi-Fi network (station mode) but does not have any active TCP/UDP connections.");
  } else if (status.indexOf("STATUS:3") != -1){
    Serial.println("[INFO] There is an active TCP/UDP connection.");
  } else if (status.indexOf("STATUS:4") != -1){
    Serial.println("[INFO] The TCP connection was disconnected.");
  } else if (status.indexOf("STATUS:5") != -1){
    Serial.println("[INFO] The ESP8266 is not connected to any Wi-Fi network.");
  } else {
    Serial.println("[ERROR] AT+CIPSTATUS command failed.");
  }
}

void server_get_req() {
  String server = "192.168.0.101";
  String port = "8000";
  String path = "/api/get-data/";

  Serial.println("[INFO] Get data from server: " + server + " on port: " + port);
  
  // Establish a TCP connection with the server
  String connectCommand = "AT+CIPSTART=\"TCP\",\"" + server + "\"," + port;
  if (send_command(connectCommand, 1000, false).indexOf("OK") == -1) {
    Serial.println("[ERROR] Failed to connect to the server.");
    return;
  }
  Serial.println("[SUCCESS] Established TCP connection with server: " + server + " on port: " + port);

  // Prepare the HTTP request
  String httpRequest = "GET " + path + " HTTP/1.1\r\n" + 
                       "Host: " + server + "\r\n" + 
                       "Connection: close\r\n\r\n";

  String sendCommand = "AT+CIPSEND=" + String(httpRequest.length());
  if (send_command(sendCommand, 1000, false).indexOf(">") == -1) {
    Serial.println("[ERROR] ESP8266 not ready to send data.");
    send_command("AT+CIPCLOSE", 1000, false);
    return;
  }
  Serial.println("[SUCCESS] ESP8266 is ready to send data.");

  // Send the actual GET request
  String response = send_command(httpRequest, 2000, false);
  Serial.println("[INFO] Server Response: " + response);
}

void server_post_req(const String& payload) {
  String server = "192.168.0.101";
  String port = "8000";
  String path = "/api/post-data/";

  Serial.println("[INFO] send data to server: " + server + " on port: " + port);
  // Establish a TCP connection with the server
  String connectCommand = "AT+CIPSTART=\"TCP\",\"" + server + "\"," + port;
  if (send_command(connectCommand, 1000, false).indexOf("OK") == -1) {
    Serial.println("[ERROR] Failed to connect to the server.");
    return;
  }
  Serial.println("[SUCCESS] Established TCP connection with server: " + server + " on port: " + port);

  // Create POST request
  String httpRequest = "POST " + path + " HTTP/1.1\r\n" +
                       "Host: " + server + "\r\n" +
                       "Content-Type: text/plain\r\n" +  
                       "Content-Length: " + String(payload.length()) + "\r\n" +
                       "Connection: close\r\n\r\n" +
                       payload; // Append the payload to the request
  
  // Notify ESP8266 of the data length
  String sendCmd = "AT+CIPSEND=" + String(httpRequest.length());
  if (send_command(sendCmd, 2000, false).indexOf(">") == -1) {
    Serial.println("[ERROR] ESP8266 not ready to send data.");
    send_command("AT+CIPCLOSE", 1000, false);
    return;
  }

  // Send the HTTP POST request
  String response = send_command(httpRequest, 2000, false);
  Serial.println("[INFO] Server Response: " + response);
}

void setup() {
  Serial.begin(BAUD_RATE); // Set up Serial communication for debugging
  while(!Serial){
    Serial.println("[WARNING] Waiting for Serial to be ready...");
  };  // Wait for Serial to be ready
  
  ESP8266.begin(BAUD_RATE); // Set up Serial1 for ESP8266 communication
  while(!ESP8266){
    Serial.println("[WARNING] Waiting for ESP8266 to be ready...");
  }  // Wait for ESP8266 to be ready

  String response = "";
  while(ESP8266.available()) {
    Serial.println(ESP8266.available());
    char c = ESP8266.read();
    response += c;
  }  // Clear the input buffer
  Serial.println(response);

  setupESP8266();
}

void loop() {
  // Serial.println(ESP8266.available());

  String payload = "t:125.80 samples:8 r_25um:6.68 ugm3_25um:0.68 pcs_25um:4176.45 r_1um:7.16 ugm3_1um:0.73 pcs_1um:4473.25";
  server_post_req(payload);
  server_get_req();
  
  delay(1000);
}
