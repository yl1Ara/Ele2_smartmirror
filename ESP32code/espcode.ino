//Made by ( ͡° ͜ʖ ͡°) tech. esp32 coding and 3d modelling expert Arttu Yli-Kujala

//for clarification I usually shorten 4-digit 7-segment display as 7s for simplicity

//libraries
  #include <WiFi.h>
  #include <HTTPClient.h> // these two are basic libraries to get API requests to work on esp32s2
  #include <Arduino_JSON.h> // The API that we get from server is in json form, so we want to use this library
  #include <SPI.h> 
  #include <TimeLib.h> // We get the clock time from the server and once per boot and the use esp32s2 inner timers to store the data
  //setTime(9, 27, 05, 14, 07, 2015);

  #include "pitches.h" // Here the different notes are stored for buzzer alarm music

//sYMBOls for weather
  #include <PNGdec.h> // library we use to draw bitmap arrays to the OLED easily:
  #include "sunny.h" // This and all the below are the bitmap 8-bit arrays for different weather symbols
  #include "rain.h" 
  #include "snow.h" 
  #include "cloudy.h" 
  #include "half_sunny.h" 
  #include "fog.h" 
  #include "thunder.h" 

  int16_t rc;


  PNG png; // PNG decoder instance

  #define MAX_IMAGE_WIDTH 480 // Adjust for your images
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];

  //make initial position for pngs
  int16_t xpos = 410;
  int16_t ypos = 0;


/////////////////////////////////////////

//buzzer
  #define BUZZER_PIN 16

//this is the alarm music that uses pitches.h library for easy writing of your own music and this music in question is "another day in paradise" - by the legendary Phil Collins
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

//This durations for the music notes that we gave befire this in melody[] and they work simply like 4=quarter note, 8=Eighth note
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
  #include <Arduino.h>  // we use these libraries to make DHT11 data gathering to be bit less time consuming, because normally it creates long(~500ns) delays in code
  #include "DHT_Async.h" //^^^^^^^^^
  #define DHT_SENSOR_TYPE DHT_TYPE_11 
  static const int DHT_SENSOR_PIN = 17;
  DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
  float temperature;
  float humidity;

//HTTP POST

  long postDelay=600000;
  long postTimer;
//Json data from server
  String data; // the initial data that we get from sever that is then made into a myObject jsonvar 
// Make a global variable out of the API json data:
  JSONVar myObject;

#include <TFT_eSPI.h> //library for the OLED screen display
  TFT_eSPI tft = TFT_eSPI();  

//for local wifi acces we need the wifi name and password
  const char* ssid = "Trtset";//"AarosIphone";
  const char* password = "foms2700";//"abcdabcd";/



//Server domain where we get all the API data for buss, alarms, weather, notes etc.
  const char* serverName = "http://www.aaroesko.xyz/main_data?Kumpula";

//Button pins:
  int tftonb= 18;
  int tftframeb= 37;
  int rstb= 38;

//OLED screeb backlight pin and state value
  int BL=1;
  int BLvalue=1; //also controls if the 7s is on/off
//7s pins
  int kello = 19;
  int latch = 20;
  int datap = 21;
  int num[4] = {33,34,35,36};
  int numer[4];



//rst pin
  int rstpin=45;

//frame indicator that by switching the value betwee 0-3 we get the different views on the oled
  int frame = 0;

//interrupt debounce
  long intdeb = 0; //the timer from last button push
  long debouncetime =200; //the minimum time that has to have passed before new push is registered in ns

//API request timers:
  unsigned long lastTime = 0; // the timer that keeps track of time from the last API request to the server
  unsigned long timerDelay = 60000; // The time that must pass before sending new API reques, 60000ns=1min



//OLED update times
  long updatetimer=-1000; //timer for the update that keeps track of time from last update
  long updatetime=60000; //currently update the screen once in a minute


//Here we setup the first view on the oled that is also the main screen:
void drawInitials(){
  /// buss schedule section
      tft.setCursor(160,207); //x,y position
      tft.setTextSize(3); // Textsize
      tft.println("DEPARTURES"); // Printed string

  /// notes 
      tft.setCursor(0,0);
      tft.setTextSize(3);
      tft.println("NOTE/ALARM");



  /// weather 
      tft.setCursor(247,0);
      tft.setTextSize(3);
      tft.println("WEATHER");

  /// White lines that divide main screen into these sections (x,y,width,height,color )
      tft.fillRect(0,200,480,5,TFT_WHITE);   //Horizontal line
      tft.fillRect(240,0,5,200,TFT_WHITE);  //Vertical line
}

//function to set screen on/off after we push the top button:
void screeonoff(){
  if (millis()-intdeb>debouncetime){
    BLvalue = !BLvalue;
    digitalWrite(BL,BLvalue);
    intdeb=millis();
  }
}

//function for updating the OLED screen with different information dependent of what frame value we currently have and this is in the main loop
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

//function for when the second button is pushed, then the frame value is changed and also it draw that views initial information that doesn't change every updatescreen loop
void framechange(){
  if (millis()-intdeb>debouncetime){ //debouncing
    frame=(frame+1)%4;  //frame get value between 0-3 for the 4 different views
    intdeb=millis();    // reset debonce timer
    tft.fillScreen(TFT_BLACK); //fill the screen with black so that no old data remains there 
    if (frame==0){    //draw inital data for main screen
      drawInitials();
    }
    else if (frame==1){   //draw initial data for weather screen
      tft.setCursor(0,0);
      tft.setTextSize(3);
      tft.println("Weather");
    }
    else if (frame==2){ //draw initial data for notes/alarms screen
      tft.setCursor(0,0);
      tft.setTextSize(5);
      tft.println("Notes");

      tft.setCursor(0,160);
      tft.setTextSize(5);
      tft.println("Alarms:");
    }
    else if (frame==3){   //draw initial data for buss schedule screen
      tft.setCursor(0,0);
      tft.setTextSize(5);
      tft.println("Departures");
    }
    updatescreen(); //updates the screen instantly so we don't have to wait a minute :D
  }
}


//Here we make a reset function for the third and last button in the program 
  void(* resetFunc) (void) = 0;

  void rstfunc(){
    resetFunc();
  }

void setup() {
  Serial.begin(115200); //Serial for debugging mostly server info

  //7s(4-digit 7-segment diplay) pin setup:
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



  //button pin setup
    pinMode(tftonb, INPUT_PULLUP);
    pinMode(tftframeb, INPUT_PULLUP);
    pinMode(rstb, INPUT_PULLUP);

  //interrupts for putton presses
    attachInterrupt(tftonb, screeonoff, RISING);
    attachInterrupt(tftframeb, framechange, RISING);
    attachInterrupt(rstb, rstfunc, RISING);

  //buzzer pin setup
    pinMode(BUZZER_PIN, OUTPUT);

  //OLED setup
    tft.begin();
    tft.setRotation(3);

  //for wifi connection we setup OLED print position
    tft.setCursor(180,50); 
    tft.setTextSize(2);

  //wifi connection
    WiFi.begin(ssid, password); //Setup with using wifi name and password
    tft.println("Connecting"); 
    while(WiFi.status() != WL_CONNECTED) {  //wait for connection
      delay(500);
      tft.print(".");
    }
    tft.println("");
    tft.print("Connected to WiFi network with IP Address: "); 
    tft.println(WiFi.localIP());  //print wifi information quickly after connection
    delay(400);
  
  //Make backscreen uniformally black 
    tft.fillScreen(TFT_BLACK);

  //draws initial/main screen layout
    drawInitials();

  //Set initial clock time by getting initial time data from server
    data = httpGETRequest(serverName);
    myObject = JSON.parse(data);
    //set the esp32s2 time as:
    setTime(myObject["time_data"]["hours"], myObject["time_data"]["mins"], myObject["time_data"]["sec"], myObject["time_data"]["day"], myObject["time_data"]["month"], myObject["time_data"]["year"]);

  //updates the screen as soon as possible in the setup
    updatescreen();

}


// Draws png pictures from bitmap byte arrays
void pngDraw(PNGDRAW *pDraw) {
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

//Here is the data for the weather display on the main screen
void Weather(){
  //weather update black box
  tft.fillRect(247,37,233,40,TFT_BLACK);//   x  y   width  height color

  //setup initial position for weather pngs
    xpos = 410;
    ypos = 0;

  //import data from jsonwar
  String temp=myObject["weather_data"]["temperature_2m"];
  String weather=myObject["weather_data"]["weather_code"];

  //Here is a long list for different weather png drawings
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

  //print temperature and weathercode
  tft.print(temp);
  tft.println(" C ");
  tft.setCursor(247,57);
  tft.print(weather);
}

//Here is the data for the weather only display
void onlyWeather(){
  //get all the data from the server data:
    String weathercodes[4]={myObject["secondary_weather_data"]["0"]["weather_code"],myObject["secondary_weather_data"]["3"]["weather_code"],myObject["secondary_weather_data"]["6"]["weather_code"],myObject["secondary_weather_data"]["9"]["weather_code"]};
    String times[4]={myObject["secondary_weather_data"]["0"]["date"],myObject["secondary_weather_data"]["3"]["date"],myObject["secondary_weather_data"]["6"]["date"],myObject["secondary_weather_data"]["9"]["date"]};
    int rainprob[4]={myObject["secondary_weather_data"]["0"]["precipitation_probability"],myObject["secondary_weather_data"]["3"]["precipitation_probability"],myObject["secondary_weather_data"]["6"]["precipitation_probability"],myObject["secondary_weather_data"]["9"]["precipitation_probability"]};
    String windspeed[4]={myObject["secondary_weather_data"]["0"]["hourly_wind_speed_10m"],myObject["secondary_weather_data"]["3"]["hourly_wind_speed_10m"],myObject["secondary_weather_data"]["6"]["hourly_wind_speed_10m"],myObject["secondary_weather_data"]["9"]["hourly_wind_speed_10m"]};

  //we have 4 different weather data for today that shows the weather now, 3 hours later, 6 hours later and 9 hours later so we setup a logic to print this data side by side
  for (int i =0; i<4;i++){
    //Here is the print position for each 3 hour data sets. they are all on the same ypos but xpos makes them go side by side and not on top of each other
    xpos =(480-60-10-70)/3*i;
    ypos = 50;

    //here we again have if statements for the different weather symbols
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

    
    //clock times printed on top of the weather symbols
      tft.setCursor(xpos,30);
      tft.setTextSize(2);
      tft.print(times[i]);

    //rain probability printed below the weather symbol
      tft.setCursor(xpos,115);
      tft.setTextSize(2);
      tft.print("rain%:");
      tft.setTextSize(2);
      tft.print(rainprob[i]);
      
    //wind printed below rain info
      tft.setCursor(xpos,135);
      tft.setTextSize(2);
      tft.print("wind:");
      tft.setTextSize(2);
      tft.print(windspeed[i]);
  }

  //And here we make new set of data for tommorrow and the day after that data:
    String weathercodes2[2]={myObject["secondary_weather_data"]["weather_plus1"]["weather_code"],myObject["secondary_weather_data"]["weather_plus2"]["weather_code"]};
    String times2[2]={myObject["secondary_weather_data"]["weather_plus1"]["date"],myObject["secondary_weather_data"]["weather_plus2"]["date"]};
    String maxwind[2]={myObject["secondary_weather_data"]["weather_plus1"]["daily_wind_speed_10m_max"],myObject["secondary_weather_data"]["weather_plus2"]["daily_wind_speed_10m_max"]};
    String maxrainprob[2]={myObject["secondary_weather_data"]["weather_plus1"]["daily_precipitation_probability_max"],myObject["secondary_weather_data"]["weather_plus2"]["daily_precipitation_probability_max"]};
    String maxtemp[2]={myObject["secondary_weather_data"]["weather_plus1"]["temperature_2m_max"],myObject["secondary_weather_data"]["weather_plus2"]["temperature_2m_max"]};

  for (int i =0; i<2;i++){
    //here it's basically same kinda logic for printing, but now we don't print them side-by-side column like but on list kinda row form
    xpos =50;
    ypos = 165+70*i;
  
    //time
    tft.setCursor(0,ypos+30);
    tft.setTextSize(2);
    tft.print(times2[i]);

    //weathercode as a string like sunny, cloudy etc.
    tft.setCursor(60,ypos+30);
    tft.print(weathercodes2[i]);

    //rain probability 
    tft.setCursor(160,ypos+30);
    tft.print("rain%:");
    tft.print(maxrainprob[i]);
    
    //windspeed
    tft.setCursor(300,ypos+30);
    tft.print("wind:");
    tft.println(maxwind[i]);
    

  }
  
  
}

//This is a function for printing the alarm data to the screen
void printEvents(int xposE,int yposE, String* meet, int textsize){
  tft.setTextSize(textsize);

  tft.setCursor(xposE,yposE); //position
  tft.print("ALARM: "); 
  //the meeting data is as (day, hour, minute, month, year)
  tft.print(meet[1]); //First we print the hour when the alarm is set to go off
  tft.print(":");
  //Here we make sure it prints as 05 and not 5, if alarm is set to 9 hour and 5 min mark it prints 9:05 rather thatt 9:5
    if (meet[2].length()==1){
      meet[2]="0" + meet[2];
    }
    tft.print(meet[2]);

  if (meet[0].toInt()!=day() || meet[3].toInt()!=day() || meet[4].toInt()!=day()){  // here we check if the set alarm is for current date and if not we print how many days until the alarm is set to go off
    int daggen=meet[0].toInt()-day()+(meet[3].toInt()-month())*30+(meet[4].toInt()-year())*365; // here we do assume that months are mostly 30 days for simplicitys sake

    int p=meet[4].toInt()-month();

    tft.print(", ");
    tft.print(daggen);
  }
}

//for main screen short display of nodes and alarms
void notes(){
  //Flush out old data from this spot:
    tft.fillRect(0,37,240,163,TFT_BLACK);

  //takes only one the most recent note setup on the server
    String note1 = myObject["note"]["note3"];

  // takes all three alarms setup
    String meeting1[5] = {myObject["events"]["event1"]["day"],myObject["events"]["event1"]["hour"],myObject["events"]["event1"]["min"],myObject["events"]["event1"]["month"],myObject["events"]["event1"]["year"]};
    String meeting2[5] = {myObject["events"]["event2"]["day"],myObject["events"]["event2"]["hour"],myObject["events"]["event2"]["min"],myObject["events"]["event2"]["month"],myObject["events"]["event2"]["year"]};
    String meeting3[5] = {myObject["events"]["event3"]["day"],myObject["events"]["event3"]["hour"],myObject["events"]["event3"]["min"],myObject["events"]["event3"]["month"],myObject["events"]["event3"]["year"]};
    
  //Print note 
    tft.setCursor(0,37);   
    tft.setTextSize(2);
    tft.println(note1);

  //use printEvents function to print alarm data to wanted positions:
    printEvents(0,90 , meeting1, 2);
    printEvents(0,110, meeting2, 2);
    printEvents(0,130, meeting3, 2);
}

//This is for printing only the note/alarm info and nothing else
void onlyNotes(){
  //takes all three nodes that have been typed and stored on the server
    String note1 = myObject["note"]["note3"];
    String note2 = myObject["note"]["note2"];
    String note3 = myObject["note"]["note1"];

  //set the position and the textsize
    tft.setCursor(0,50);   
    tft.setTextSize(4);

  //prints the notes as list form on to the OLED:
    tft.print("-");
    tft.println(note3);
    tft.print("-");
    tft.println(note2);
    tft.print("-");
    tft.println(note1);

  //Alarms:
  // get all the alarm data
    String meeting1[5] = {myObject["events"]["event1"]["day"],myObject["events"]["event1"]["hour"],myObject["events"]["event1"]["min"],myObject["events"]["event1"]["month"],myObject["events"]["event1"]["year"]};
    String meeting2[5] = {myObject["events"]["event2"]["day"],myObject["events"]["event2"]["hour"],myObject["events"]["event2"]["min"],myObject["events"]["event2"]["month"],myObject["events"]["event2"]["year"]};
    String meeting3[5] = {myObject["events"]["event3"]["day"],myObject["events"]["event3"]["hour"],myObject["events"]["event3"]["min"],myObject["events"]["event3"]["month"],myObject["events"]["event3"]["year"]};

  //print 'days:' as a clarification because now the alarm data is printed as like 16:34, 15 where 15 presents the days until alarm so on this view we print 'days:' on top of those for clarification
    tft.setCursor(230,200);
    tft.setTextSize(2);
    tft.println("days:");

  //prints the alarms using printEvents function
    printEvents(0,230 , meeting1, 3);
    printEvents(0,260, meeting2, 3);
    printEvents(0,290, meeting3, 3);
}

//function for printing bus information on the main screen
void bus(){
  //make string variables out of the server data:
    //bussSnam = short name (like 57, 8 etc.)
    String buss1Snam = myObject["public_transport_data"]["0"]["shortname"];
    String buss2Snam = myObject["public_transport_data"]["1"]["shortname"];
    String buss3Snam = myObject["public_transport_data"]["2"]["shortname"];

    //The time until the buss departures from that station
    String buss1dep = myObject["public_transport_data"]["0"]["departure"];
    String buss2dep = myObject["public_transport_data"]["1"]["departure"];
    String buss3dep = myObject["public_transport_data"]["2"]["departure"];

  //prints it all in wanted positions:
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

//function for printin bus information on it's own frame view
void onlyBus(){
  //make variables out of server data:
    //bus short name (like 57, 8 etc.)
      String buss1Snam = myObject["public_transport_data"]["0"]["shortname"];
      String buss2Snam = myObject["public_transport_data"]["1"]["shortname"];
      String buss3Snam = myObject["public_transport_data"]["2"]["shortname"];

    //bus departure
      String buss1dep = myObject["public_transport_data"]["0"]["departure"];
      String buss2dep = myObject["public_transport_data"]["1"]["departure"];
      String buss3dep = myObject["public_transport_data"]["2"]["departure"];

    //bus heading like (Rautatientori via sörnäinen)
    String buss1head = myObject["public_transport_data"]["0"]["headsign"];
    String buss2head = myObject["public_transport_data"]["1"]["headsign"];
    String buss3head = myObject["public_transport_data"]["2"]["headsign"];

  //Printing
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

//Function for making numbers to a bit that is then send to the shift register to light up correct segments on the 7s
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

//Samekinda function but for when we need to but a dot on the number.
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

//parses hours and minutes to 10s and 1s and updates the global array numer 
void klockan(){
  int h=hour();//gets hours from the Timelib.h library
  int m=minute(); //gets minutes from the Timelib.h library
  //hours
  numer[0]=numtobit(h/10);//changes the numer array first position to a byte presenting the hour 10s using numtobit function
  numer[1]=numtobit2(h%10);//changes the numer array second position to a byte presenting the hour 1s and here we use the numtbit2 that gives byte with dot segment light up to make the dot in time like 17.31 light uo
  //minutes
  numer[2]=numtobit(m/10); //same logic for these two
  numer[3]=numtobit(m%10);
}

//the main clock func where the data is changed on the 7 segment 4-digit display. Also in here we have function for getting DHT11data
void clockupdate(){
  klockan();//Gets updated bytes for current time to light correct segments on the 7s
  for (int j = 0; j <= 3; j++) { //for loop that goes through all the digits and numbers
    digitalWrite(num[j], LOW);//sets digit LOW so it's the only one lighten up
    digitalWrite(latch, LOW); //Sets shift register latch open
    shiftOut(datap, kello, LSBFIRST, numer[j]); //sends the correct byte info to shift register
    digitalWrite(latch, HIGH); //Closes latch
    DHT11data(); // Here we need delay so we funnily know that dht11 measurement makes delay so we but it here so that 7s clock display is more optimized
    delay(1); //We still need bit more delay to make sure the numbers look good
    digitalWrite(num[j], HIGH); //Unlight the digit
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

//function for the alarm 
void musicalbuzzer(String* alarmName){ 
    //used info for the for loop duration
    int size = sizeof(durations) / sizeof(int);

    //print the alarm info on the OLED
    tft.fillScreen(TFT_BLACK);
    printEvents(40,120, alarmName, 5);

  for (int note = 0; note < size; note++) {
    //to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int duration = 2*1000 / durations[note];
    //plays the tone:
    tone(BUZZER_PIN, melody[note], duration);

    //makes the buttons to stop the alarm
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


//Function for getting DHT11 data every 4 seconds 
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

//DHT11data comes from here and this function is optimzed to run along better than the other DHT11 data gathering ways to optimize delays 
void DHT11data(){
    /* Measure temperature and humidity.  If the functions returns
       true, then a measurement is available. */
  if (measure_environment(&temperature, &humidity)) {
    if (frame==0){
      //prints the room information gotten with DHT11 on the main scree:
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

//This function checks if the alarm needs to go off in the void loop by comparing the alarm data to the clock data set up on the esp
void checkAlarm(){
  //gets all the alarms:
    String meeting1[5] = {myObject["events"]["event1"]["day"],myObject["events"]["event1"]["hour"],myObject["events"]["event1"]["min"],myObject["events"]["event1"]["month"],myObject["events"]["event1"]["year"]};
    String meeting2[5] = {myObject["events"]["event2"]["day"],myObject["events"]["event2"]["hour"],myObject["events"]["event2"]["min"],myObject["events"]["event2"]["month"],myObject["events"]["event2"]["year"]};
    String meeting3[5] = {myObject["events"]["event3"]["day"],myObject["events"]["event3"]["hour"],myObject["events"]["event3"]["min"],myObject["events"]["event3"]["month"],myObject["events"]["event3"]["year"]};

  // for each alarm check if the day, hour, minute, month and year are same as the current time stored on esp. If this is true it goes to the musicalbuzzer func that plays the alarm
    if (meeting1[0].toInt() == day() && meeting1[1].toInt() == hour() && meeting1[2].toInt() == minute() && meeting1[3].toInt() == month() && meeting1[4].toInt() == year() ) {
      myObject["events"]["event1"]["day"]=0; //Here we simply set that alarms day to zero so that after we press the button and shutdown the alarm it doesn't start up again.
      musicalbuzzer(meeting1);
    }
    else if (meeting2[0].toInt() == day() && meeting2[1].toInt() == hour() && meeting2[2].toInt() == minute() && meeting2[3].toInt() == month() && meeting2[4].toInt() == year() ) {
      myObject["events"]["event2"]["day"]=0;
      musicalbuzzer(meeting2);
    }
    else if (meeting3[0].toInt() == day() && meeting3[1].toInt() == hour() && meeting3[2].toInt() == minute() && meeting3[3].toInt() == month() && meeting3[4].toInt() == year() ) {
      myObject["events"]["event3"]["day"]=0;
      musicalbuzzer(meeting3);
    }
}

void loop() {
  //updates the clock in the loop. If the OLED is off the 7s is also off to give more reflective space to check yourself in the mirror.
  if (BLvalue==1){
    clockupdate();
  }

  //checks the alarm in the loop so we get it to play as soon as the alarm is set to go off
  checkAlarm();
 
  //updates the OLED screen with new information. Currently every minute. This is reasonable since only data that updates fast is the buss departure and that is accurate to 1 min range:
  if ((millis()-updatetimer)>updatetime){
    updatescreen();
  }

  //Sends API to our serves once a minute to get new data. If you want data faster you can also press the reset button.
  if ((millis() - lastTime) > timerDelay) {
    loopAPIGet();
  }

  //posts our DHT11 data to our server that can then be stored, dipslayd and plotted on our server page
  if ((millis()-postTimer)>5000){
    postFunc();
  }
}

//The function to send data to our server from ESP. This data consist from the DHT11 reading to make plots out of your room temperature and humidity
void postFunc(){
  // Specify content-type header
  if(WiFi.status()== WL_CONNECTED){ //Check if wifi connection is goodS
    const char* serverName2 = "http://www.aaroesko.xyz/DHT_data"; //server domain where we send this data
    WiFiClient client; //declaring variables
    HTTPClient http;

    //setup
    http.begin(client, serverName2);

    //authorization for sending data
    http.setAuthorization("Admin", "Admin");


    //give content type 
    http.addHeader("Content-Type", "application/json");

    // Data to send with HTTP POST
    String httpRequestData = "{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"" + String(temperature) + "\",\"value2\":\"" + String(humidity) + "\",\"value3\":\"1005.14\"}";

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);

    //we get the httpResponseCode to serial for debugging if any issues. 200 tell's us that it works and anything else is problematic
    Serial.println(httpResponseCode);

    //Free resources
    http.end();
  }
  else {
      Serial.println("WiFi Disconnected");
  }
  //set the timer to current millis()
  postTimer=millis();
  
}

void loopAPIGet(){
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      
      //Using httpGETRequest func get data from server
      data = httpGETRequest(serverName);
      //makes it into a JSON object
      myObject = JSON.parse(data);
      
      // for debugging if we have problems parsing the data to a JSON object
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    //update timer
    lastTime = millis();
}

String httpGETRequest(const char* serverName) {
  //declare variables
  WiFiClient client;
  HTTPClient http;
    
  //Setup 
  http.begin(client, serverName);
  
  //authorization for getting data
  http.setAuthorization("Admin", "Admin");
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {   //gets the data
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {    //for debugging
    Serial.print("Error code: ");
    Serial.print("not connected");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  //returns the data
  return payload;
}
