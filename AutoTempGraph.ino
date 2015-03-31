#include <SPI.h>
#include "Adafruit_FRAM_SPI.h"
#include "DHT.h"
#include "openGLCD.h"
#include <Bounce.h>

uint8_t FRAM_CS = 45;
uint8_t FRAM_SCK= 42;
uint8_t FRAM_MISO = 43;
uint8_t FRAM_MOSI = 44;
//Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_SCK, FRAM_MISO, FRAM_MOSI, FRAM_CS);
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS);

#define DHTPIN 27     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE,12);
//DHT dht(DHTPIN, DHTTYPE,3);
unsigned long pm1 = millis();
unsigned long pm2 = millis();
unsigned long pm3 = millis();
unsigned long pm4 = millis();
float h,t;
float ta[99];

gText textArea1;
gText textArea2;

float taav;
float tcav;
int mmpos = 98;
boolean cani = true;
boolean fail = false;
boolean lframe = true;
Bounce bluebutton = Bounce( 41,5 );
Bounce graybutton = Bounce( 42,5 ); 
Bounce greenbutton = Bounce( 43,5 ); 
byte xRange = 1;
String zoomstr = "1:1";

void setup() {

  pinMode(0, OUTPUT);
  analogWrite(0, 100);
  Serial.begin(9600); 
  GLCD.Init();
  GLCD.ClearScreen();
  GLCD.SelectFont(System5x7, BLACK);
  //GLCD.SelectFont(font8x8, BLACK);
  if (fram.begin()) {
    GLCD.println("OK");
  } 
  else {
    GLCD.println("  No SPI FRAM found. \n\n  Check Connections. \n\n   Reset when ready");
    //Serial.println("No SPI FRAM found ... check your connections\r\n");
    while (1){
      analogWrite(0, ((millis()/7)%156)+100);
    };
  }

  pinMode(41, INPUT_PULLUP); 
  pinMode(42, INPUT_PULLUP); 
  pinMode(43, INPUT_PULLUP); 
  pinMode(6, OUTPUT);

  textArea1.SelectFont(System5x7, BLACK);
  textArea1.DefineArea(0, 0, 29, 16, SCROLL_UP);
  textArea2.SelectFont(Wendy3x5, BLACK);
  textArea2.DefineArea(0, 20, 29, 63, SCROLL_UP);

  dht.begin();
}

void loop() {

  if (mmpos > 97){
    GLCD.CursorTo(6,3);
    GLCD.println("Starting...");
  }



  if(pm2 < millis()){// read temp every 2 seconds-ish

      //float th = dht.readHumidity();
    float tt = dht.readTemperature();

    //if (isnan(th) || isnan(tt)) {
    if (isnan(tt)) {
      // Serial.println("Failed to read from DHT sensor!");
      fail = true;

    }
    else{
      //h = th;
      fail = false;
      t = tt;
      taav+=t;
      tcav++;

    }
    pm2=millis()+2000;
  }
  
    if(pm1 < millis()){// heartbeat blinky and mode 0 display
    pm1=millis()+800;
    if(true){ //todo modes

    
      if (fail){
        textArea1.CursorTo(0,0);
        textArea1.println("Fail");
        textArea1.println("Read");

        textArea1.CursorTo(4,1);
        if(cani){
          textArea1.println("!");
          analogWrite(0, 255);
        }
        else{
          textArea1.println(" ");
          analogWrite(0, 127);
        }
      }
      else{
        textArea1.CursorTo(0,0);
        textArea1.println("Now");
        textArea1.print(t, 1);
        textArea1.println("C");
        textArea1.CursorTo(3,0);
        analogWrite(0, 255);
        if(cani)
          textArea1.println(":");
        else
          textArea1.println(" ");
      }
      cani = !cani;
    }
  }
  


  if(pm4 < millis()){
    pm4=millis()+42187;
    
    GLCD.DrawLine(0,17,29,17,WHITE);
    if (mmpos <= 97){
      GLCD.CursorTo(8,1);
      GLCD.print("Working...");
    }

    if(fail && t == 0){
      t = fram.read8(0);
      t = (t*100) + fram.read8(4096);
      t = (float)t/100;
    }

    for(int16_t i=4095; i>0; i--) {
      uint8_t v1 = fram.read8(i-1);
      fram.writeEnable(true);
      fram.write8(i, v1);
      fram.writeEnable(false);

      uint8_t v2 = fram.read8((i+4096)-1);
      fram.writeEnable(true);
      fram.write8(i+4096, v2);
      fram.writeEnable(false);

      byte xm;
      if (mmpos <= 97){
        xm = map(i,4095,0,30,127);
        GLCD.SetDot(xm,17,lframe+254);
      }
      else{
        xm = map(i,4095,0,0,127);
        analogWrite(0, map(i,4095,0,100,255));
        GLCD.SetDot(xm,34,lframe+254);

      }
      lframe = xm%2;
    }

    float temptemp;
    int8_t byteA; 
    int8_t byteB;
    int workingW;

    if(taav== 0 || tcav == 0){
      temptemp = t;
    }
    else{
      temptemp = (taav/tcav);
    }

    byteA = temptemp; //force temptemp into byteA
    // convert two decimal places to a byte:
    workingW = temptemp*100; //times by 100
    workingW = workingW%100; //get the modulo by 100 
    byteB = workingW; //store answer in byteB
    //both ByteA and ByteB can now be stored
    fram.writeEnable(true);
    fram.write8(0x0, int8_t(byteA));
    fram.writeEnable(false);
    fram.writeEnable(true);
    fram.write8(0x1000, int8_t(byteB));
    fram.writeEnable(false);

    taav = 0;
    tcav = 0;

    if(mmpos > 0)
      mmpos--;

    graphupdate();
    graphdisplay();

    //pm4=millis()+42188;
  }
  
  int progdot = map((pm4-millis()),0,42187,0,29);
  GLCD.SetDot(progdot,17,((progdot+1)%2)+254);

  buttons();
  delay(1);

}

void graphupdate(){
  int zl;

  switch (xRange) {
  case 0:
    zl=98;
    zoomstr = "1:1";
    break;

  case 1:
    zl=170;
    zoomstr = "2 Hours";
    break;

  case 2:
    zl=255;
    zoomstr = "3 Hours";
    break;

  case 3:
    zl=511;
    zoomstr = "6 Hours";
    break;

  case 4:
    zl=1023;
    zoomstr = "1/2 day";
    break;

  case 5:
    zl=2047;
    zoomstr = "1 day";
    break;

  case 6:
    zl=4095;
    zoomstr = "2 days";
    break;

  }

  for(int i = 0; i < 99; i++){
    int mz = map(i,0,98,0,zl);
    int8_t byteAr = fram.read8(mz);
    int8_t byteBr = fram.read8((mz+4096));
    int workingR = (byteAr*100)+byteBr;
    float output = (float)workingR/100;
    byte mi = map(i,0,98,98,0);
    ta[mi] = output;
  }

  //  for(int i = 0; i < 99; i++){
  //    int8_t byteAr = fram.read8(i);
  //    int8_t byteBr = fram.read8((i+4096));
  //    int workingR = (byteAr*100)+byteBr;
  //    float output = (float)workingR/100;
  //    byte mi = map(i,0,98,98,0);
  //    ta[mi] = output;
  //  }

}

void graphdisplay(){

  float mint = 99;
  float maxt = 0;

  for(int i = 0; i < 99; i++){
    maxt = max(ta[i],maxt);
    mint = min(ta[i],mint);
  }
  
  

  if (true){ //todo modes
    textArea2.CursorToXY(0,0);
    textArea2.println("X Scale");
    textArea2.CursorToXY(1,6);
    textArea2.println(zoomstr);

    textArea2.CursorToXY(0,14);
    textArea2.println("Y Scale");
    textArea2.CursorToXY(1,20);
    textArea2.println("Max");
    textArea2.CursorToXY(2,26);
    textArea2.print(maxt, 2);
    textArea2.println("c");

    textArea2.CursorToXY(1,32);
    textArea2.println("Min");
    textArea2.CursorToXY(2,38);
    textArea2.print(mint, 2);
    textArea2.println("c");
  }



  int dmax = maxt*100;
  int dmin = mint*100;

  for(int i = 0; i < 99; i++){
    GLCD.DrawLine( (30+i), 0, (30+i), 63, WHITE);

    int dot = map((ta[i]*100),dmin,dmax,63,0);

    if(dot < 64 && dot > -1){

      GLCD.DrawLine( (30+i), dot, (30+i), 63, BLACK);

    }

  }

}

void buttons(){
  bluebutton.update();
  graybutton.update();
  greenbutton.update();
  if ( bluebutton.fallingEdge()){
    digitalWrite(6, HIGH);
    xRange+=6;
    xRange = xRange%7;
    graphupdate();
    graphdisplay();
  }

  if ( graybutton.fallingEdge()){
    digitalWrite(6, HIGH);
  }

  if ( greenbutton.fallingEdge()){
    digitalWrite(6, HIGH);
    xRange++;
    xRange = xRange%7;
    graphupdate();
    graphdisplay();
  }

  digitalWrite(6, LOW);

}
