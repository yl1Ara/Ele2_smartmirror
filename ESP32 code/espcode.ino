#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <SPI.h>
#include <TimeLib.h> // esp32 inner clock setup
//setTime(9, 27, 05, 14, 07, 2015);

#include "pitches.h" // pitches for musical buzzer

//sYMBOls for weather
#include <PNGdec.h>
#include "sunny.h" // Image is stored here in an 8-bit array
#include "rain.h" // Image is stored here in an 8-bit array
#include "snow.h" // Image is stored here in an 8-bit array
#include "cloudy.h" // Image is stored here in an 8-bit array
#include "half_sunny.h" // Image is stored here in an 8-bit array
#include "fog.h" // Image is stored here in an 8-bit array
#include "thunder.h" // Image is stored here in an 8-bit array

int16_t rc;


PNG png; // PNG decoder instance

#define MAX_IMAGE_WIDTH 480 // Adjust for your images
uint16_t lineBuffer[MAX_IMAGE_WIDTH];

int16_t xpos = 410;
int16_t ypos = 0;


/////////////////////////////////////////

//buzzer
#define BUZZER_PIN 16


int melody[] = {
  //1
  NOTE_G3,REST, NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,
  NOTE_G3,REST,NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,

  //5
  NOTE_G3,REST, NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,
  NOTE_G3,REST,NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,

  //9
  REST, NOTE_B3, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_A3,
  NOTE_G3, NOTE_FS3, NOTE_E3,REST,
  REST, REST, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_A3,
  NOTE_D4, REST, REST,

  //13
  REST, NOTE_B3, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_A3,
  NOTE_G3, NOTE_FS3, NOTE_E3,REST,
  REST, NOTE_B3, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_A3,
  NOTE_D4, REST, REST,

  //17
  NOTE_E4, REST, NOTE_B3,
  NOTE_B3, REST,NOTE_G3, NOTE_A3,
  NOTE_B3, NOTE_B3, NOTE_A3, NOTE_G3, NOTE_A3, NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_A3, NOTE_B3, NOTE_B3, NOTE_B3,

  //21
  NOTE_E4,REST, NOTE_B3,
  NOTE_B3,REST, NOTE_G3, NOTE_A3,
  NOTE_B3, NOTE_B3, NOTE_A3, NOTE_G3, NOTE_A3, NOTE_G3, NOTE_E3,
  NOTE_A3, NOTE_B3, NOTE_A3, NOTE_G3,

  //25
  NOTE_G3,REST, NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,
  NOTE_G3,REST,NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,

  //29
  NOTE_G3,REST, NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,
  NOTE_G3,REST,NOTE_G3, NOTE_A3, NOTE_G3,
  NOTE_FS3, NOTE_E3, NOTE_D3, NOTE_C3,
};

int durations[] = {
  //1
  4, 8, 8, 4, 4,
  8, 8, 4, 2,
  4, 8, 8, 4, 4,
  8, 8, 4, 2,

  //5
  4, 8, 8, 4, 4,
  8, 8, 4, 2,
  4, 8, 8, 4, 4,
  8, 8, 4, 2,

  //9
  8,8,8,4,8,8,4,
  8,8,2,8,
  4,8,4,8,8,4,
  4,8,2,

  //13
  4,8,4,8,8,4,
  8,8,2,8,
  8,8,8,4,8,8,4,
  2,8,4,

  //17
  2,4, 4,
  2,4,8,8,
  8,8,8,8,8,8,8,8,
  4,8,8,2,

  //21
  2,4, 4,
  2,4,8,8,
  8,8,8,8,8,8,4,
  4,4,4,4,  

  //25
  4, 8, 8, 4, 4,
  8, 8, 4, 2,
  4, 8, 8, 4, 4,
  8, 8, 4, 2,

  //29
  4, 8, 8, 4, 4,
  8, 8, 4, 2,
  4, 8, 8, 4, 4,
  8, 8, 4, 2,
};

//DHT11
#include <Arduino.h>
#include "DHT_Async.h"
#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 17;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
float temperature;
float humidity;

//HTTP POST
long postDelay=600000;
long postTimer;

// myObject.keys() can be used to get an array of all the keys in the object
JSONVar myObject;

#include <TFT_eSPI.h>

const char* ssid = "Trtset";//"AarosIphone";//"Trtset";
const char* password = "foms2700";//"abcdabcd";//"foms2700";

TFT_eSPI tft = TFT_eSPI();  

//Your Domain name with URL path or IP address with path
const char* serverName = "http://www.aaroesko.xyz/main_data?Kumpula";

//pins
int BL=1;
int tftonb= 18;
int tftframeb= 37;
int rstb= 38;

//7s pins
int kello = 19;
int latch = 20;
int datap = 21;
int num[4] = {33,34,35,36};
int numer[4];

//bl value
int BLvalue=1;

//rst pin
int rstpin=45;

//frame indicator
int frame = 0;

//interrupt debounce
long intdeb = 0;
long debouncetime =200;

//timers
unsigned long lastTime = 0;
unsigned long timerDelay = 60000;

//Json data from server
String data;

//OLED update times
long updatetimer=-1000;
long updatetime=60000;

//initial layout for screen
void drawInitials(){
/// bussit
      tft.setCursor(160,207);
      tft.setTextSize(3);
      tft.println("DEPARTURES");

/// notes 
      tft.setCursor(0,0);
      tft.setTextSize(3);
      tft.println("NOTE/ALARM");



/// weather 
      tft.setCursor(247,0);
      tft.setTextSize(3);
      tft.println("WEATHER");

/// viivat
      tft.fillRect(0,200,480,5,TFT_WHITE);   //   x  y   width  height color 
      tft.fillRect(240,0,5,200,TFT_WHITE);//pystyviiva
}

void screeonoff(){
  if (millis()-intdeb>debouncetime){
    BLvalue = !BLvalue;
    digitalWrite(BL,BLvalue);
    intdeb=millis();
  }
}

void updatescreen(){
  updatetimer=millis();
    if (frame==0){
        notes();
        bus();
        Weather();
      }
    if (frame==1){
      onlyWeather();
    }
    if (frame==2){
      onlyNotes();
    }
    if (frame==3){
      onlyBus();
    }
}

void framechange(){
  if (millis()-intdeb>debouncetime){
    frame=(frame+1)%4;
    intdeb=millis();
    tft.fillScreen(TFT_BLACK);
    if (frame==0){
      drawInitials();
    }
    else if (frame==1){
      tft.setCursor(0,0);
      tft.setTextSize(3);
      tft.println("Weather");
    }
    else if (frame==2){
      tft.setCursor(0,0);
      tft.setTextSize(5);
      tft.println("Notes");

      tft.setCursor(0,160);
      tft.setTextSize(5);
      tft.println("Alarms:");
    }
    else if (frame==3){
      tft.setCursor(0,0);
      tft.setTextSize(5);
      tft.println("Departures");
    }
    updatescreen();
  }
}

void(* resetFunc) (void) = 0;

void rstfunc(){
  resetFunc();
}

void setup() {
  Serial.begin(115200);

  //7s
  pinMode(kello, OUTPUT);
  pinMode(latch, OUTPUT);
  pinMode(datap, OUTPUT);

  pinMode(num[0], OUTPUT);
  pinMode(num[1], OUTPUT);
  pinMode(num[2], OUTPUT);
  pinMode(num[3], OUTPUT);

  //Backlight of screen
  pinMode(BL,OUTPUT);
  digitalWrite(BL,BLvalue);

  //rstpin
  //pinMode(rstpin, OUTPUT);
  //digitalWrite(rstpin, HIGH);

  //button reading:
  pinMode(tftonb, INPUT_PULLUP);
  pinMode(tftframeb, INPUT_PULLUP);
  pinMode(rstb, INPUT_PULLUP);

  //interrupts
  attachInterrupt(tftonb, screeonoff, RISING);
  attachInterrupt(tftframeb, framechange, RISING);
  attachInterrupt(rstb, rstfunc, RISING);

  //buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  //init oled
  tft.begin();
  tft.setRotation(3);

  //for wifi connect prints
  tft.setCursor(180,50); 
  tft.setTextSize(2);

  //wifi connection
  WiFi.begin(ssid, password);
  tft.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }
  tft.println("");
  tft.print("Connected to WiFi network with IP Address: ");
  tft.println(WiFi.localIP());
  delay(400);
 
  //Make backscreen uniformally black
  tft.fillScreen(TFT_BLACK);

  //draws initial layout
  drawInitials();

  // Set initial time
  data = httpGETRequest(serverName);
  myObject = JSON.parse(data);

  setTime(myObject["time_data"]["hours"], myObject["time_data"]["mins"], myObject["time_data"]["sec"], myObject["time_data"]["day"], myObject["time_data"]["month"], myObject["time_data"]["year"]);

  updatescreen();

}


// Draws pictures
void pngDraw(PNGDRAW *pDraw) {
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}


void Weather(){
  //weather update black box
  tft.fillRect(247,37,233,40,TFT_BLACK);//   x  y   width  height color

  xpos = 410;
  ypos = 0;

  //import and print data
  String temp=myObject["weather_data"]["temperature_2m"];
  String weather=myObject["weather_data"]["weather_code"];

  if (weather == "Sunny"){
    rc = png.openFLASH((uint8_t *)sunny, sizeof(sunny), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode
    }
  }
  else if (weather == "Rainy"){
    rc = png.openFLASH((uint8_t *)rain, sizeof(rain), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode
    }
  }
  else if (weather == "Cloudy"){
    rc = png.openFLASH((uint8_t *)cloudy, sizeof(cloudy), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode
    }
  }
  else if (weather == "Half cloudy"){
    rc = png.openFLASH((uint8_t *)half_sunny, sizeof(half_sunny), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode

    }
  }
  else if (weather == "Snowy"){
    rc = png.openFLASH((uint8_t *)snow, sizeof(snow), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode
    }
  }
  else if (weather == "Thunder"){
    rc = png.openFLASH((uint8_t *)thunder, sizeof(thunder), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode
    }
  }
  else if (weather == "Fog"){
    rc = png.openFLASH((uint8_t *)fog, sizeof(fog), pngDraw);
    if (rc == PNG_SUCCESS) {
      tft.startWrite();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      png.close(); // not needed for memory->memory decode
    }
  }
  else if (weather == "Unknown"){
    tft.fillRect(410,0,60,60,TFT_BLACK);
  }
      
  //set print place to the right box
  tft.setCursor(247,37); 
  tft.setTextSize(2);

  tft.print(temp);
  tft.println(" C ");
  tft.setCursor(247,57);
  tft.print(weather);
}

void onlyWeather(){
  String weathercodes[4]={myObject["secondary_weather_data"]["0"]["weather_code"],myObject["secondary_weather_data"]["3"]["weather_code"],myObject["secondary_weather_data"]["6"]["weather_code"],myObject["secondary_weather_data"]["9"]["weather_code"]};
  String times[4]={myObject["secondary_weather_data"]["0"]["date"],myObject["secondary_weather_data"]["3"]["date"],myObject["secondary_weather_data"]["6"]["date"],myObject["secondary_weather_data"]["9"]["date"]};
  int rainprob[4]={myObject["secondary_weather_data"]["0"]["precipitation_probability"],myObject["secondary_weather_data"]["3"]["precipitation_probability"],myObject["secondary_weather_data"]["6"]["precipitation_probability"],myObject["secondary_weather_data"]["9"]["precipitation_probability"]};
  String windspeed[4]={myObject["secondary_weather_data"]["0"]["hourly_wind_speed_10m"],myObject["secondary_weather_data"]["3"]["hourly_wind_speed_10m"],myObject["secondary_weather_data"]["6"]["hourly_wind_speed_10m"],myObject["secondary_weather_data"]["9"]["hourly_wind_speed_10m"]};

  for (int i =0; i<4;i++){
    xpos =(480-60-10-70)/3*i;
    ypos = 50;


    String weather=weathercodes[i];
    if (weather == "Sunny"){
      rc = png.openFLASH((uint8_t *)sunny, sizeof(sunny), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
      }
    }
    else if (weather == "Rainy"){
      rc = png.openFLASH((uint8_t *)rain, sizeof(rain), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
      }
    }
    else if (weather == "Cloudy"){
      rc = png.openFLASH((uint8_t *)cloudy, sizeof(cloudy), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
      }
    }
    else if (weather == "Half cloudy"){
      rc = png.openFLASH((uint8_t *)half_sunny, sizeof(half_sunny), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode

      }
    }
    else if (weather == "Snowy"){
      rc = png.openFLASH((uint8_t *)snow, sizeof(snow), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
      }
    }
    else if (weather == "Thunder"){
      rc = png.openFLASH((uint8_t *)thunder, sizeof(thunder), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
      }
    }
    else if (weather == "Fog"){
      rc = png.openFLASH((uint8_t *)fog, sizeof(fog), pngDraw);
      if (rc == PNG_SUCCESS) {
        tft.startWrite();
        rc = png.decode(NULL, 0);
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
      }
    }

    //times on top of symbols
    tft.setCursor(xpos,30);
    tft.setTextSize(2);
    tft.print(times[i]);

    //rain probability printed
    tft.setCursor(xpos,115);
    tft.setTextSize(2);
    tft.print("rain%:");
    tft.setTextSize(2);
    tft.print(rainprob[i]);
    
    //wind printed
    tft.setCursor(xpos,135);
    tft.setTextSize(2);
    tft.print("wind:");
    tft.setTextSize(2);
    tft.print(windspeed[i]);
  }

  String weathercodes2[2]={myObject["secondary_weather_data"]["weather_plus1"]["weather_code"],myObject["secondary_weather_data"]["weather_plus2"]["weather_code"]};
  String times2[2]={myObject["secondary_weather_data"]["weather_plus1"]["date"],myObject["secondary_weather_data"]["weather_plus2"]["date"]};
  String maxwind[2]={myObject["secondary_weather_data"]["weather_plus1"]["daily_wind_speed_10m_max"],myObject["secondary_weather_data"]["weather_plus2"]["daily_wind_speed_10m_max"]};
  String maxrainprob[2]={myObject["secondary_weather_data"]["weather_plus1"]["daily_precipitation_probability_max"],myObject["secondary_weather_data"]["weather_plus2"]["daily_precipitation_probability_max"]};
  String maxtemp[2]={myObject["secondary_weather_data"]["weather_plus1"]["temperature_2m_max"],myObject["secondary_weather_data"]["weather_plus2"]["temperature_2m_max"]};

  for (int i =0; i<2;i++){
    xpos =50;
    ypos = 165+70*i;
  
    //time
    tft.setCursor(0,ypos+30);
    tft.setTextSize(2);
    tft.print(times2[i]);

    //weathercode
    tft.setCursor(60,ypos+30);
    tft.print(weathercodes2[i]);

    //rain probability printed
    tft.setCursor(160,ypos+30);
    tft.print("rain%:");
    tft.print(maxrainprob[i]);
    
    //wind printed
    tft.setCursor(300,ypos+30);
    tft.print("wind:");
    tft.println(maxwind[i]);
    

  }
  
  
}

//print events 
void printEvents(int xposE,int yposE, String* meet, int textsize){
  tft.setTextSize(textsize);

  tft.setCursor(xposE,yposE);
  tft.print("ALARM: ");
  tft.print(meet[1]);
  tft.print(":");
  if (meet[2].length()==1){
    meet[2]="0" + meet[2];
  }
  tft.print(meet[2]);

  if (meet[0].toInt()!=day() || meet[3].toInt()!=day()){
    int daggen=meet[0].toInt()-day()+(meet[3].toInt()-month())*30+(meet[4].toInt()-year())*365;

    int p=meet[4].toInt()-month();

    tft.print(", ");
    tft.print(daggen);
    //tft.print("days");
  }
}

void notes(){
  tft.fillRect(0,37,240,163,TFT_BLACK);//   x  y   width  height color

  String note1 = myObject["note"]["note3"];

  String meeting1[5] = {myObject["events"]["event1"]["day"],myObject["events"]["event1"]["hour"],myObject["events"]["event1"]["min"],myObject["events"]["event1"]["month"],myObject["events"]["event1"]["year"]};
  String meeting2[5] = {myObject["events"]["event2"]["day"],myObject["events"]["event2"]["hour"],myObject["events"]["event2"]["min"],myObject["events"]["event2"]["month"],myObject["events"]["event2"]["year"]};
  String meeting3[5] = {myObject["events"]["event3"]["day"],myObject["events"]["event3"]["hour"],myObject["events"]["event3"]["min"],myObject["events"]["event3"]["month"],myObject["events"]["event3"]["year"]};
  
  //note3
  tft.setCursor(0,37);   // 
  tft.setTextSize(2);
  tft.println(note1);

  tft.setCursor(160,75);
  tft.setTextSize(2);
  //tft.println("days:");

  printEvents(0,90 , meeting1, 2);
  printEvents(0,110, meeting2, 2);
  printEvents(0,130, meeting3, 2);
}

void onlyNotes(){
  String note1 = myObject["note"]["note3"];
  String note2 = myObject["note"]["note2"];
  String note3 = myObject["note"]["note1"];

  tft.setCursor(0,50);   // 
  tft.setTextSize(4);

  tft.print("-");
  tft.println(note3);
  tft.print("-");
  tft.println(note2);
  tft.print("-");
  tft.println(note1);

  //calendar:
  String meeting1[5] = {myObject["events"]["event1"]["day"],myObject["events"]["event1"]["hour"],myObject["events"]["event1"]["min"],myObject["events"]["event1"]["month"],myObject["events"]["event1"]["year"]};
  String meeting2[5] = {myObject["events"]["event2"]["day"],myObject["events"]["event2"]["hour"],myObject["events"]["event2"]["min"],myObject["events"]["event2"]["month"],myObject["events"]["event2"]["year"]};
  String meeting3[5] = {myObject["events"]["event3"]["day"],myObject["events"]["event3"]["hour"],myObject["events"]["event3"]["min"],myObject["events"]["event3"]["month"],myObject["events"]["event3"]["year"]};

  tft.setCursor(230,200);
  tft.setTextSize(2);
  tft.println("days:");

  printEvents(0,230 , meeting1, 3);
  printEvents(0,260, meeting2, 3);
  printEvents(0,290, meeting3, 3);
}

void bus(){
  //tft.fillRect(0,239,470,20,TFT_BLACK);//   x  y   width  height color
  //buss short name (like 57, 8 etc.)
  String buss1Snam = myObject["public_transport_data"]["0"]["shortname"];
  String buss2Snam = myObject["public_transport_data"]["1"]["shortname"];
  String buss3Snam = myObject["public_transport_data"]["2"]["shortname"];

  //buss departure
  String buss1dep = myObject["public_transport_data"]["0"]["departure"];
  String buss2dep = myObject["public_transport_data"]["1"]["departure"];
  String buss3dep = myObject["public_transport_data"]["2"]["departure"];

   
  tft.setTextSize(4);
  tft.setCursor(0,239);
  tft.println(buss1Snam);
  tft.setCursor(190,239);
  tft.println(buss2Snam);
  tft.setCursor(380,239);
  tft.println(buss3Snam);
  tft.setTextSize(2);
  tft.setCursor(0,279);
  tft.println(buss1dep);
  tft.setCursor(190,279);
  tft.println(buss2dep);
  tft.setCursor(380,279);
  tft.println(buss3dep);

}

void onlyBus(){
  //bus short name (like 57, 8 etc.)
  String buss1Snam = myObject["public_transport_data"]["0"]["shortname"];
  String buss2Snam = myObject["public_transport_data"]["1"]["shortname"];
  String buss3Snam = myObject["public_transport_data"]["2"]["shortname"];

  //bus departure
  String buss1dep = myObject["public_transport_data"]["0"]["departure"];
  String buss2dep = myObject["public_transport_data"]["1"]["departure"];
  String buss3dep = myObject["public_transport_data"]["2"]["departure"];

  //bus heading
  String buss1head = myObject["public_transport_data"]["0"]["headsign"];
  String buss2head = myObject["public_transport_data"]["1"]["headsign"];
  String buss3head = myObject["public_transport_data"]["2"]["headsign"];

  //shortnames
  tft.setTextSize(5);
  tft.setCursor(0,50);
  tft.println(buss1Snam);
  tft.setCursor(0,130);
  tft.println(buss2Snam);
  tft.setCursor(0,210);
  tft.println(buss3Snam);

  //departure times
  tft.setTextSize(3);
  tft.setCursor(220,80);
  tft.println(buss1dep);
  tft.setCursor(220,160);
  tft.println(buss2dep);
  tft.setCursor(220,240);
  tft.println(buss3dep);

  //heading
  tft.setTextSize(2);
  tft.setCursor(120,50);
  tft.println(buss1head);
  tft.setCursor(120,130);
  tft.println(buss2head);
  tft.setCursor(120,210);
  tft.println(buss3head);

}

//functions for clock//////////////////////////////////////////////////////////////////////////////////////

//numbers to bit
int numtobit(int num){
  switch (num){
    case 1:
      return B00100010;
    case 2:
      return B11001110;
    case 3:
      return B11101010;
    case 4:
      return B01100011;
    case 5:
      return B11101001;
    case 6:
      return B11101101;
    case 7:
      return B10100010;
    case 8:
      return B11101111;
    case 9:
      return B11101011;
    case 0:
      return B10101111;
  }
}

//numbers to bit with dot
int numtobit2(int num){
  switch (num){
    case 1:
      return B00110010;
    case 2:
      return B11011110;
    case 3:
      return B11111010;
    case 4:
      return B01110011;
    case 5:
      return B11111001;
    case 6:
      return B11111101;
    case 7:
      return B10110010;
    case 8:
      return B11111111;
    case 9:
      return B11111011;
    case 0:
      return B10111111;
  }
}

//parses hours and minutes to 10s and 1s
void klockan(){
  int h=hour();
  int m=minute(); 
  //hours
  numer[0]=numtobit(h/10);
  numer[1]=numtobit2(h%10);
  //minutes
  numer[2]=numtobit(m/10);
  numer[3]=numtobit(m%10);
}

//the main clock func where the data is changed on the 7 segment 4-digit display.
void clockupdate(){
  klockan();
  for (int j = 0; j <= 3; j++) {
    digitalWrite(num[j], LOW);
    digitalWrite(latch, LOW);
    shiftOut(datap, kello, LSBFIRST, numer[j]);
    digitalWrite(latch, HIGH);
    DHT11data();
    delay(1);
    digitalWrite(num[j], HIGH);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

void musicalbuzzer(String* alarmName){
    int size = sizeof(durations) / sizeof(int);

    tft.fillScreen(TFT_BLACK);
    printEvents(40,120, alarmName, 5);

  for (int note = 0; note < size; note++) {
    //to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int duration = 2*1000 / durations[note];
    tone(BUZZER_PIN, melody[note], duration);

    if (digitalRead(tftonb)==1){
      lastTime=millis();
      break;
    }
    else if (digitalRead(tftframeb)==1){
      lastTime=millis();
      break;
    }

    //to distinguish the notes, set a minimum time between them.
    //the note's duration + 30% seems to work well:
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);

    //stop the tone playing:
    noTone(BUZZER_PIN);
  }
}

static bool measure_environment(float *temperature, float *humidity) {
    static unsigned long measurement_timestamp = millis();

    /* Measure once every four seconds. */
    if (millis() - measurement_timestamp > 4000ul) {
        if (dht_sensor.measure(temperature, humidity)) {
            measurement_timestamp = millis();
            return (true);
        }
    }

    return (false);
}

void DHT11data(){
    /* Measure temperature and humidity.  If the functions returns
       true, then a measurement is available. */
  if (measure_environment(&temperature, &humidity)) {
    if (frame==0){
      tft.setCursor(247,90);
      tft.setTextSize(2);
      tft.print("Roomtemp: ");
      tft.print(temperature, 1);
      tft.println(" C");
      tft.setCursor(247,120);
      tft.print("Humidity: ");
      tft.print(humidity, 1);
      tft.println("%");
    } 
  }
}


void checkAlarm(){
  String meeting1[5] = {myObject["events"]["event1"]["day"],myObject["events"]["event1"]["hour"],myObject["events"]["event1"]["min"],myObject["events"]["event1"]["month"],myObject["events"]["event1"]["year"]};
  String meeting2[5] = {myObject["events"]["event2"]["day"],myObject["events"]["event2"]["hour"],myObject["events"]["event2"]["min"],myObject["events"]["event2"]["month"],myObject["events"]["event2"]["year"]};
  String meeting3[5] = {myObject["events"]["event3"]["day"],myObject["events"]["event3"]["hour"],myObject["events"]["event3"]["min"],myObject["events"]["event3"]["month"],myObject["events"]["event3"]["year"]};

  if (meeting1[0].toInt() == day() && meeting1[1].toInt() == hour() && meeting1[2].toInt() == minute() && meeting1[3].toInt() == month() && meeting1[4].toInt() == year() ) {
    myObject["events"]["event1"]["day"]=0;
    musicalbuzzer(meeting1);
  }
  else if (meeting2[0].toInt() == day() && meeting2[1].toInt() == hour() && meeting2[2].toInt() == minute() && meeting2[3].toInt() == month() && meeting2[4].toInt() == year() ) {
    myObject["events"]["event1"]["day"]=0;
    musicalbuzzer(meeting2);
  }
  else if (meeting3[0].toInt() == day() && meeting3[1].toInt() == hour() && meeting3[2].toInt() == minute() && meeting3[3].toInt() == month() && meeting3[4].toInt() == year() ) {
    myObject["events"]["event1"]["day"]=0;
    musicalbuzzer(meeting3);
  }
}

void loop() {
  if (BLvalue==1){
    clockupdate();
  }

  checkAlarm();
 
  if ((millis()-updatetimer)>updatetime){
    updatescreen();
  }

  if ((millis() - lastTime) > timerDelay) {
    loopAPIGet();
  }

  if ((millis()-postTimer)>5000){
    postFunc();
  }
}

void postFunc(){
  // Specify content-type header
  if(WiFi.status()== WL_CONNECTED){
    const char* serverName2 = "http://www.aaroesko.xyz/DHT_data";
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverName2);

    http.addHeader("Content-Type", "application/json");

    // Data to send with HTTP POST
    String httpRequestData = "{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"" + String(temperature) + "\",\"value2\":\"" + String(humidity) + "\",\"value3\":\"1005.14\"}";
    //httpRequestData += ",\"value1\":\" + String(temperature) + ""\",";
    //httpRequestData += ",\"value2\":\" + String(humidity) + "\"}"";
    //httpRequestData += "&value2=" + String(humidity);

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);

    Serial.println(httpResponseCode);

    http.end();
  }
  else {
      Serial.println("WiFi Disconnected");
  }
  postTimer=millis();
  
}

void loopAPIGet(){
      //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      data = httpGETRequest(serverName);
      myObject = JSON.parse(data);
      
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.print("not connected");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
