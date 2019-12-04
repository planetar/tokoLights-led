/*
 * This is customized to serve as a lights controller for an 3d printer 
 * 
 * it supports one neopixel string (led-ring, 8 leds, but could be anything longer) 
 * it supports 2 pwm-dimmed white led strips that are interfaced by MOSFETs. (range 0-1023 with 1023 ~ max brightness)
 * 
 * it connects to a wifi, subscribes and publishes to an mqtt broker and is ready for OTA updates
 * 
 * wifi, mqtt and OTA secrets are kept in separate files
 * 

 libraries:

 05.19: deleted lots of unused code
  
*/

///////#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER


#include "mqtt_secrets.h"
#include "wifi_secrets.h"



#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FastLED.h"
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <Streaming.h>

//#define EIGHTER true
#define TOKOLIGHTS true

const char* version = "05.18";



#ifdef TOKOLIGHTS

  #define SENSORNAME "tokoLights-led"
  
  const char* state_topic = "led/tl/state";
  const char* set_topic   = "led/tl/set";
  const char* dbg_topic   = "led/tl/dbg";
  
  byte brightness = 23;
  #define NUM_LEDS    8
  
  bool stateOn = false;
  bool startFade = false;
  
  CHSV baseColor(255,240,255);
  CHSV currColor(15,255,255);
  
  int hueRange=12;
  int quart=2;
  int direction=1;
  
  int stepTime=180;
  const char* effect = "shiftBand";
#endif



const char* on_cmd = "ON";
const char* off_cmd = "OFF";


String effectString = "solid";

// timed_loop
unsigned long INTERVAL_0 = 60;

#define INTERVAL_1   3000
#define INTERVAL_2  30000
#define INTERVAL_3  60000
#define INTERVAL_4 180000

unsigned long time_0 = 0;
unsigned long time_1 = 0;
unsigned long time_2 = 0;
unsigned long time_3 = 0;
unsigned long time_4 = 0;


// debug messages
const int numMsg=20;
int msgCount=0;
String msg=".";
String arrMessages[numMsg];



/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(32);



/*********************************** FastLED Defintions ********************************/


#define DATA_PIN    2



#define CHIPSET     WS2812B
#define COLOR_ORDER GRB

struct CRGB leds[NUM_LEDS];


byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;

byte red = 255;
byte green = 255;
byte blue = 255;

unsigned long  INTERVAL_anim = 2000;
int animFactor = 1;
unsigned long time_anim = 0;


bool isStalled = false;
bool firstRun  = true;

/******************************** tokoLights vars        *******************************/

#define P0_PIN  D5
#define P1_PIN  D6

int rangeMax=1024;
int p0_curr = 1;
int p1_curr = 1;




/******************************** GLOBALS for animations *******************************/


int loopCount=0;
int animCount=0;
unsigned long lastLoop=millis();



float hueStep;
float hue;
int delayMultiplier = 1;

unsigned int tempKelvin = 900;


/////////////////////////////////////////////////////////////////////////////////////////


WiFiClient espClient;
PubSubClient mqClient(espClient);


/////////////////////////////////////////////////////////////////////////////////////////



/********************************** START SETUP*****************************************/
void setup() {
  
  Serial.begin(115200);

  debug(String(SENSORNAME)+" "+version,true);
    
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  setupWifi();
  
  setupMq();
  setupOta();

  
  colorInit();

  pinMode(DATA_PIN,OUTPUT);
  pinMode(P0_PIN,OUTPUT);
  pinMode(P1_PIN,OUTPUT);
}



/********************************** START Set Color*****************************************/

void setColor(int inR, int inG, int inB) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].red   = inR;
    leds[i].green = inG;
    leds[i].blue  = inB;
  }

  FastLED.setBrightness(brightness);
  FastLED.show();

}


void colorInit(){
  effectString=effect;
  hueStep = hueRange*1.0 / quart;

 for (int i = 0; i < NUM_LEDS; i++) {
  currColor.h = ( baseColor.h+i);
    leds[i]=currColor;
       //FastLED.show();
  }


  
  FastLED.setBrightness(brightness);
  FastLED.setDither( 0 );
  FastLED.show();
}

void fillHsv(CHSV hsv){
  for (int i = 0; i < NUM_LEDS; i++) {
    hsv2rgb_rainbow( hsv, leds[i]);
  }
  FastLED.setBrightness(brightness);
  
}

/********************************** START MAIN LOOP*****************************************/

void loop() {

  mqClient.loop();
  ArduinoOTA.handle();
  

  timed_loop();
} 

void timed_loop() {
   if(millis() > time_0 + INTERVAL_0){
    time_0 = millis();

      colorLoop();
      
  } 
  
  if(millis() > time_1 + INTERVAL_1){
    time_1 = millis();
    
    if (!mqClient.connected()) {
      mqConnect();
    }  
    checkDebug();      
  }
   
  if(millis() > time_2 + INTERVAL_2){
    time_2 = millis();
      
  }
 
  if(millis() > time_3 + INTERVAL_3){
    time_3 = millis();
      

  }

  if(millis() > time_4 + INTERVAL_4){
    time_4 = millis();

  }


}

void setP0(int aVal) {
  aVal = aVal % rangeMax;
  if (aVal != p0_curr){
    p0_curr = aVal;
    analogWrite(P0_PIN,p0_curr);
  }
}

void setP1(int aVal) {
  aVal = aVal % rangeMax;
  if (aVal != p1_curr){
    p1_curr = aVal;
    analogWrite(P1_PIN,p1_curr);
  }
}

void colorLoop(){
  
   if (stateOn && !isStalled) {

    if (effectString == "rangeWave"){
      rangeWave();
    }
    else if (effectString == "shiftBand"){
      shiftBand();
    }
    else if (effectString == "sineHue"){
      sineHue();
    }
    else if (effectString == "solid"){
      solid();
    }    
     else if (effectString == "kelvin"){
      kelvin();
    }   
    else {
      debug("unknown effect: "+effectString,true);
      effectString="solid";
      //delay(1000);
    }
    //showleds();
    FastLED.show();
    
  }
  INTERVAL_0 = stepTime * delayMultiplier;
}

void showleds(){

  delay(1);

  if (stateOn) {
    FastLED.setBrightness(brightness);  //EXECUTE EFFECT COLOR
    FastLED.show();
    if (stepTime > 0 && stepTime < 250) {  //Sets animation speed based on receieved value
      FastLED.delay(stepTime / 10 * delayMultiplier); //1000 / transitionTime);
      //delay(10*transitionTime);
    }
  }
}


void rangeWave(){
    unsigned long now = millis();

    if (now - lastLoop > stepTime) {
        lastLoop = now;
        loopCount++;
        loopCount = loopCount % NUM_LEDS;
        
        hue = baseColor.h*1.0;
        hueStep = hueRange*1.0 / quart;
        currColor = baseColor;
        int pos=0;

        for (int i = 0; i < quart*4; i++) {
          if (i<quart){direction=1;}
          if (i>=quart && i<quart*2){direction=-1;}
          if (i>=quart*2 && i<quart*3){direction=-1;}
          if (i>=quart*3 && i<quart*4){direction=1;}
          
          hue += hueStep*direction;
          currColor.h = (int) hue;
          pos=(loopCount+i) % NUM_LEDS;

          hsv2rgb_rainbow( currColor, leds[pos]);
        }
    }
}



void shiftBand(){
  for (int i = 0; i < NUM_LEDS; i++) {
    currColor.h = ( baseColor.h+i+loopCount);
    leds[i]=currColor;
  }
  loopCount++;
}

void sineHue(){

      static uint8_t hue_index = 0;
      static uint8_t led_index = 0;
 
      if (led_index <= 0||led_index >= NUM_LEDS) {
        direction=direction*-1;
      }
      for (int i = 0; i < NUM_LEDS; i = i + 1)
      {
        leds[i] = CHSV(hue_index, 255, 255 - int(abs(sin(float(i + led_index) / NUM_LEDS * 2 * 3.14159) * 255)));
      }

      led_index +=direction;
      hue_index -=direction;

     if (hue_index >= 255) {
        hue_index = 0;
      }
      if (hue_index <= 0) {
        hue_index = 255;
      }    
      delayMultiplier = 2;
         
}

void solid() {
  if (firstRun){
    FastLED.clear();
    fillHsv(baseColor);
    FastLED.show();
    firstRun=false;
  }
}

void kelvin(){
      if (firstRun){
        FastLED.clear();
        CRGB tempColor = temp2rgb(tempKelvin);
        fill_solid(leds,NUM_LEDS,tempColor);
        FastLED.show();
        firstRun=false;
      }
}



CRGB temp2rgb(unsigned int kelvin) {
    int tmp_internal = kelvin / 100.0;
    int red, green, blue;
    CRGB result;
    
    // red 
    if (tmp_internal <= 66) {
        red = 255;
    } else {
        float tmp_red = 329.698727446 * pow(tmp_internal - 60, -0.1332047592);
        if (tmp_red < 0) {
            red = 0;
        } else if (tmp_red > 255) {
            red = 255;
        } else {
            red = tmp_red;
        }
    }
    
    // green
    if (tmp_internal <=66){
        float tmp_green = 99.4708025861 * log(tmp_internal) - 161.1195681661;
        if (tmp_green < 0) {
            green = 0;
        } else if (tmp_green > 255) {
            green = 255;
        } else {
            green = tmp_green;
        }
    } else {
        float tmp_green = 288.1221695283 * pow(tmp_internal - 60, -0.0755148492);
        if (tmp_green < 0) {
            green = 0;
        } else if (tmp_green > 255) {
            green = 255;
        } else {
            green = tmp_green;
        }
    }
    
    // blue
    if (tmp_internal >=66) {
        blue = 255;
    } else if (tmp_internal <= 19) {
        blue = 0;
    } else {
        float tmp_blue = 138.5177312231 * log(tmp_internal - 10) - 305.0447927307;
        if (tmp_blue < 0) {
            blue = 0;
        } else if (tmp_blue > 255) {
            blue = 255;
        } else {
            blue = tmp_blue;
        }
    }
    result = CRGB(red,green,blue);
    return result;
}


/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {

    
  StaticJsonDocument<512> root;
  deserializeJson(root, payload, length);


  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
      setColor(0, 0, 0);
    }
  }
  if (root.containsKey("pause")) {
    if (strcmp(root["pause"], on_cmd) == 0) {
      isStalled = true;
    }
    else if (strcmp(root["pause"], off_cmd) == 0) {
      isStalled = false;

    }
  }

    if (root.containsKey("hueRange")) {
      hueRange = root["hueRange"];
    }

     if (root.containsKey("quart")) {
      quart = root["quart"];
    }  

    if (root.containsKey("animFactor")) {
      animFactor = root["animFactor"];
    } 

    if (root.containsKey("transition")) {
      stepTime = root["transition"];
    }

    if (root.containsKey("stepTime")) {
      stepTime = root["stepTime"];
    }

    if (root.containsKey("bright")) {
      brightness = root["bright"];
      FastLED.setBrightness(brightness);
    }

    if (root.containsKey("brightness")) {
      brightness = root["brightness"];
      FastLED.setBrightness(brightness);
    }


    if (root.containsKey("p0")) {
      int aVal = root["p0"];
      setP0(aVal);
    }

    if (root.containsKey("p1")) {
      int aVal = root["p1"];
      setP1(aVal);
    }

 

    if (root.containsKey("colorHsv")) {
      baseColor.h = root["colorHsv"]["h"];
      baseColor.s = root["colorHsv"]["s"];
      baseColor.v = root["colorHsv"]["v"];
      
      fillHsv(baseColor);
    }

    if (root.containsKey("effect")) {
      firstRun=true;
      effect = root["effect"];
    }

     
   if (root.containsKey("kelvin")) {
      firstRun=true;
      tempKelvin  = root["kelvin"];
      effect = "kelvin";
   }
 
  effectString=effect;

  if (stateOn) {

    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
  }
  else {

    realRed = 0;
    realGreen = 0;
    realBlue = 0;
  }

  root.clear(); 
  sendState();
}



/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonDocument<512> root;

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  root["bright"] = brightness;
  root["effect"] = effect;
  root["stepTime"] = stepTime;
  root["p0"] = p0_curr;
  root["p1"] = p1_curr;
  root["kelvin"] = tempKelvin;
  
  root["vers"] = version;

  
     
  char buffer[512];
  serializeJson(root, buffer);

  mqClient.publish(state_topic, buffer, true);

  root.clear();
}



///////////////////////////////////////////////////////////////////////////////
// send a message to mq
void sendDbg(String msg){
  StaticJsonDocument<512> doc;
 
  doc["dbg"]=msg;
  

  char buffer[512];
  size_t n = serializeJson(doc, buffer);

  mqClient.publish(dbg_topic, buffer, n);
}

// called out of timed_loop async
void checkDebug(){
  if (msgCount>0){
    
    String message = arrMessages[0];

     for (int i = 0; i < numMsg-1; i++) {
      arrMessages[i]=arrMessages[i+1];
    }
    arrMessages[numMsg-1]="";
    msgCount--;
    sendDbg(message);
  }
  
  
}

// stuff the line into an array. Another function will send it to mq later
void debug(String dbgMsg, boolean withSerial){
   
  if (withSerial) {
    Serial.println( dbgMsg );
  }
  if (msgCount<numMsg){
    arrMessages[msgCount]=dbgMsg;
    msgCount++;
  }
  
}



/////////////////////////////////////////////////////////////////

void setupWifi(){
  // Connect to WiFi

  // make sure there is no default AP set up unintendedly
  WiFi.mode(WIFI_STA);
  //WiFi.hostname(HOSTNAME);
  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  msg = "WiFi connected, local IP: "+WiFi.localIP().toString();
  debug(msg,true);
  
}


void setupMq(){
  // pubsub setup
  mqClient.setServer(mqtt_server, mqtt_port);
  mqClient.setCallback(callback);
  mqConnect();  
}


void setupOta(){

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    debug("Starting OTA",false);
  });
  ArduinoOTA.onEnd([]() {
    debug("End OTA",false);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) debug("OTA Auth Failed",false);
    else if (error == OTA_BEGIN_ERROR) debug("OTA Begin Failed",false);
    else if (error == OTA_CONNECT_ERROR) debug("OTA Connect Failed",false);
    else if (error == OTA_RECEIVE_ERROR) debug("OTA Receive Failed",false);
    else if (error == OTA_END_ERROR) debug("OTA End Failed",false);
  });
  ArduinoOTA.begin();
  
}




/********************************** START mosquitto *****************************************/

void mqConnect() {
  // Loop until we're reconnected
  while (!mqClient.connected()) {

    // Attempt to connect
    if (mqClient.connect(SENSORNAME, mqtt_username, mqtt_password)) {
      
      mqClient.subscribe(set_topic);      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}
