/*
NodeMCU script to control NeoPixel ws2812b led strips.
Control colors, brightness and some effects with HTTP calls.
  _____                 
 |_   ____ _ _ _ _ __ _ 
   | |/ -_| '_| '_/ _` |
   |_|\___|_| |_| \__,_|
Stefan Schmidhammer 2017
GPL-3.0

Some animation effects are based upon the NeoPixelBus library Examples published by Michael Miller / Makuna at
https://github.com/Makuna/NeoPixelBus

Fire and Aqua effect based on Examples by John Wall
www.walltech.cc/neopixel-fire-effect/
*/

/***************************************************** LIBRARIES ***/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <NeoPixelAnimator.h>
#include <NeoPixelBrightnessBus.h>


/************************************* WIFI AND MQTT INFORMATION ***/
#define wifi_ssid "<wifi_ssid>"
#define wifi_password "<wifi_password>"

#define mqtt_server "<mqtt_brokerip>"
#define mqtt_user "<mqtt_user>"
#define mqtt_password "<mqtt_password>"
#define mqtt_port 1883
#define mqtt_topic_state "home/ledcontroller"
#define mqtt_topic_set "home/ledcontroller/set"
#define mqtt_topic_log "home/ledcontroller/log"

/**************************************************** OTA UPDATE ***/
#define OTA_hostname "<OTA_hostname>"
#define OTA_password "<OTA_password>"
const uint16_t OTA_port = 8266;

/**************************************** NEOPIXELBUS/LED CONFIG ***/
const uint16_t PixelCount = 250;
const uint16_t PixelPin   = 2; //ignored in case of ESP8266

/****************************************** USER DEFINED CLASSES ***/
struct MyAnimationState {
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel;
};

/*************************************** GLOBAL VARS AND OBJECTS ***/
//init brightness levels
uint16_t brightnessLevelPercent = 40;
uint16_t brightnessLevel = brightnessLevelPercent*2.55;

//some control and state variables
bool StopSignReceived           = false;
bool WaitForReset               = false;
unsigned long WaitForResetTimer = false;
bool StartAnimationOnce         = true;
const uint16_t ResetAnimationDuration = 300; //change this to make the switch to other animations faster or slower
const int MQTT_BUFFER_SIZE = 500;
float startmillis;

//some default colors
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor white(255);
RgbColor black(0);

//default animation on startup
String definedAnimation     = "off";
String definedAnimationNext = "off";
String ColorOfStripStr      = "";
RgbColor ColorOfStrip       = green;

//valid animation array
String AnimTypes[] = {"fun","pulse","cylon","color","fire","aqua","beam","off"};
uint16_t AnimCount = 8;

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(PixelCount);
MyAnimationState animationState[PixelCount];
WiFiClient espClient;
PubSubClient client(espClient);

#define MQTT_MAX_PACKET_SIZE 512
//note that you may have to change the MAX_PACKET_SIZE in the header file 
//of the library itself


// ################################################################################################################
// ################################################################################################################
// USER DEFINED FRAMEWORK FUNCTIONS
// ################################################################################################################
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  sendLog("OK", "Node has started");

}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJsonMessage(message)) {
    return;
  }

}

bool processJsonMessage(char* message) {

  StaticJsonBuffer<MQTT_BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    sendLog("FAIL","parseObject() failed");
    return false;
  }


  if ( root.containsKey("animation") ) {

    String animationType = "";
    animationType = root["animation"].asString();
    if( animationType != definedAnimation || animationType.indexOf("color") == 0 ) {
  
      String ColorOfStripStr  = "";
      if( animationType.indexOf("color") == 0 ) {

        ColorOfStripStr = animationType.substring( 5 , (animationType.length()) ); //substring colorcode or name
        animationType   = "color";
  
        if( ColorOfStripStr == "red" ) {
          ColorOfStrip = red;
        } else if( ColorOfStripStr == "blue" ) {
          ColorOfStrip = blue;
        } else if( ColorOfStripStr == "green" ) {
          ColorOfStrip = green;
        } else if( ColorOfStripStr == "black" ) {
          ColorOfStrip = black;
        } else if( ColorOfStripStr == "white" ) {
          ColorOfStrip = white;
        } else if( root.containsKey("color") ) {
          String r = root["color"]["r"].asString();
          String g = root["color"]["g"].asString();
          String b = root["color"]["b"].asString();
          if( isValidNumber(r+g+b) ) {
            ColorOfStrip = RgbColor( r.toInt() , g.toInt() , b.toInt() );
          }
        }
  
      }

      if( !inArray(AnimTypes, AnimCount, animationType) ) {
        sendLog("FAIL","Invalid animation received, fallback to Fire animation: " + animationType);
        definedAnimationNext = "fire";
        animationType        = "fire";
      }
  
      sendLog("OK","Set new animation '" + animationType + "' with optional strip color '" + ColorOfStripStr + "'");
      StopSignReceived     = true;
      definedAnimationNext = animationType;
      definedAnimation     = "";
      StartAnimationOnce   = true;
  
    }

  }
  
  if ( root.containsKey("brightness") ) {

    String brightnessLevelStr    = root["brightness"].asString();
    if( brightnessLevelStr.length() <= 3 && brightnessLevelStr.length() >= 1 && isValidNumber(brightnessLevelStr) ) {
      brightnessLevelPercent = brightnessLevelStr.toInt();
      brightnessLevel = brightnessLevelPercent*2.55;
      if( brightnessLevelPercent >= 0 && brightnessLevelPercent <= 100 ) {
        strip.SetBrightness( brightnessLevel );
        sendLog("OK","Set Brightness: " + brightnessLevel);
        sendState();
      } else {
        sendLog("FAIL","Brightness was not set, invalid value passed, must be 0-100: " + brightnessLevel);
      }
    } else {
      sendLog("FAIL","Brightness was not set, invalid value passed: " + brightnessLevelStr);
    }
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(OTA_hostname, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic_set);
      sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendState() {

  StaticJsonBuffer<MQTT_BUFFER_SIZE> jsonBuffer;

  JsonObject& root        = jsonBuffer.createObject();
  root["uptime"]          = millis();
  root["uptimeH"]         = millis()/1000/60/60;
  root["animation"]       = definedAnimation+definedAnimationNext;
  root["brightness"]      = brightnessLevelPercent;
  root["brightnessraw"]   = brightnessLevel;

  JsonObject& strip_color = root.createNestedObject("color");
  strip_color["r"]        = ColorOfStrip.R;
  strip_color["g"]        = ColorOfStrip.G;
  strip_color["b"]        = ColorOfStrip.B;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(mqtt_topic_state, buffer, true);

}

void sendLog(String code,String message) {

  StaticJsonBuffer<MQTT_BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["host"]     = OTA_hostname;
  root["code"]     = code;
  root["message"]  = message;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(mqtt_topic_log, buffer, true);

}

boolean isValidNumber(String str) {
   for(byte i=0;i<str.length();i++)
   {
       if(!(isDigit(str.charAt(i)) || str.charAt(i) == '+' || str.charAt(i) == '.' || str.charAt(i) == '-')) return false;
   }
   return true;
}

boolean inArray(String arr[], int count, String needle) {
  for(int a=0;a < count;a++) {
    if( arr[a] == needle ) {
      return true;
    }
  }
  return false;
}

// ################################################################################################################
// USER DEFINED FRAMEWORK FUNCTIONS
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// ANIMATION FUN
// ################################################################################################################
void FunLights(float luminance) {
  if(StopSignReceived == false) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = HslColor(random(360) / 360.0f, 1.0f, luminance);
        animations.StartAnimation(pixel, 300, FunLightsAnim);
      }
    }
  }
}

void FunLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// ANIMATION FUN
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// ANIMATION PULSE
// ################################################################################################################
void PulseLights(float luminance) {
  if(StopSignReceived == false) {
    RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = target;
        animations.StartAnimation(pixel, 2000, PulseLightsAnim);
      }
    }
  }
}

void PulseLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// ANIMATION PULSE
// ################################################################################################################
// ################################################################################################################





// ################################################################################################################
// ################################################################################################################
// ANIMATION CYLON
// ################################################################################################################
const RgbColor CylonEyeColor(HtmlColor(0x7f0000));
uint16_t CylonLastPixel = 0; // track the eye position
int8_t CylonMoveDir     = 1; // track the direction of movement
AnimEaseFunction moveEase =
//      NeoEase::Linear;
//      NeoEase::QuadraticInOut;
//      NeoEase::CubicInOut;
        NeoEase::QuarticInOut;
//      NeoEase::QuinticInOut;
//      NeoEase::SinusoidalInOut;
//      NeoEase::ExponentialInOut;
//      NeoEase::CircularInOut;
void CylonLights(float luminance) {
  if(StopSignReceived == false && StartAnimationOnce == true) {
    animations.StartAnimation(0, 5, CylonFadeAnimUpdate);
    animations.StartAnimation(1, 3000, CylonMoveAnimUpdate);
    StartAnimationOnce = false;
  }
}

void CylonFadeAll(uint8_t darkenBy) {
  if(StopSignReceived == false) {
    RgbColor color;
    for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++) {
        color = strip.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        strip.SetPixelColor(indexPixel, color);
    }
  }
}

void CylonFadeAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {
    if (param.state == AnimationState_Completed) {
        CylonFadeAll(10);
        animations.RestartAnimation(param.index);
    }
  }
}

void CylonMoveAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {

    // apply the movement animation curve
    float progress = moveEase(param.progress);

    // use the curved progress to calculate the pixel to effect
    uint16_t CylonNextPixel;
    if (CylonMoveDir > 0)
    {
        CylonNextPixel = progress * PixelCount;
    }
    else
    {
        CylonNextPixel = (1.0f - progress) * PixelCount;
    }

    // if progress moves fast enough, we may move more than
    // one pixel, so we update all between the calculated and
    // the last
    if (CylonLastPixel != CylonNextPixel)
    {
        for (uint16_t i = CylonLastPixel + CylonMoveDir; i != CylonNextPixel; i += CylonMoveDir)
        {
            strip.SetPixelColor(i, CylonEyeColor);
        }
    }
    strip.SetPixelColor(CylonNextPixel, CylonEyeColor);

    CylonLastPixel = CylonNextPixel;

    if (param.state == AnimationState_Completed)
    {
        // reverse direction of movement
        CylonMoveDir *= -1;

        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
    }

  }
}
// ################################################################################################################
// ANIMATION CYLON
// ################################################################################################################
// ################################################################################################################



// ################################################################################################################
// ################################################################################################################
// ANIMATION COLOR
// ################################################################################################################
void ColorLights(float luminance) {
  if(StopSignReceived == false && StartAnimationOnce == true) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = ColorOfStrip;
        animations.StartAnimation(pixel, ResetAnimationDuration, ColorLightsAnim);
      }
    }
    StartAnimationOnce = false;
  }
}

void ColorLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// ANIMATION COLOR
// ################################################################################################################
// ################################################################################################################



// ################################################################################################################
// ################################################################################################################
// ANIMATION FIRE
// ################################################################################################################

uint16_t fire_r = 255;
uint16_t fire_g = fire_r-180;
uint16_t fire_b = 40;
uint16_t fire_flicker;
int16_t fire_r_fadein;
int16_t fire_g_fadein;
int16_t fire_b_fadein;
void FireLights(float luminance) {
  if(StopSignReceived == false) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        fire_flicker = random(0,150);
        fire_r_fadein = fire_r-fire_flicker;
        fire_g_fadein = fire_g-fire_flicker;
        fire_b_fadein = fire_b-fire_flicker;
        if(fire_g_fadein<0) fire_g_fadein=0;
        if(fire_r_fadein<0) fire_r_fadein=0;
        if(fire_b_fadein<0) fire_b_fadein=0;
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = RgbColor(fire_r_fadein, fire_g_fadein, fire_b_fadein);
        animations.StartAnimation(pixel, random(100,300), FireLightsAnim);
      }
    }
  }
}

void FireLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// ANIMATION FIRE
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// ANIMATION AQUA
// ################################################################################################################

uint16_t aqua_r = 10;
uint16_t aqua_g = aqua_r-180;
uint16_t aqua_b = 255;
uint16_t aqua_flicker;
int16_t aqua_r_fadein;
int16_t aqua_g_fadein;
int16_t aqua_b_fadein;
void AquaLights(float luminance) {
  if(StopSignReceived == false) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        aqua_flicker = random(0,200);
        aqua_r_fadein = aqua_r-aqua_flicker;
        aqua_g_fadein = aqua_g-aqua_flicker;
        aqua_b_fadein = aqua_b-aqua_flicker;
        if(aqua_g_fadein<0) aqua_g_fadein=0;
        if(aqua_r_fadein<0) aqua_r_fadein=0;
        if(aqua_b_fadein<0) aqua_b_fadein=0;
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = RgbColor(aqua_r_fadein, aqua_g_fadein, aqua_b_fadein);
        animations.StartAnimation(pixel, random(100,300), AquaLightsAnim);
      }
    }
  }
}

void AquaLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// ANIMATION AQUA
// ################################################################################################################
// ################################################################################################################


// ################################################################################################################
// ################################################################################################################
// ANIMATION BEAM
// ################################################################################################################
uint16_t BeamFrontPixel = 0;  // the front of the loop
RgbColor BeamFrontColor;  // the color at the front of the loop
const uint16_t BeamPixelFadeDuration = 300;
const uint16_t BeamNextPixelMoveDuration = 1000 / PixelCount;
void BeamLights(float luminance) {
  if(StopSignReceived == false && StartAnimationOnce == true) {
    animations.StartAnimation(0, BeamNextPixelMoveDuration, BeamLoopAnimUpdate);
    StartAnimationOnce = false;
  }
}

void BeamLoopAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {
    if (param.state == AnimationState_Completed) {
      animations.RestartAnimation(param.index);
      BeamFrontPixel = (BeamFrontPixel + 1) % PixelCount; // increment and wrap
      if (BeamFrontPixel == 0) {
          BeamFrontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
      }
      uint16_t indexAnim;
      if (animations.NextAvailableAnimation(&indexAnim, 1)) {
          animationState[indexAnim].StartingColor = BeamFrontColor;
          animationState[indexAnim].EndingColor   = RgbColor(0, 0, 0);
          animationState[indexAnim].IndexPixel    = BeamFrontPixel;
          animations.StartAnimation(indexAnim, BeamPixelFadeDuration, BeamFadeOutAnimUpdate);
      }
    }
  }
}

void BeamFadeOutAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(animationState[param.index].IndexPixel, updatedColor);
  }
}
// ################################################################################################################
// ANIMATION BEAM
// ################################################################################################################
// ################################################################################################################


// ################################################################################################################
// ################################################################################################################
// ANIMATION STOP
// ################################################################################################################
void stopAllAnimations() {
  //Serial.println("stop animation defined, running for " + String(ResetAnimationDuration) + "ms");
  for(int pixel=0; pixel < PixelCount; pixel++) {
    animations.StopAnimation(pixel);
    animations.StartAnimation(pixel, ResetAnimationDuration, stopAllAnimationsAnim);
  }
  StopSignReceived = false;
}

void stopAllAnimationsAnim(const AnimationParam& param) {
  strip.SetPixelColor(param.index, black);
}
// ################################################################################################################
// ANIMATION STOP
// ################################################################################################################
// ################################################################################################################










void setup() {

  Serial.begin(115200);
  delay(4);

  ArduinoOTA.setPort(OTA_port);
  ArduinoOTA.setHostname(OTA_hostname);
  ArduinoOTA.setPassword((const char *)OTA_password);

  Serial.println("Starting Node named " + String(OTA_hostname));

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);


  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());


  strip.Begin();
  strip.Show();
  strip.SetBrightness(brightnessLevel);

  startmillis = millis();

}

void loop() {

  if( definedAnimationNext != "" ) {
    definedAnimation     = definedAnimationNext;
    definedAnimationNext = "";
    StartAnimationOnce   = true;
  }

  if( StopSignReceived == true) {
    stopAllAnimations();
    WaitForReset = true;
    WaitForResetTimer = millis();
  }

  if( WaitForReset && (WaitForResetTimer+ResetAnimationDuration+100) < millis() ) { //add 100ms for safety
    WaitForResetTimer = 0;
    WaitForReset      = false;
    //sendState();
  }

  if( StopSignReceived == false && WaitForReset == false ) {

    if( definedAnimation == "fun" ) {
      FunLights(0.25f);
    } else if( definedAnimation == "pulse" ) {
      PulseLights(0.25f);
    } else if( definedAnimation == "cylon" ) {
      CylonLights(0.25f);
    } else if( definedAnimation == "beam" ) {
      BeamLights(0.25f);
    } else if( definedAnimation == "color" ) {
      ColorLights(0.25f);
    } else if( definedAnimation == "fire" ) {
      FireLights(0.25f);
    } else if( definedAnimation == "aqua" ) {
      AquaLights(0.25f);
    } else if( definedAnimation == "off" ) {
      ColorOfStripStr = "black";
      ColorOfStrip = black;
      ColorLights(0.25f);
    }

  }

  animations.UpdateAnimations();  
  strip.Show();


  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
 
}
