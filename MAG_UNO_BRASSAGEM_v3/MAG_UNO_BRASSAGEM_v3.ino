#include <Wire.h>
#include <OneWire.h>
#include <LiquidCrystal_I2C.h>

#define ONE_WIRE_BRASSAGEM 8   //  -- A PRIMEIRA PORTA de baixo para cima
#define ONE_WIRE_LAVAGEM 12  // OK -- SEGUNDA PORTA de baixo para cima

  #define VERSAO 1.0

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);

void setup(void) {
  Serial.begin(9600);
  
  lcd.begin(16,2);

lcd.setBacklight(HIGH);

  
}

void loop(void) {


TelaLCD(Temperatura(ONE_WIRE_BRASSAGEM), Temperatura(ONE_WIRE_LAVAGEM));

  
}


void TelaLCD(float temp3, float temp4)
{
  char outstr[20];
  lcd.setCursor(0,0);
  dtostrf(temp3, 2, 2, outstr);
  lcd.print(" BRASSAGEM:");
  lcd.print(outstr);
  
  lcd.setCursor(1,1);
  dtostrf(temp4, 2, 2, outstr);
  lcd.print("LAVAGEM  :");
  lcd.print(outstr);

}


float Temperatura(int porta)
{
    
  OneWire  ds(porta);  // on pin 10 (a 4.7K resistor is necessary)

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
  if ( !ds.search(addr)) {
    ds.reset_search();
    return -6;
  }
  
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      return -6;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

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
  Serial.print("  Temperature = ");
  Serial.println(celsius);
  return celsius;
}
