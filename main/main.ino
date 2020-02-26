// include the library code: 
#include <LiquidCrystal.h> 
#include <Arduino_MKRTHERM.h>
// initialize the library by associating any needed LCD interface pin 
// with the arduino pin number it is connected to 
const int rs = 12, en = 11, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 
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
 } 
void loop() {
  float thermoCouple = 0.0;
  thermoCouple = THERM.readTemperature();
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print(thermoCouple);
  lcd.print(" C");
  delay(500);
 } 
