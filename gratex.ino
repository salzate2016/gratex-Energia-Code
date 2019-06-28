#include <Servo.h>

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <hcrs04.h>

#define PINTRIG 6
#define PINECHO 5
String voice; 
OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)
const int humidity = A0;/* Soil moisture sensor O/P pin */
int motorPin =7;
LiquidCrystal lcd(P2_0, P2_1, P2_3, P2_4, P2_5, P1_6);
Servo myservo;
int pos = 0; 
hcrs04 ultra(PINTRIG, PINECHO);

void setup() {
  Serial.begin(9600); /* Define baud rate for serial communication */
  lcd.begin(16, 2);
  pinMode(19, OUTPUT);
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  digitalWrite(19, LOW);
  ultra.begin();
  
}

void loop() {
  int onOff=appControl();
  int lightLevel=light();
  float moistureLevel=moisture();
  float temperature=temp();
  float distance=cutGrass();
  //control(temperature, moistureLevel, lightLevel);
  if(onOff==0){
    control(temperature, moistureLevel, lightLevel);
  }
  appPrint(temperature, moistureLevel,distance);
  appControl();
  
}

int light(){
  int lightLevel = analogRead(15); // Read the light level from analog pin 2
  delay(1); // delay for 1 millisecond for smoothness
  //Serial.println(lightLevel); // Print the analog value to Serial
  // adjust the value 0 to max resolution to span 0 to 255
  lightLevel = map(lightLevel, 0, 300, 0, 255); 
  lightLevel = constrain(lightLevel, 0, 255); // constrain values between 0-255
  return lightLevel;
}

float moisture(){
  float moisture_percentage;
  int humidity_analog;
  humidity_analog = analogRead(humidity);
  //Serial.print(humidity_analog);
  moisture_percentage = ( ( (humidity_analog/1023.00) * 100 ) );
  //Serial.print("Moisture Percentage = ");
  //Serial.print(moisture_percentage);
 //Serial.print("%\n\n");
  printMoisture(moisture_percentage);
  //delay(1000);
  return moisture_percentage;
}

float temp(){
   byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
    //return 0;
  }
  
  //Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    //Serial.write(' ');
    //Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      //Serial.println("CRC is not valid!");
      //return 0;
  }
  //Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return 0;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  //Serial.print("  Temperature = ");
  //Serial.print(celsius);
  //Serial.print(" Celsius, ");
  //Serial.print(fahrenheit);
  //Serial.println(" Fahrenheit");
  printTemp(fahrenheit);

  return fahrenheit;
}

void waterOn(){
  digitalWrite(motorPin, HIGH);
  //delay(2000);
}

void waterOff(){
  digitalWrite(motorPin, LOW);
  //delay(2000);
}

float cutGrass(){
  float distance = ultra.read(); /* Read the distance value */
  //Serial.print("Distance : ");
  //Serial.print(distance);
  //Serial.println(" cm");
  if(distance<=10)
  {digitalWrite(19, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("please cut Grass");
  //delay(2000);
  }else{
    digitalWrite(19, LOW);
    }
  return distance;
}

void appPrint(float temp, float moisture, float distance)
{
  Serial.print(temp);
  Serial.print(" F");
  Serial.print(",");
  Serial.print(moisture);
  Serial.print(" %");
  Serial.print(",");
  if(distance<=10)
  {
    Serial.println("Cut your grass");
    //Serial.println("|");
  }
  else
  {
    Serial.println("Grass ok");
    //Serial.println("|");
  }
  //delay(2000);
}



int appControl()
{
  if (Serial.available())
  { 
  delay(10); 
  char c = Serial.read(); 
  voice += c; 
  }  
  if (voice.length() > 0)
  {
    if(voice == "on")
    {
      digitalWrite(13, HIGH);
      return 1;
    }  
    if(voice == "off")
    {
      digitalWrite(13, LOW);
      return 0;
    } 
    voice="";
  }
}

void printMoisture(float moisture)
{
  lcd.setCursor(0, 1);
  lcd.print("Moisture=");
  lcd.print(moisture);
  lcd.print("%");
}

void printTemp(int temp)
{
  lcd.setCursor(0, 0);
  lcd.print("Temperature:  ");
  lcd.print(temp);
}


void control(float fahrenheit, float moisture_percentage, int lightLevel){
  if((fahrenheit > 80 && fahrenheit < 81)||(moisture_percentage > 50 && moisture_percentage <55)||(lightLevel > 150 && lightLevel < 155)){}
  else
  {
    if(fahrenheit<=80 && moisture_percentage <= 50 && lightLevel >= 150)
    {
        digitalWrite(motorPin, HIGH);
        myservo.attach(18);
        servo();
        delay(200);
    }
    else
    {
        digitalWrite(motorPin, LOW);
        myservo.detach();
        delay(200);
    }
  }
}

void servo(){
  for(pos = 10; pos < 100; pos += 1)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  delay(1000);
  for(pos = 100; pos>=10; pos-=1)     // goes from 180 degrees to 0 degrees 
  {                                
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  
}

