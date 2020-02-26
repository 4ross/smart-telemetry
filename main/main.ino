// include the library code:
#include <LiquidCrystal.h>
#include <Arduino_MKRTHERM.h>
#include <WiFiNINA.h>
#include <RTCZero.h>
#include <ArduinoJson.h>

#include "arduino_secrets.h"
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);
IPAddress ip;

/* Create an rtc object */
RTCZero rtc;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 00;
const byte hours = 17;

/* Change these values to set the current initial date */
const byte day = 17;
const byte month = 11;
const byte year = 15;

float thermoCouple = 0.0;

StaticJsonDocument<200> doc;
char output[128];
  
void setup() {
  analogWrite(A3, 50); // Set the brightness to its maximum value
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);  // Print a message to the LCD.
  lcd.print("LCD: ok");
  delay(500);
  lcd.clear();
  lcd.print("THERM: init");
  if (!THERM.begin()) {
    while (1);
  }
  delay(500);
  lcd.clear();
  lcd.print("THERM: ok");

  // check for the WiFi module:
  lcd.clear();
  lcd.print("WIF: init");
  if (WiFi.status() == WL_NO_MODULE) {
    lcd.clear();
    lcd.print("WIFI: failed");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    lcd.clear();
    lcd.print("WIFI: firm upgrade");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    lcd.clear();
    lcd.print("WIFI: connecting");
    lcd.setCursor(0, 1);
    lcd.print(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  server.begin();                           // start the web server on port 80
  wifiStatus();                        // you're connected now, so print out the status




  rtc.begin();

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);
  rtc.setAlarmSeconds(seconds + 10);
  rtc.enableAlarm(rtc.MATCH_SS);

  rtc.attachInterrupt(alarmMatch);
}
void loop() {

  //WIFI
  WiFiClient client = server.available();   // listen for incoming clients





  if (client) {
    newWifiClient(client, thermoCouple);
  }
  delay(500);
}

void wifiStatus() {
  lcd.clear();
  lcd.print("WIFI: ready");

  // print your board's IP address:
  ip = WiFi.localIP();
  lcd.setCursor(0, 1);
  lcd.print(ip);
}

void newWifiClient(WiFiClient client, float thermoCouple )
{
  // if you get a client,
  lcd.clear();
  lcd.print("new client");
  String currentLine = "";                // make a String to hold incoming data from the client
  while (client.connected()) {            // loop while the client's connected
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      if (c == '\n') {                    // if the byte is a newline character

        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          //client.print("T: ");
          //client.print(thermoCouple);
          client.print(output);
          // The HTTP response ends with another blank line:
          //client.println();
          // break out of the while loop:
          break;
        } else {    // if you got a newline, then clear currentLine:
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }

      // Check to see if the client request was "GET /H" or "GET /L":
      if (currentLine.endsWith("GET /H")) {

      }
    }
  }
  // close the connection:
  client.stop();
  lcd.clear();
  lcd.print("disconnected");
}

void readThermocouple()
{
  thermoCouple = THERM.readTemperature();
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print(thermoCouple);
  lcd.print(" C");

  doc["temp"] = thermoCouple;
  doc["rpm"] = 0.0;
  
  serializeJson(doc, output);
}

void alarmMatch()
{
  
  byte newAlarm = rtc.getSeconds();
  byte hours = rtc.getHours();
  byte minutes = rtc.getHours();
  String dateTime = "";
  dateTime = String(hours)+String(minutes)+String(newAlarm);
  if (newAlarm >= 59)
  {
    newAlarm = 0;
  }
  rtc.setAlarmSeconds(newAlarm);
  lcd.clear();

  print2digits(hours);
  lcd.print(":");
  print2digits(minutes);
  lcd.print(":");
  print2digits(rtc.getSeconds());

  lcd.setCursor(0, 1);
  
  doc["time"] = dateTime;
  readThermocouple();
}

void print2digits(int number) {
  if (number < 10) {
    lcd.print("0"); // print a 0 before if the number is < than 10
  }
  lcd.print(number);
}
