// String command = "AT+CIPSTART=\"TCP\",\"" + server + "\"," + port;
  // if(send_command(command, 5000, true).indexOf("OK") == -1) {
  //   Serial.println("[ERROR] Failed to connect to server.");
  //   return;
  // }
  // Serial.println("[Success] established tcp connection with server: " + server + " on port: " + port);

  // String httpRequest = "GET " + path + " HTTP/1.1\r\nHost: " + server + "\r\nConnection: close\r\n\r\n";
  // send_command("AT+CIPSEND=" + String(httpRequest.length()), 2000, true); // Notify ESP of data length
  // send_command(httpRequest, 5000, true); // Send the actual GET request
  // send_command("AT+CIPCLOSE", 2000, true); // Close connection