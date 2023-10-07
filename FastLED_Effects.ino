#include <Arduino.h>
#define Serial_BAUDRATE 74880 // Arduino: 9600, ESP32: z.B. 115200

// Comment #define and uncomment #undef to exclude DHT11 Sensor if you just want the LEDs
//#define ENABLE_DHT11 1
#undef ENABLE_DHT11

// WiFi connectivity and webserver
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
const char* my_ssid = "";
const char* my_pass = ""; 
const char * esp_name = "ESP LED";
int rssi = -1;
int apmode = 0;
int ssid_len = 0;
int ssid_ind =-1;
int pw_len = 0;
int pw_ind = -1;

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
//Avoid getting to dark (or black) as this is perceived as a flicker
#define FADE_MIN 32
int timer=0;


// Setting that should survive power cycle
#include <EEPROM.h>
// Change this if the structure of "settings" is changing to avoid incorrect assignments
#define MAGIC_WORD 12569
struct settings {
  uint16_t magic=MAGIC_WORD;
  uint8_t mode = 0;  // Effect mode
  uint8_t speed = 10; // Speed setting for some modes
  uint8_t fade = 50;
  uint8_t hue = 20;
  uint8_t col_r = 255;
  uint8_t col_g = 0;
  uint8_t col_b = 0;
  uint8_t rgbmode = 0;
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

#define BUTTON_PIN 2

// To test the page it can be renamed to .html
// uses the C++ raw string literals 
#include "webpage.h"

// Called when system is powered up

void setup() {
  Serial.begin(Serial_BAUDRATE);
  Serial.println("Starting ESP");

  // Try to read WiFi connection parameters from EEPROM
  EEPROM.begin(160);
  uint16_t magic;
  EEPROM.get(0,magic);
  //Check magic word to make sure settings are valid
  if (magic==MAGIC_WORD) {
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
      Serial.println("Error connecting Wifi!");
      IPAddress local_IP(192,168,33,1);
      IPAddress subnet(255,255,255,0);
      WiFi.mode(WIFI_AP);
      WiFi.softAP(esp_name);
      delay(100);
      WiFi.softAPConfig(local_IP, local_IP, subnet);
      Serial.print("Soft-AP IP address = ");
      Serial.println(WiFi.softAPIP());
      apmode=1;
  } else {
      Serial.println("Successful WiFi connect with stored values");
      Serial.flush();    
      apmode=0;
  }
  
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(esp_name);

#ifdef ENABLE_DHT11
  dht.setup(dhtPin, DHTesp::DHT11);
#endif

  pinMode(BUTTON_PIN,INPUT);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  leds.fadeToBlackBy(5);
  setBrightness(mysets.brightness);
  random16_add_entropy( random());
  sinus_init();

  Serial.print("WiFi connected to ");
  Serial.print(WiFi.localIP());
  Serial.print(" with ");
  Serial.println(WiFi.RSSI());

  if (apmode) {
  server.on("/", []() {
    server.send(200, "text/html", login_page);
  });
  server.on("/login", []() {
    Serial.print("SSID:");
    Serial.println(server.arg("ssid"));
    ssid_len=urlDecode(server.arg("ssid").c_str(),mysets.ssid,64);
    Serial.println(mysets.ssid);
    Serial.print("Password:");
    Serial.println(server.arg("pw"));
    urlDecode(server.arg("pw").c_str(),mysets.pw,64);
    pw_len=Serial.println(mysets.pw);
    Serial.println(mysets.pw);
  });
  } else {
    server.on("/", []() {
      server.send(200, "text/html", html_page);
    });
    server.on("/command", []() { //Define the handling function for the javascript path
      int newmode=server.arg("id").toInt();
      if (newmode==100) {
        EEPROM.put(0,mysets);
        EEPROM.commit();
      } else if (newmode==101) {
        timer=millis()/1000+60*60; // fixed timer 1h for now
      } else {
        initMode(newmode);
      }
      server.send(200,"text/html","ok");
    });
    server.on("/settings", []() {
    String reply="{\"speed\":\""+String(mysets.speed)+"\",\"fade\":\""+String(mysets.fade)+"\",";
    reply+="\"brightness\":\""+String(mysets.brightness)+"\",";
    reply+="\"hue\":\""+String(mysets.hue)+"\",";
    reply+="\"effect\":\""+String(mysets.mode)+"\",";
    if (timer>0) {
      reply+="\"timer\":\"(Timer: "+String((timer-millis()/1000)/60)+" min"+")\",";
    } else {
      reply+="\"timer\":\" \",";
    }
    #ifdef ENABLE_DHT11
      reply+= "\"temperature\":\""+String(temperature)+"\",\"humidity\":\""+String(humidity)+"\",";
    #endif
    reply+="\"col_r\":\""+String(mysets.col_r)+"\",\"col_g\":\""+String(mysets.col_g)+"\",\"col_b\":\""+String(mysets.col_b)+"\",";
    reply+="\"rgb\":\""+String(mysets.rgbmode)+"\",\"endless\":\""+String(mysets.endless)+"\",\"reverse\":\""+String(mysets.reverse)+"\"}";
    Serial.println(reply);
    server.send(200,"text/html",reply);
    });
    server.on("/slider", []() { //Define the handling function for the javascript path
      mysets.speed=set_arg(mysets.speed,server.arg("speed"));
      mysets.fade=set_arg(mysets.fade,server.arg("fade"));
      mysets.hue=set_arg(mysets.hue,server.arg("hue"));
      mysets.col_r=set_arg(mysets.col_r,server.arg("r"));
      mysets.col_g=set_arg(mysets.col_g,server.arg("g"));
      mysets.col_b=set_arg(mysets.col_b,server.arg("b"));
      mysets.rgbmode=set_arg(mysets.rgbmode,server.arg("rgb"));
      mysets.endless=set_arg(mysets.endless,server.arg("endless"));
      mysets.reverse=set_arg(mysets.reverse,server.arg("reverse"));
      String my_brightness=server.arg("brightness");
      if (my_brightness.length()>0) {
        mysets.brightness=my_brightness.toInt();
        setBrightness(mysets.brightness);
      }
      server.send(200,"text/html","ok");
    });
    server.on("/reset", []() {
      // next reboot will require all settings including WiFi to be set again
      mysets.magic=0;
      EEPROM.put(0,mysets);
      EEPROM.commit();      
    }); 
  }
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
  Serial.println("Executing main");
  Serial.flush();
}

void initMode(int newmode) {
  mysets.mode=newmode;
  //Reset brightness as some effects change it
  setBrightness(mysets.brightness);
  // Set reasonable defaults
  switch (mysets.mode) {
    case 0:  timer=0; leds.fadeToBlackBy(5); break; //OFF
    case 1:  mysets.speed=255; mysets.fade=50; 
            mysets.rgbmode=1; mysets.col_r=255; mysets.col_g=0; mysets.col_b=0;
            mysets.endless=0; break; // KITT
    case 2:  mysets.speed=180; mysets.fade=50; mysets.rgbmode=0; break; // Multi
    case 3:  mysets.speed=255; mysets.fade=50; mysets.rgbmode=0; mysets.endless=1; break; // Mirror
    case 4:  mysets.speed=255; mysets.hue=32; break; // Pride 
    case 5:  mysets.speed=50; break; // Constant
    case 6:  mysets.speed=50; break; // ConstantFade
    case 7:  sinus_init(); mysets.speed=100; mysets.fade=25; break; // Sinelon
    case 8:  mysets.speed=140; mysets.fade=80; break; // Fire
    case 9:  mysets.speed=230; mysets.fade=100; mysets.rgbmode=0; break; // Multi
    case 10: mysets.speed=50; mysets.hue=32; break; // Pacifica
    case 11: mysets.speed=50; mysets.hue=32; break; // Rainbow
    case 12: mysets.speed=50; mysets.hue=32; break; // Rainbow with Glitter
    case 13: mysets.fade=20; mysets.speed=120; break; // Confetti
    case 14: mysets.fade=8; mysets.speed=255; break; // Confetti fast
    case 15: mysets.speed=90; mysets.fade=30; break; // BPM
    case 16: sinus_init(); mysets.speed=50; mysets.fade=30; break; // Juggle
  }

}

// Sets the brightness exponentially which rather looks like what you expect
void setBrightness(int val) {
  FastLED.setBrightness(val*val/256);
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

//Revert javascript function encodeURIComponent(pw) (%xx -> character)
int urlDecode(const char *src,char *dest,int len) {
  Serial.println(src);
  char ch[3];
  int d=0;
  int i, ii;
  for (i=0; i<strlen(src); i++) {
      if (src[i]=='%') {
          ch[0]=src[i+1];
          ch[1]=src[i+2];
          ch[2]=0;
          sscanf(ch, "%x", &ii);
          dest[d++]=static_cast<char>(ii);
          i=i+2;
      } else {
          dest[d++]=src[i];
      }
  }
  dest[d]=0;
  return d;
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

int button=1;

// Executed repeatedly after setup has finished
void loop() {
  if (apmode) {
    getCredentials();
  }
  server.handleClient();
#ifdef ENABLE_DHT11
  handle_dht11();
#endif
  handle_led();

  int bt=digitalRead(BUTTON_PIN);
  if (button!=bt) {
    Serial.print("Button:");
    Serial.println(bt);
    button=bt;
    if (button==1) {
      mysets.mode++;
      if (mysets.mode>16) {
        mysets.mode=0;
      }
      initMode(mysets.mode);
      Serial.print("New mode:");
      Serial.println(mysets.mode);
    }
  }

}

// Scan serial for SSID/PW is stateful mode to allow AP Input in parallel
void getCredentials() {
  if (ssid_len == 0) {
    if (ssid_ind<0) {
      Serial.println();
      Serial.print("Enter SSID:");
      ssid_ind=0;
    } else {
      int len = Serial.readBytes(&mysets.ssid[ssid_ind], 64-ssid_ind);
      ssid_ind+=len;
      if (mysets.ssid[ssid_ind-1]==10) {
        mysets.ssid[ssid_ind-1]=0;
        ssid_len=ssid_ind-1;
        Serial.println(mysets.ssid);
        Serial.flush();
      }
    }
  }
  if (pw_len == 0 && ssid_len>0) {
    if (pw_ind<0) {
      Serial.println();
      Serial.print("Enter Password:");
      pw_ind=0;
    } else {
      int len = Serial.readBytes(&mysets.pw[pw_ind], 64-pw_ind);
      pw_ind+=len;
      if (mysets.pw[pw_ind-1]==10) {
        mysets.pw[pw_ind-1]=0;
        pw_len=pw_ind-1;
        Serial.println(mysets.pw);
        Serial.flush();
      }
    }
  }
  if (pw_len>0 && ssid_len>0) {
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    Serial.println("Connecting to "+String(mysets.ssid)+"/"+String(mysets.pw));
    Serial.flush();
    WiFi.begin(mysets.ssid,mysets.pw);
    if (WiFi.waitForConnectResult(10000) != WL_CONNECTED) {
      Serial.println("Error connecting Wifi!");
    } else {
    Serial.println("Successfully registered new WiFi connection");
    EEPROM.put(0,mysets);
    EEPROM.commit();      
    }
    ESP.restart();
  }
}

int sec=0;
int mil=0;

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
  if (mysets.rgbmode==1) {
    col=CRGB(mysets.col_r,mysets.col_g,mysets.col_b);
  } else {
    col=CHSV( gHue, 255, 192);
  }
  if (timer>0 && millis()/1000>timer) {
    mysets.mode=0; timer=0;
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
    case 5: Constant(col); break;
    case 6: ConstantFade(col); break;
    case 7: sinelon(col); break;
    case 8: Fire(0,-50);Fire(50,50);Fire(100,-50);Fire(150,50);Fire(200,-50); Fire(250,50); break;
    case 9: Multi(col,mysets.reverse==0?1:-1); break; // with faster settings
    case 10: pacifica_loop(); break;
    case 11: rainbow(); break;
    case 12: rainbowWithGlitter(); break;
    case 13: confetti(); break;
    case 14: confetti(); break; // with faster settings
    case 15: bpm(); break;
    case 16: juggle(); break;

  }

  if (mil<millis()) {
    mil=millis()+mysets.hue; 
    gHue++; // slowly cycle the "base color" through the rainbow
  }

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
  FastLED.delay(256-mysets.speed);
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
  FastLED.delay(256-mysets.speed);
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
  FastLED.delay(256-mysets.speed);
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
  FastLED.delay(256-mysets.speed);
}

void Constant(CRGB col) {
  for (int i=0;i<NUM_LEDS;i++) {
    leds[i]=col;
  }
  FastLED.delay(mysets.speed);
}

void ConstantFade(CRGB col) {
  Constant(col);
  int fade_max=(mysets.brightness<FADE_MIN?FADE_MIN:mysets.brightness);
  int t=(millis()*mysets.speed/2000)%(fade_max*2);
  if (t>fade_max) {t=fade_max*2-t;}
  if (t<FADE_MIN) {t=FADE_MIN;}
  setBrightness(t);
}

// Inspired by FastLED Sinelon example, but rewritten to fill gaps for smoother output

void sinelon(CRGB col) {
    fadeToBlackBy( leds, NUM_LEDS, mysets.fade);
    //Divide by 50 to reduce to reasonable range
    sinus(mysets.speed/10+1,0,col);
    FastLED.delay(20);
    // Settings Beat and Speed independently does not make much sense
    // Speed could be fixed to 20 and beat from 1-255 - so speed setting could still be used
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

/**** Below are effects copied from
 **** http://fastled.io/docs/examples.html
 **** some have been slightly modified to work in this framework
****/

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  FastLED.delay((256-mysets.speed)/3+1);    
}
 
void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  for (int i=0;i<NUM_LEDS/64;i++) { // Longer strip, more glitter
    addGlitter(80);
  }
  //FastLED.show();  
  FastLED.show(); // No delay here as rainbow() already does it    
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
  FastLED.delay((256-mysets.speed)/3);    
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = mysets.speed/3;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  FastLED.delay(mysets.speed/3);    
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
  //FastLED.show();  
  FastLED.delay(mysets.speed/5);    
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
  FastLED.delay(mysets.speed/5);    
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
  FastLED.delay((256-mysets.speed)/5);
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
  FastLED.delay((256-mysets.speed)/20+1);
}

