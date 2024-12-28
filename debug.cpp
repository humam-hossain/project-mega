/*
 ESP8266 module - run from arduino 3V3 supply but needs a 4700uF capacitor on the 3V line
 CH_PD pulled high
 newer ESP8266 modules boot at 9600 but can be reprogrammed back to 115200
 reset pin to arduino pin 12 so can do hardware resets
 echo data to arduino terminal  
 The ESP8266 is 3V and arduino is 5V, the ESP8266 chip can handle ? up to 6V on the pins
 in theory, need a 5V to 3V level converter
 in practice, a 10k and 20k resistor divider didn't work, and connecting the 5V directly did work.
 Testing the current through a 100R resistor, getting only microamps, so conclusion is that it is ok to directly connect to arduino.

 
 The circuit: 
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
 
 */
#include <Arduino.h>
#include <SoftwareSerial.h>
//#include <avr/pgmspace.h> // saves ram, can store strings in flash as the little UNO only has 2k of ram and every quote uses this

#define SSID "Oslo_Cabin" // insert your SSID
#define PASS "" // insert your password (blank if no password)
#define destHost "www.google.com"
#define destPage "/"
#define destPort "80"
#define RxPin 10
#define TxPin 11
#define espRstPin 12
#define diagLedPin 13
#define serverLoopDelay 30 // server stays alive for this number of seconds, if a connection, then listens, if no connection then try google 
// PROGMEM prog_uchar signMessage[] = {"Hello World"};


// xively values
#define APIKEY         "shsCNFtxuGELLZx8ehqglXAgDo9lkyBam5Zj22p3g3urH2FM" // replace your pachube api key here
#define FEEDID         "970253233" // replace your feed ID
#define USERAGENT      "Arduino1" // user agent is the project name

SoftwareSerial espSerial(RxPin, TxPin); // RX, TX

char outBuff[256]; // store in one place rather than building strings and then copying them around in memory (or could use pointers)
int outBuffLen = 0; // can't use 0 as an end of string marker as might need to send out this as a valid binary character

String commandString = ""; // use .reserve on strings to save fixed memory. Use F when printing strings to store that string in flash

void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200); // for debugging
  while (!Serial) { 
    ; // wait for serial port to connect.
  }
  commandString.reserve(40); // use one general command string, avoids strange bugs with fragmented memory building tiny strings
  Serial.println(F("Starting Arduino"));
  espSerial.begin(9600);
  pinMode(diagLedPin, OUTPUT); // diagnostic led
  pinMode(espRstPin, OUTPUT); // ESP8266 reset pin
  digitalWrite(diagLedPin,LOW); //led off
  printFreeRam();
  //testStringMemory();
  rebootTestInternet();
}

void loop() 
{
  serverLoop(serverLoopDelay); // if getting connections keep the server going, if no connections for n seconds, check google is still live
  quickTestInternet(); // is google still contactable?
  //sendXively("sensor1",47);
  //delay(10000);
  //quickTestInternet();
  //delay(4000);
}

void testStringMemory()
{
  //int k=0;
  //char c;
  
  //c =  pgm_read_byte_near(signMessage + k); 
  //Serial.println(c);
  //strcpy_P(outBuff, (char*)pgm_read_word(&(signMessage))); // Necessary casts and dereferencing, just copy.  - crashed the program

}

boolean rebootTestInternet()
{
   boolean reply = false;
   resetESP8266(); // gets out of any hangs, eg if two browsers try to read at the same time
   flashLed();
   commandAT();
   commandCWMODE();
   flashLed();
   connectWiFi();
   displayIPaddress(); // useful for debugging and working out what address this has been assigned, could skip this 
   flashLed();
   commandCIPMUX();
   flashLed();
   reply = commandGOOGLE();
   return reply;
}

void quickTestInternet() // for second and subsequent tests, no need to disconnect the router etc
{
  boolean googleAlive = false;
  // Serial.println("Test internet alive");
  flashLed();
  googleAlive = commandGOOGLE();
  if (googleAlive == false) { // failed to reconnect so do a complete reboot and keep trying till reconnects
    do
    { 
      googleAlive = rebootTestInternet();
    } while (googleAlive == false); // keep repeating if can't reconnect
  }  
}  

void resetESP8266()
{
  int i;
  Serial.println(F("Hardware reset ESP8266"));
  digitalWrite(12,LOW);
  delay(200);
  digitalWrite(12,HIGH);
  for(i=0;i<1000;i++) // echo the startup message
  {
      if (espSerial.available())
         Serial.write(espSerial.read());
      delay(1);
  }   
}

void commandAT()
{
  espSerial.println(F("AT"));
  if(delayOK())
    Serial.println(F("AT OK"));
}  

void commandCWMODE()
{
    espSerial.println(F("AT+CWMODE=3"));
    delay(200); // replies with "no change" not OK so need custom code here
    if(espSerial.find("no change")) {
      Serial.println(F("CWMODE OK"));
    }else{
      Serial.println(F("Error CWMODE")); // the very first time the module runs this, will return an error, but remembers this forever in eeprom
   }  
}  

//void requestIPaddress() // this is a new function ?Dec 2014 and might not be available on some modules. Can set the IP address
//{
//  espSerial.println("AT+CIPSTA=\"192.168.1.70\""); 
//  waitReply(); // see all the text that comes back
//}

boolean delayOK() // times out after 8 seconds or if no OK. Sometimes takes 4 to 5 serial.find loops ie 4-5 seconds to respond
{
  
  int i = 0;
  boolean abort = false;
  serialFlush(); // remove any leftover things
  do
  {
    if(espSerial.find("OK")) {
       abort = true; // will abort the loop
     }   
     i+=1;
  }  
  while ((abort == false) && (i<8));
}  

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}   

boolean connectWiFi()
{
  commandString ="AT+CWJAP=\""; // global big string variable
  commandString += SSID;
  commandString += "\",\"";
  commandString += PASS;
  commandString += "\"";
  Serial.println(commandString);
  espSerial.println(commandString);
  if(delayOK()){
    Serial.println(F("OK, Connected to WiFi"));
    return true;
  }else{
    Serial.println(F("Cannot connect to the WiFi"));
    return false;
  }
}

void displayIPaddress() // a useful way to check actually connected
{
  espSerial.println(F("AT+CIFSR"));
  waitReply();
}

void waitReply() // wait for a character then prints out, the esp8266 is faster than arduino so once all the bytes have stopped that is it
{ 
  int counter = 0;
  do {
    counter +=1;
    delay(1);
  } while ((counter<3000) && (!espSerial.available())); // wait until one character comes in or times out
  while (espSerial.available()) {
    Serial.write(espSerial.read()); // print out the text, might be just a reply, or even an entire web page
  }  
}


void commandCIPMUX()
{
   espSerial.println(F("AT+CIPMUX=1"));
   if(delayOK())
     Serial.println(F("CIPMUX OK"));
}

boolean commandGOOGLE() // connect to google to test internet connectivity
{
  boolean reply = false;
  int count = 0;
  Serial.println(F("Test google home page"));
  commandString = "AT+CIPSTART=0,\"TCP\",\"";
  commandString += destHost;
  commandString += "\",80";
  espSerial.println(commandString);
  Serial.println(commandString);
  if(delayOK())
     Serial.println(F("CIPSTART OK")); 
  espSerial.find("Linked");
  outBuffClear(); // clear the output buffer prior to adding new data to send
  outBuffAppend("GET ");
  outBuffAppend(destPage);
  outBuffAppend(" HTTP/1.1\r\nHost: ");
  outBuffAppend(destHost);
  outBuffAppend(":");
  outBuffAppend(destPort);
  outBuffAppend("\r\n\r\n");
  commandSendBuff("0"); // send this data
  //waitReply(); // display what comes back - this does seem to overrun the buffer, not sure why as works at 9600 into vb.net
  // or light a led if got a correct website
  while ((reply == false) && (count <4))
  {
    if(espSerial.find("Found"))
    {
      digitalWrite(diagLedPin,HIGH); // led on, found a website 
      Serial.println(F("Reply from GOOGLE"));
      reply = true; 
    }  
    count +=1;
  }
  delay(1000);
  espSerial.flush();
  commandCIPCLOSEzero();
  return reply; // true or false
}  

void commandCIPCLOSEzero() // close port zero
{
    espSerial.println(F("AT+CIPCLOSE=0")); // close the connection from this end
    Serial.println(F("Close connection"));
    waitReply();
}  

void commandSendBuff(String ch_id) // sends outbuff array
{
  int i;
  commandString = "AT+CIPSEND=";
  commandString += ch_id;
  commandString += ",";
  commandString += String(outBuffLen);
  espSerial.println(commandString);
  Serial.println(commandString); // for debugging
  espSerial.find(">"); // wait for the command prompt to come back
  for (i=0;i<outBuffLen;i++) {
    espSerial.write(outBuff[i]); // output a byte
    //Serial.write(outBuff[i]); // debugging, might slow things down?
  }
}


void flashLed()
{
  digitalWrite(diagLedPin,HIGH);
  delay(50);
  digitalWrite(diagLedPin,LOW);
}

void serverLoop(int timeout) // if getting regular connections then no need to check google
{
  boolean hits = false;
  do
  {
    hits = runServer(timeout); // if get a hit then keep looping as no need to check google all the time
  } while (hits == true);
}

boolean runServer(int timeout) // http://mcuoneclipse.com/2014/11/30/tutorial-web-server-with-the-esp8266-wifi-module/
// timeout is in seconds
{
  int channelNumber;
  int counter = 0;
  boolean connection = false;
  commandAT(); // see if module is awake, sometimes it is busy from a previous command
  Serial.println(F("Starting server"));
  delay(1000);
  espSerial.println(F("AT+CIPSERVER=1,80")); // set up a server, 1 means open connection (0 is close), 80 is the port
  waitReply(); // can't look for OK, as sometimes replies as no change, and sometimes as busy 
  Serial.println(F("Connect now from a browser"));
  //need this next line to time out after a while so can recheck if still on the internet
  while ((espSerial.find("+IPD,") == false) && (counter < timeout)) // this is the login routine, and the next character will be the port number
  { 
    counter +=1; // one second
    Serial.print(timeout - counter); // countdown timer
  }  
  if (counter < timeout) { // must have got a connection, so process it, otherwise fall through and close off
    Serial.print(F("\n\rGot a connection on channel "));  
    channelNumber = espSerial.read() - 48; // 48 is ascii 0
    Serial.println(channelNumber); 
    waitReply();
    Serial.println(F("\n\rServing home page now"));
    serveHomePage(channelNumber); // or can just send some text, if replying to another node, or even just send "Hello"
    delay(2000); //delay
    waitReply();
    Serial.println(F("\n\rClose connection"));
    espSerial.println(F("AT+CIPCLOSE=0")); // must close connection before browser will display anything - returns Unlink so can't look for ok
    delay(100);
    waitReply();
    //espSerial.println("AT+CIPSERVER=0"); // turn off server mode - need to wait a bit after cipclose, otherwise returns busy, please wait
    //delay(100); // not sure if cipserver=0 is needed, will need to do more testing, does this disable google next cylcle, or just bad luck with my link?
    Serial.println(F("Finished closing"));
    connection = true;
  }else{
    Serial.println(F("\r\nNo connections received"));
    espSerial.println(F("AT+CIPSERVER=0")); // close down the server as nothing happened
    waitReply();
  }  
  return connection;
}  

void serveHomePage(int ch_id) { // http://christinefarion.com/2014/12/esp8266-serial-wifi-module/
{
  String header = "";
  String content = "";
  header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"; 
  // header += "Connection: close\r\n";  // or close with cipclose from this end
  content="";
  // output the value of each analog input pin
  for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    int sensorReading = analogRead(analogChannel);
    content += "analog input ";
    content += analogChannel;
    content += " is ";
    content += sensorReading;
    content += "<br />\n";       
  }
  header += "Content-Length:";
  header += (int)(content.length());
  header += "\r\n\r\n";
  
  // print to debug, then to wifi
  
  Serial.print("AT+CIPSEND=");
  Serial.print(ch_id);
  Serial.print(",");
  Serial.println(header.length()+content.length());
  Serial.print(header);
  Serial.print(content);
  
  espSerial.print("AT+CIPSEND=");
  espSerial.print(ch_id);
  espSerial.print(",");
  espSerial.println(header.length()+content.length());
  espSerial.print(header);
  espSerial.print(content);

  }
}

//char server[] = "api.xively.com";   // name address for xively API

/*
// this method makes a HTTP connection to the server:
void sendXively(String sensorName, int data)
{
  String xivelyData;
  Serial.println("Please wait");
  delay(4000);
  Serial.println("Connecting to xively");
  String cmd = "AT+CIPSTART=0,\"TCP\",\"";
  cmd += "api.xively.com";
  cmd += "\",80";
  espSerial.println(cmd);
  waitReply();
  delay(3000);
  waitReply();
  //Serial.println(cmd);
  //if(delayOK()) {
  //   Serial.println("CIPSTART OK"); 
  //}else{
 //    Serial.println("CIPSTART fail");
  //}   
  xivelyData = sensorName;
  xivelyData += ",";
  xivelyData += String(data);
  // build the put string  - can't send as one long string so send in pieces
  // or maybe put in an array first
  commandSend("PUT /v2/feeds/");
  commandSend(FEEDID);
  commandSend(".csv HTTP/1.1\r\nHost: api.xively.com\r\nX-ApiKey: ");
  commandSend(APIKEY);
  commandSend("\r\nUser-Agent: ");
  commandSend(USERAGENT);
  commandSend("\r\nContent-Length: ");
  commandSend(String(xivelyData.length())); // length of the data string
  commandSend("\r\nContent-Type: text/csv\r\nConnection: close\r\n\r\n"); 
  commandSend(xivelyData); // the actual data to send
  commandSend("\r\n\r\n"); // ? 1 or 2 crlfs
  

  //delay(5000);
  waitReply();
  delay(4000);
  waitReply();
  commandCIPCLOSEzero();
  waitReply();   
}
*/
void outBuffClear()
{
  outBuffLen = 0; // resent the counter. Can't reset the array to zero as zero is a valid binary number
}

void outBuffAppend(String s)
{ 
  int i;
  int st;
  int fin;
  int ln;
  char c;
  st = outBuffLen; // start at the beginning
  ln = s.length();
  fin = outBuffLen + ln;
  if (ln == 0) {
    Serial.println(F("Error - zero string length - buffer not being filled"));
  }  
  if (fin < 256) { // add string if there is space
    for(i=0;i<ln;i++) {
      c = s.charAt(i); // get the character, make sure it is a character
      outBuff[st] = c; // store it in the array
      st += 1; // increment the st value
    }
    outBuffLen += ln;
  }else{
    Serial.println(F("Buffer overrun"));
  } 
}


void outBuffInsert(String s) // inserts data at the beginning, shuffles existing data up. Safer than using strings as the buffer is predefined
{
  
}  

void outBuffPrint() // for debugging, print out all the values in hex
{
  int i;
  char c;
  for(i=0;i<outBuffLen;i++) {
    c=outBuff[i];
    Serial.print(c);
    printHex(outBuff[i]);
  }
}

void printHex(char x) {
   if (x < 16) {Serial.print(F("0"));}
   Serial.print(x, HEX);
   Serial.print(" ");
}

void printFreeRam()
{
  Serial.print(F("Free ram = ")); 
  Serial.println(freeRam());
}  

int freeRam () {
   extern int __heap_start, *__brkval; 
   int v; 
   return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}