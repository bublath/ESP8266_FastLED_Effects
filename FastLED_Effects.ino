#include <Arduino.h>
#include <EEPROM.h>
#define Serial_BAUDRATE 74880 // Arduino: 9600, ESP32: z.B. 115200

// Comment #define and uncomment #undef to exclude DHT11 Sensor if you just want the LEDs
#define ENABLE_DHT11 1
//#undef ENABLE_DHT11

// WiFi connectivity and webserver
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
const char* my_ssid = "";
const char* my_pass = ""; 
const char * esp_name = "ESP LED";
int rssi = -1;

// LED
#define FASTLED_ESP8266_RAW_PIN_ORDER
//To get rid of all the warnings, add
//d1_mini_lite.build.extra_flags=-Wno-register -Wno-misleading-indentation
//to c:\Users\xxxxx\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\
#pragma GCC diagnostic ignored "-Wregister"
#include <FastLED.h>
#define DATA_PIN 16 //D6
#define NUM_LEDS 300
CRGBArray<NUM_LEDS> leds;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
#define FRAMES_PER_SECOND  120

/* -------------------Some defaults----------------------------------------- */

struct settings {
  uint16_t magic=39523;
  uint8_t mode = 0;  // Effect mode
  uint8_t speed = 10; // Speed setting for some modes
  uint8_t fade = 50;
  uint8_t col_r = 255;
  uint8_t col_g = 0;
  uint8_t col_b = 0;
  uint8_t rbgmode = 0;
  uint8_t endless = 1;
  uint8_t reverse = 0;
  uint8_t brightness = 255;
  uint8_t ssid_len = 0;
  uint8_t pw_len = 0;
  char ssid[64]="ssid";
  char pw[64]="pw";
};

settings mysets;

#ifdef ENABLE_DHT11
// Temperature and Humidity sensor
#include "DHTesp.h"
DHTesp dht;
int dhtPin = 5; //D1
float temperature = -1;
float humidity = -1;
#endif

// HTML Pages

const char * html_head ="<html><head><style>\
                          .button {\
                            background-color: #990033;\
                            border: none;\
                            color: white;\
                            padding: 10px 25px;\
                            width: 150px;\
                            text-align: center;\
                            cursor: pointer;\
                          }\
                          </style></head>\
                          <body onload=\"updateValues()\">\
                          <p><h1>ESP8266 LED Effect Driver</h1></p>";
const char * html_table ="<table>\
                          <tr>\
                          <th>Brightness:</th>\
                          <th><input id=\"brightness\" type=\"range\" onChange=\"sliderFunction(\'brightness\',this.value)\" min=\"1\" max=\"255\" value=\"255\"></th>\
                          <th>R<input id=\"col_r\" type=\"range\" onChange=\"sliderFunction(\'r\',this.value)\" min=\"0\" max=\"255\" value=\"255\"></th>\
                          </tr><tr>\
                          <th>Fade Speed:</th>\
                          <th><input id=\"fade\" type=\"range\" onChange=\"sliderFunction(\'fade\',this.value)\" min=\"1\" max=\"255\" value=\"50\"></th>\
                          <th>G<input id=\"col_g\" type=\"range\" onChange=\"sliderFunction(\'g\',this.value)\" min=\"0\" max=\"255\" value=\"0\"></th>\
                          </tr><tr>\
                          <th>Effect Speed:</th>\
                          <th><input id=\"speed\" type=\"range\" onChange=\"sliderFunction(\'speed\',this.value)\" min=\"1\" max=\"200\" value=\"10\"></th>\
                          <th>B<input id=\"col_b\" type=\"range\" onChange=\"sliderFunction(\'b\',this.value)\" min=\"0\" max=\"255\" value=\"0\"></th>\
                          </tr><tr>\
                          <th>RGB mode<input type=\"checkbox\" onChange=\"sliderFunction('rbg',this.checked)\" id=\"rbg\"></th>\
                          <th>Endless mode<input type=\"checkbox\" onChange=\"sliderFunction('endless',this.checked)\" id=\"endless\"></th>\
                          <th>Reverse<input type=\"checkbox\" onChange=\"sliderFunction('reverse',this.checked)\" id=\"reverse\"></th>\
                          </tr><tr>\
                          <td colspan=\"3\">Full parameter effects:</td>\
                           </tr><tr>\
                          <th><input type=\"button\" onclick=\"buttonFunction(1)\" class=\"button\" value=\"K.I.T.T\"></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(14)\" class=\"button\" value=\"Sinelon\"></th>\
                           </tr><tr>\
                           <th><input type=\"button\" onclick=\"buttonFunction(3)\" class=\"button\" value=\"Mirror\"></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(2)\" class=\"button\" value=\"Multiband\"></th>\
                          </tr><tr>\
                          <td colspan=\"3\">Effects with variable speed/fade:</td>\
                          </tr><tr>\
                          <th><input type=\"button\" onclick=\"buttonFunction(16)\" class=\"button\" value=\"Juggle\"><br></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(15)\" class=\"button\" value=\"BPM\"></th>\
                          </tr><tr>\
                          <th><input type=\"button\" onclick=\"buttonFunction(13)\" class=\"button\" value=\"Confetti\"></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(17)\" class=\"button\" value=\"Fire\"></th>\
                          </tr><tr>\
                          <td colspan=\"3\">Static effects:</td>\
                          </tr><tr>\
                          <th><input type=\"button\" onclick=\"buttonFunction(4)\" class=\"button\" value=\"Pride\"></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(10)\" class=\"button\" value=\"Pacifica\"></th>\
                          </tr><tr>\
                          <th><input type=\"button\" onclick=\"buttonFunction(11)\" class=\"button\" value=\"Rainbow\"></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(12)\" class=\"button\" value=\"Rainbow with Glitter\"></th>\
                          </tr><tr>\
                          <th></th>\
                          </tr><tr>\
                          <th><input type=\"button\" onclick=\"buttonFunction(100)\" class=\"button\" value=\"Save\"><br></th>\
                          <th><input type=\"button\" onclick=\"buttonFunction(0)\" class=\"button\" value=\"OFF\"><br></th>\
                          </tr>";
const char * html_script ="<script>\
                            function updateValues() {\
                              const req = new XMLHttpRequest();\
                              req.open(\"GET\",\"/settings\");\
                              req.send();\
                              req.onreadystatechange = () => {\
                                if (req.readyState === XMLHttpRequest.DONE) {\
                                  const status = req.status;\
                                  if (status == 200) {\
                                     var reply = JSON.parse(req.responseText);\
                                     document.getElementById('rbg').checked=(reply.rbg==1?true:false);\
                                     document.getElementById('endless').checked=(reply.endless==1?true:false);\
                                     document.getElementById('reverse').checked=(reply.reverse==1?true:false);\
                                     document.getElementById('speed').value= reply.speed;\
                                     document.getElementById('fade').value= reply.fade;\
                                     document.getElementById('brightness').value= reply.brightness;\
                                     document.getElementById('col_r').value = reply.col_r;\
                                     document.getElementById('col_g').value = reply.col_g;\
                                     document.getElementById('col_b').value = reply.col_b;\
                                  }\
                                }\
                              };\
                            }\
                            function buttonFunction(id) {\
                               const req = new XMLHttpRequest();\
                               req.open(\"POST\",\"/command?id=\"+id);\
                               req.send();\
                               updateValues();\
                              return false;\
                            }\
                            function sliderFunction(name,val) {\
                               console.log(name+\"=\"+val);\
                               const req = new XMLHttpRequest();\
                               req.open(\"POST\",\"/slider?\"+name+\"=\"+val);\
                               req.send();\
                               return false;\
                            }\
                          </script>\
                          </body></html>";

// Called when system is powered up

void setup() {
  //size_t ssid_len,pw_len;
  //char ssid[64];
  //char pw[64];
  //ssid[0]=0;
  //pw[0]=0;
  Serial.begin(Serial_BAUDRATE);
  Serial.println("Starting ESP32");

  // Try to read WiFi connection parameters from EEPROM
  EEPROM.begin(160);
  uint16_t magic;
  EEPROM.get(0,magic);
  //Check magic word to make sure settings are valid
  if (magic==49523) {
    Serial.println("EEPROM valid, reading settings");
    EEPROM.get(0,mysets);
  }

  Serial.flush();
  delay(50);
  Serial.print("SSID: ");
  Serial.println(mysets.ssid);
  Serial.print("PASS: ");
  Serial.println(mysets.pw);

  // While developping prioritize hardcoded setting
  if (strlen(my_ssid)>0 and strlen(my_pass)>0) {
    WiFi.begin(my_ssid,my_pass);
  }
  WiFi.begin(mysets.ssid,mysets.pw);
  if (WiFi.waitForConnectResult(10000) != WL_CONNECTED) {
     while (WiFi.waitForConnectResult(10000) != WL_CONNECTED) {
      Serial.println("Error connecting Wifi!");
      Serial.setTimeout(60*1000); // 1 Minute
      int ssid_len = 0;
      while (ssid_len == 0) {
        Serial.println();
        Serial.print("Enter SSID:");
        ssid_len = Serial.readBytesUntil(10, mysets.ssid, 64);
        mysets.ssid[ssid_len]=0;
      }
      Serial.println();
      int pw_len = 0;
      while (pw_len == 0) {
        Serial.println();
        Serial.print("Enter Password:");
        pw_len = Serial.readBytesUntil(10, mysets.pw, 64);
        mysets.pw[pw_len]=0;
      }
      Serial.println();
      Serial.println("Connecting to "+String(mysets.ssid)+"/"+String(mysets.pw));
      Serial.flush();
      WiFi.begin(mysets.ssid,mysets.pw);
    }
    Serial.println("Successfully registered new WiFi connection");
    EEPROM.put(0,mysets);
    EEPROM.commit();
  } else {
      Serial.println("Successful WiFi connect with stored values");
      Serial.flush();    
  }
  
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(esp_name);

#ifdef ENABLE_DHT11
  dht.setup(dhtPin, DHTesp::DHT11);
#endif

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  leds.fadeToBlackBy(5);
  FastLED.setBrightness(mysets.brightness);
  random16_add_entropy( random());
  sinus_init();

  Serial.print("WiFi connected to ");
  Serial.print(WiFi.localIP());
  Serial.print(" with ");
  Serial.println(WiFi.RSSI());

  server.on("/", []() {
    String html_temp = "";
  #ifdef ENABLE_DHT11
    html_temp += "<p>Temperature:"+String(temperature)+" &deg;C</p><p>Humidity:"+String(humidity) + " %</p>";
  #endif
    String html = String(html_head)+html_temp+String(html_table)+String(html_script);
    server.send(200, "text/html", html.c_str());
  });
  server.on("/command", []() { //Define the handling function for the javascript path
    int newmode=server.arg("id").toInt();
    if (newmode==100) {
      EEPROM.put(0,mysets);
      EEPROM.commit();
    } else {
      mysets.mode=newmode;
      // Set reasonable defaults
      switch (mysets.mode) {
        case 0:  leds.fadeToBlackBy(5); break; // OFF
        case 1:  mysets.speed=10; mysets.fade=50; break; // KITT
        case 2:  mysets.speed=10; mysets.fade=50; break; // Multi
        case 3:  mysets.speed=10; mysets.fade=50; break; // Mirror
        case 4:  break; // Pride has no arguments to modify
        case 10: mysets.speed=50; break; // Pacifica
        case 11: mysets.speed=50; break; // Rainbow
        case 12: mysets.speed=50; break; // Rainbow with Glitter
        case 13: mysets.fade=25; mysets.speed=74; break; // Confetti
        case 14: sinus_init(); mysets.speed=100; mysets.fade=25; break; // Sinelon
        case 15: mysets.speed=30; mysets.fade=30; break; // BPM
        case 16: sinus_init(); mysets.speed=50; mysets.fade=30; break; // Juggle
        case 17: mysets.speed=60; mysets.fade=80; break; // Fire
      }
    }
    server.send(200,"text/html","ok");
  });
  server.on("/settings", []() {
  String reply="{\"speed\":\""+String(mysets.speed)+"\",\"fade\":\""+String(mysets.fade)+"\",";
  reply+="\"brightness\":\""+String(mysets.brightness)+"\",";
  reply+="\"col_r\":\""+String(mysets.col_r)+"\",\"col_g\":\""+String(mysets.col_g)+"\",\"col_b\":\""+String(mysets.col_b)+"\",";
  reply+="\"rbg\":\""+String(mysets.rbgmode)+"\",\"endless\":\""+String(mysets.endless)+"\",\"reverse\":\""+String(mysets.reverse)+"\"}";
  Serial.println(reply);
  server.send(200,"text/html",reply);
  });
  server.on("/slider", []() { //Define the handling function for the javascript path
    mysets.speed=set_arg(mysets.speed,server.arg("speed"));
    mysets.fade=set_arg(mysets.fade,server.arg("fade"));
    mysets.col_r=set_arg(mysets.col_r,server.arg("r"));
    mysets.col_g=set_arg(mysets.col_g,server.arg("g"));
    mysets.col_b=set_arg(mysets.col_b,server.arg("b"));
    mysets.rbgmode=set_arg(mysets.rbgmode,server.arg("rbg"));
    mysets.endless=set_arg(mysets.endless,server.arg("endless"));
    mysets.reverse=set_arg(mysets.reverse,server.arg("reverse"));
    String my_brightness=server.arg("brightness");
    if (my_brightness.length()>0) {
      mysets.brightness=my_brightness.toInt();
      FastLED.setBrightness(mysets.brightness);
     }
    server.send(200,"text/html","ok");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
  Serial.println("Executing main");
  Serial.flush();
}

// Helper function to only modify an argument if it was actually set
int set_arg(int val,String arg) {
  if (arg.length()>0) {
    if (arg == "true") {
      return 1;
    }
    if (arg == "false") {
      return 0;
    }
    return arg.toInt();
  }
  return val;
}

/* ------------------------------------------------------------ */

void handleNotFound() {
  String message = "Page Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  message += "Headers:";
  message += server.headers();
  message += "\n";

  for (uint8_t i = 0; i < server.headers(); i++) {
    message += " " + server.headerName(i) + ":" + server.header(i) + "\n";
  }
  
  Serial.println(message);
  server.send(404, "text/plain", message);
}

// Executed repeatedly after setup has finished
void loop() {
  server.handleClient();
#ifdef ENABLE_DHT11
  handle_dht11();
#endif
  handle_led();
}

int sec=0;

#ifdef ENABLE_DHT11
// Update Temperature and Humidity
void handle_dht11() {
  
  int cur=millis()/1000;

  if (sec<cur) {
    TempAndHumidity newValues = dht.getTempAndHumidity();
    if (newValues.temperature != temperature) {
      temperature=newValues.temperature;
      Serial.println("T:" + String(newValues.temperature));
    }
    if (newValues.humidity != humidity) {
      humidity=newValues.humidity;
      Serial.println("H:" + String(newValues.humidity));
    }
    // Only read temperatures once every 30 seconds, regardless how long the LED effects take
    sec=sec+30;
  }
}
#endif

// Handle all the LED related stuff
void handle_led() {

  CRGB col;
  if (mysets.rbgmode==1) {
    col=CRGB(mysets.col_r,mysets.col_g,mysets.col_b);
  } else {
    col=CHSV( gHue, 255, 192);
  }
  
  switch (mysets.mode) {
    case 0: leds.fadeToBlackBy(25); FastLED.delay(50); FastLED.show(); break;
    case 1: if (mysets.endless==0) {
              KITT(col); 
            } else {
              KITT2(col,mysets.reverse==0?1:-1);
            }
            break;
    case 2: Multi(col,mysets.reverse==0?1:-1); break;
    case 3: Mirror(col); break;
    case 4: pride(); break;
    case 10: pacifica_loop(); break;
    case 11: rainbow(); break;
    case 12: rainbowWithGlitter(); break;
    case 13: confetti(); break;
    case 14: sinelon(col); break;
    case 15: bpm(); break;
    case 16: juggle(); break;
    case 17: Fire(0,-50);Fire(50,50);Fire(100,-50);Fire(150,50);Fire(200,-50); Fire(250,50); break;
  }
  
  // Modes 1-5 are self contained
  // Modes 4-10 need this to work
  if (mysets.mode>=10) {
    FastLED.show();  
    // insert a delay to keep the framerate modest
    //FastLED.delay(1000/FRAMES_PER_SECOND); 
    FastLED.delay(1000/mysets.speed/3);    
    // do some periodic updates
  }
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

}

int pos=0;
int dir=1;

// Some self developed effects

// Two streams meeting in the middle - either reversing back or repeating

void Mirror(CRGB col){ 
  static uint8_t hue;
  pos+=dir;
  if (pos>NUM_LEDS/2) {
    if (mysets.endless==0) {
      pos=0;
    } else {
      dir=-1;
    }
  }
  if (pos<1) {
    dir=1;
  }
  leds.fadeToBlackBy(mysets.fade);
  leds[pos] = col;
  leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1 ,0);
  FastLED.delay(mysets.speed);
}

// Multiple streams that get more when fade is set faster (so the LED strip is still pretty filled)

void Multi(CRGB col,int dir) {
  pos+=dir;
  if (pos>=NUM_LEDS) {
    pos=0;
  }
  if (pos<0) {
    pos=NUM_LEDS-1;
  }
  int step=NUM_LEDS/(mysets.fade/4+1);
  leds.fadeToBlackBy(mysets.fade);
  for (int i=0;i<NUM_LEDS;i+=step) {
    leds[abs(pos+i)%(NUM_LEDS-1)]= col;
  }
  FastLED.delay(mysets.speed);
}

// Classical Knight Rider effect, but with different modes, directions, speed and colors

void KITT(CRGB col) {
  pos=pos+dir;
  if (pos>=NUM_LEDS-1) {
     dir=-1;
  }
  if (pos<=0) {
    dir=1;
  }
  leds.fadeToBlackBy(mysets.fade);
  leds[pos]= col;
  FastLED.delay(mysets.speed);
}

void KITT2(CRGB col, int dir) {
  pos+=dir;
  if (pos>=NUM_LEDS) {
     pos=0;
  }
  if (pos<=-1) {
    pos=NUM_LEDS-1;
  }
  leds.fadeToBlackBy(mysets.fade);
  leds[pos]= col;
  FastLED.delay(mysets.speed);
}

/**** Below are effects copied from
 **** http://fastled.io/docs/examples.html
 **** some have been slightly modified to work in this framework
****/

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}
 
void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}
 
void addGlitter( fract8 chanceOfGlitter) {
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, mysets.fade);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = mysets.speed;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

// Inspired by FastLED Sinelon example, but rewritten to fill gaps for smoother output

void sinelon(CRGB col) {
    fadeToBlackBy( leds, NUM_LEDS, mysets.fade);
    sinus(mysets.speed/10,0,col);
}

 // remember positions for sinus-wave
 int ppos[8];
 int cpos[8];

// init start position
 void sinus_init() {
    for( int i = 0; i < 8; i++) {
      ppos[i]=beatsin16( i+7, 0, NUM_LEDS );
      cpos[i]=beatsin16( i+7, 0, NUM_LEDS );
    }
 }

// Based on https://github.com/atuline/FastLED-Demos/blob/master/juggle_pal/juggle_pal.ino
// Originally by: Mark Kriegsman
// Modified By: Andrew Tuline
// Modified to use an enhanced sinus function that fills the gaps between the dots of the wave resulting in a nicer flow

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, mysets.fade);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    sinus(i+7,i,CHSV( dothue, 255, 192));
    dothue += 32;
  }
}

// Support function for Sinus-Wave that fills the missed out LEDs - used by juggle + sinelon
void sinus(int beat, int index, CRGB col) {
  ppos[index]=cpos[index];
  cpos[index] = beatsin16(beat, 0, NUM_LEDS );
  if (ppos[index]<cpos[index]) {
    for (int i=ppos[index];i<cpos[index];i++) {
      leds[i] |= col;
    }
  }
 if (ppos[index]>=cpos[index]) {
    for (int i=cpos[index];i<ppos[index];i++) {
      leds[i] |= col;
    }
  }
}

// From FastLED examples
// https://github.com/FastLED/FastLED/blob/master/examples/Pride2015/Pride2015.ino

void pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);
 
  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
 
    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;
 
    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }
  FastLED.show();
}

// Pacifica effect, simulating waves, all in blue 
// December 2019, Mark Kriegsman and Mary Corey March.
// https://dmadison.github.io/FastLED/docs/_pacifica_8ino-example.html 

CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };
 
  
void pacifica_loop()
{
 // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));
 
  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));
 
  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));
 
  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();
 
  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
  FastLED.show();
}
 
// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}
 
// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}
 
// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 
    leds[i] |= CRGB( 2, 5, 7);
  }
}

// Based on Fire2012 by Mark Kriegsman, July 2012
// https://dmadison.github.io/FastLED/docs/_fire2012_8ino-example.html
// Modified slightly to support multiple mirroring fires for long LED strips

#define SPARKING 120
 
void Fire(int start,int size_s)
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];
  int size=abs(size_s);
// Step 1.  Cool down every cell a little
  for( int i = start; i < start+size; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((mysets.fade * 10) / size) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= size+start; k > start+2; k--) { //int k= size+start - 3; k > start; k--)
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
  
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y+start] = qadd8( heat[y+start], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < size; j++) {
    if (size_s>0) {
      leds[j+start] = HeatColor( heat[j+start]);
    } else {
      //Copy in reverse to create to mirroring fires when size is negative
      leds[start+size-j] = HeatColor( heat[j+start]);
    }
  }
}
 