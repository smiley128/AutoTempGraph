#include <SPI.h>
#include "Adafruit_FRAM_SPI.h"
#include "DHT.h"
//#include <glcd.h>
#include "openGLCD.h"
#include "fonts/allFonts.h"

uint8_t FRAM_CS = 45;
uint8_t FRAM_SCK= 42;
uint8_t FRAM_MISO = 43;
uint8_t FRAM_MOSI = 44;
//Or use software SPI, any pins!
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
//float data[5760];
gText textArea;
//float maxt;
//float mint = 99;
float taav;
float tcav;
int mmpos = 98;
boolean cani = true;
boolean fail = false;
boolean lframe = true;


void setup() {
  	
  pinMode(0, OUTPUT);
  analogWrite(0, 20);
  //Serial.begin(9600); 
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

  //GLCD.println("Starting");
  textArea.SelectFont(System5x7, BLACK);
  textArea.DefineArea(0, 0, 29, 63, SCROLL_UP);
  //textArea.DefineArea(0, 0, 29, 63);
  dht.begin();
}

void loop() {

  if (mmpos > 97){
    GLCD.CursorTo(6,3);
    GLCD.println("Starting...");
  }

  if(pm2 < millis()){// read temp every 2 seconds

      //float th = dht.readHumidity();
    float tt = dht.readTemperature();

    //if (isnan(th) || isnan(tt)) {
    if (isnan(tt)) {
      // Serial.println("Failed to read from DHT sensor!");
      fail = true;
      textArea.CursorTo(0,0);
      textArea.println("Fail");
      textArea.println("Read");
    }
    else{
      //h = th;
      fail = false;
      t = tt;
      taav+=t;
      tcav++;
      textArea.CursorTo(0,0);
      textArea.println("Now");
      textArea.print(t, 1);
      textArea.println("C");
    }
    pm2=millis()+2000;
  }

  if(pm4 < millis()){
    pm4=millis()+42188;
    if (mmpos <= 97){
      GLCD.CursorTo(7,1);
      GLCD.print("Working...");
    }

    if(fail && t == 0){
      t = fram.read8(0);
      t = t + ((float)fram.read8(4096)/100);
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
        GLCD.SetDot(xm,16,lframe+254);
      }
      else{
        xm = map(i,4095,0,0,127);
        analogWrite(0, map(i,4095,0,20,255));
        GLCD.SetDot(xm,32,lframe+254);

      }
      lframe = xm%2;
    }


    //    for(int i=1; i<99; i++) {
    //      ta[i-1] = ta[i];
    //    }

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

  if(pm1 < millis()){// heartbeat blinky 
    pm1=millis()+1000;
    if (fail){
      textArea.CursorTo(4,1);
      if(cani){
        textArea.println("!");
        analogWrite(0, 255);
      }
      else{
        textArea.println(" ");
        analogWrite(0, 127);
      }
    }
    else{
      textArea.CursorTo(3,0);
      analogWrite(0, 255);
      if(cani)
        textArea.println(":");
      else
        textArea.println(" ");
    }
    cani = !cani;
  }

  delay(1);

}

void graphupdate(){

  for(int i = 0; i < 99; i++){
    int8_t byteAr = fram.read8(i);
    int8_t byteBr = fram.read8((i+4096));
    int workingR = (byteAr*100)+byteBr;
    float output = (float)workingR/100;
    byte mi = map(i,0,98,98,0);
    ta[mi] = output;
  }

}

void graphdisplay(){

  float mint = 99;
  float maxt = 0;

  for(int i = 0; i < 99; i++){
    maxt = max(ta[i],maxt);
    mint = min(ta[i],mint);
  }

  textArea.CursorTo(0,3);
  textArea.println("Y max");
  textArea.print(maxt, 2);
  textArea.println("");
  textArea.CursorTo(0,6);
  textArea.println("Y min");
  textArea.print(mint, 2);
  textArea.println("");

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
