#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0X27,16,2);  

long randomNumber;      //Variable donde almacenaremos el numero aleatorio
int tiempo = 200; 

byte ojo_abierto1[8] = {
  B01000,
  B00100,
  B00010,
  B10001,
  B01011,
  B00110,
  B01110,
  B11100
};

byte ojo_abierto2[8] = {
  B01000,
  B00111,
  B11111,
  B11100,
  B10001,
  B00111,
  B01111,
  B11111
};
byte ojo_abierto3[8] = {
  B00010,
  B11100,
  B11111,
  B00111,
  B10001,
  B11100,
  B11110,
  B11111
};


byte ojo_abierto4[8] = {
  B00010,
  B00100,
  B01000,
  B10001,
  B11010,
  B01100,
  B01110,
  B00111
};

byte ojo_abierto5[8] = {
  B11100,
  B01110,
  B00111,
  B00011,
  B00001,
  B00000,
  B00000,
  B00000
};

byte ojo_abierto6[8] = {
  B11111,
  B01111,
  B00111,
  B10001,
  B11100,
  B11111,
  B00111,
  B00000
};

byte ojo_abierto7[8] = {
  B11111,
  B11110,
  B11100,
  B10001,
  B00111,
  B11111,
  B11100,
  B00000
};

byte ojo_abierto8[8] = {
  B00111,
  B01110,
  B11100,
  B11000,
  B10000,
  B00000,
  B00000,
  B00000
};




byte ojo_cerrado1[8] = {
  B00000,
  B00000,
  B00000,
  B00001,
  B00011,
  B00111,
  B01111,
  B11111
};

byte ojo_cerrado2[8] = {
  B00000,
  B00111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte ojo_cerrado3[8] = {
  B00000,
  B11100,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte ojo_cerrado4[8] = {
  B00000,
  B00000,
  B00000,
  B10000,
  B11000,
  B11100,
  B11110,
  B11111
};

byte ojo_cerrado5[8] = {
  B11111,
  B01111,
  B00111,
  B01011,
  B10001,
  B00010,
  B00100,
  B01000
};

byte ojo_cerrado6[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00111,
  B01000
};

byte ojo_cerrado7[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11100,
  B00010
};

byte ojo_cerrado8[8] = {
  B11111,
  B11110,
  B11100,
  B11010,
  B10001,
  B01000,
  B00100,
  B00010
};


void setup() {
 lcd.begin();     // configuramos el LCD de 16x2
 lcd.clear();
 randomSeed(analogRead(A0));    //Establecemos la semilla en un pin analogico 
}

    
void loop() {
    
   randomNumber = random(1,100);     //Genera un numero aleatorio entre 1 y 100
   delay(tiempo);
   if(randomNumber>=55){
    ojos_abiertos();
   }
   else{
    ojos_cerrados();
   }
   
}

void ojos_abiertos() 
{
  lcd.createChar(1,ojo_abierto1);
  lcd.createChar(2,ojo_abierto2);
  lcd.createChar(3,ojo_abierto3);
  lcd.createChar(4,ojo_abierto4);
  lcd.createChar(5,ojo_abierto5);
  lcd.createChar(6,ojo_abierto6);
  lcd.createChar(7,ojo_abierto7);
  lcd.createChar(8,ojo_abierto8);

  lcd.setCursor(2,0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);

  lcd.setCursor(2,1);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);
  lcd.write(8);

 
  lcd.setCursor(10,0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);

  lcd.setCursor(10,1);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);
  lcd.write(8);
   
}

void ojos_cerrados() 
{
  lcd.createChar(1,ojo_cerrado1);
  lcd.createChar(2,ojo_cerrado2);
  lcd.createChar(3,ojo_cerrado3);
  lcd.createChar(4,ojo_cerrado4);
  lcd.createChar(5,ojo_cerrado5);
  lcd.createChar(6,ojo_cerrado6);
  lcd.createChar(7,ojo_cerrado7);
  lcd.createChar(8,ojo_cerrado8);

  lcd.setCursor(2,0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);

  lcd.setCursor(2,1);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);
  lcd.write(8);

  
  lcd.setCursor(10,0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);

  lcd.setCursor(10,1);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);
  lcd.write(8);
 
}
