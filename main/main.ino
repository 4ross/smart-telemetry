// include the library code:
#include <LiquidCrystal.h>
#include <Arduino_MKRTHERM.h>
#include <WiFiNINA.h>
#include <RTCZero.h>
#include <ArduinoJson.h>
#include <CircularBuffer.h>

#include "arduino_secrets.h"

#define ARRAY_SIZE 10
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

/*** RTC ***/
/* Create an rtc object */
RTCZero rtc;

/* Change these values to set the current initial time */
const byte StartSeconds = 00;
const byte StartMinutes = 00;
const byte StartHours = 00;

/* Change these values to set the current initial date */
const byte StartDay = 00;
const byte StartMonth = 00;
const byte StartYear = 00;

byte seconds = 00;
byte minutes = 00;
byte hours = 00;
byte day = 00;
byte month = 00;
byte year = 00;

bool rtcAlarm = false;

/*** SAMPLE VALUE ***/
int rtcTime = 0;
float thermoCouple = 0.0;
int rpm = 0;

/*** ARRAY VALUES ***/
int ArrayIndex;

//int   RtcTimeArray[ARRAY_SIZE];
float ThermoCoupleArray[ARRAY_SIZE];
int   RpmArray[ARRAY_SIZE];

//*** CIRCULAR BUFFER***/
CircularBuffer<int, 10> TimeCircularBuffer;      
CircularBuffer<float, 10> TempCircularBuffer;    
CircularBuffer<int, 10> RpmCircularBuffer;      

/***JSON***/
const int capacity = JSON_ARRAY_SIZE(ARRAY_SIZE) + 10 * JSON_OBJECT_SIZE(3);
DynamicJsonDocument doc(capacity);
JsonObject jsonObject[ARRAY_SIZE];

void setup() {
  //*** LCD ***//
  analogWrite(A3, 50); // Set the brightness to its maximum value
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);  // Print a message to the LCD.
  lcd.print("LCD: ok");
  delay(500);
  lcd.clear();

  //*** THERMO ***//
  lcd.print("THERM: init");
  if (!THERM.begin()) {
    while (1);
  }
  delay(500);
  lcd.clear();
  lcd.print("THERM: ok");

  //*** WIFI ***//
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

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  lcd.clear();
  lcd.print("WIFI: connecting");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  if (status != WL_AP_LISTENING) {

    lcd.clear();
    lcd.print("Creating access point failed");
    // don't continue
    while (true);
  }
  delay(10000);

  // attempt to connect to Wifi network:
  //  while (status != WL_CONNECTED) {
  //    lcd.clear();
  //    lcd.print("WIFI: connecting");
  //    lcd.setCursor(0, 1);
  //    lcd.print(ssid);
  //    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  //    status = WiFi.begin(ssid, pass);
  //    // wait 10 seconds for connection:
  //    delay(10000);
  //  }

  server.begin();                           // start the web server on port 80
  wifiStatus();                        // you're connected now, so print out the status

  //***JSON***//
  createJsonDoc();

  //*** RTC ***//
  rtc.begin();
  rtc.setTime(StartHours, StartMinutes, StartSeconds);
  rtc.setDate(StartDay, StartMonth, StartYear);
  rtc.setAlarmSeconds(StartSeconds + 9);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(alarmMatch);

  /***ARRAY SAMPLES**/
  ArrayIndex = 0;
}
void loop() {
  //*** THERMO ***//
  thermoCouple = THERM.readTemperature();

  if (rtcAlarm)
  {
    rtcAlarm = false;
    if (ArrayIndex >= ARRAY_SIZE)
    {
      ArrayIndex = 0;
    }
    saveTime();
    saveThermocouple();
    saveRpm();
    ArrayIndex += 1;

  }

  //*** WIFI ***//
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      lcd.clear();
      lcd.print("WIFI: new device");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      lcd.clear();
      lcd.print("WIFI: disc device");
    }
  }

  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {
    putDataInJson();
    newWifiClient(client);
  }
}

void wifiStatus() {
  lcd.clear();
  lcd.print("WIFI: ready");

  // print your board's IP address:
  ip = WiFi.localIP();
  lcd.setCursor(0, 1);
  lcd.print(ip);
}

void newWifiClient(WiFiClient client)
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
          client.println();
          serializeJson(doc, client);
          // The HTTP response ends with another blank line:
          client.println();
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

void createJsonDoc()
{
  for (int i = 0 ; i < ARRAY_SIZE; i++)
  {
    jsonObject[i] = doc.createNestedObject();
  }
}

void putDataInJson()
{
  for (int i = 0 ; i < ARRAY_SIZE; i++)
  {
    jsonObject[i]["time"] = TimeCircularBuffer[i];
    jsonObject[i]["temp"] = ThermoCoupleArray[i];
    jsonObject[i]["rpm"] = RpmArray[i];
  }
}


void alarmMatch()
{
  rtcAlarm = true;
  seconds = rtc.getSeconds();
  hours = rtc.getHours();
  minutes = rtc.getMinutes();

  rtc.setAlarmSeconds(seconds);
}

void saveRpm()
{
  RpmArray[ArrayIndex] = 0.0;

  lcd.print("R:");
  lcd.print(RpmArray[ArrayIndex]);
}

void saveThermocouple()
{
  ThermoCoupleArray[ArrayIndex] = thermoCouple;

  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(ThermoCoupleArray[ArrayIndex]);
}

void saveTime()
{
  rtcTime = getRtcTime(hours, minutes, seconds);
  if (TimeCircularBuffer.isFull())
  {
    TimeCircularBuffer.shift();
  }
  TimeCircularBuffer.push(rtcTime);

  lcd.clear();
  lcd.print(TimeCircularBuffer[ArrayIndex]);
  lcd.print(",");
  lcd.print(ArrayIndex);
}

int getRtcTime(int hours, int minutes, int seconds)
{
  int time = 0;
  time = time + seconds;
  time = time + 100 * minutes;
  time = time + 10000 * hours;
  return time;
}
