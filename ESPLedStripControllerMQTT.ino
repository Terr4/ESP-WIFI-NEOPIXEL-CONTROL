/*
NodeMCU script to control NeoPixel ws2812b led strips.
Control colors, brightness and some effects with HTTP calls.

Example Calls below.
The http server gets initialzed on port 5001, this can be changed in the code.
You should change the following variables to your needs:
- wifi_ssid
- wifi_password
- PixelCount
- PixelPin (note that the pin is fixed to the RX pin on an ESP board, this config will be ignored on ESP boards)

Control Controller:
{
  "animation":"fun",
  "brightness":34,
  "color":{
    "r":9,
    "g":49,
    "b":9
  }
}
Note: color values only get considered when animation=color is selected
there are some default color "animations" that can be used: colorred, colorgreen, colorblue, colorwhite, colorblack

Status of Controller:
{
  "uptime":143248,
  "uptimeH":0,
  "animation":"fun",
  "brightness":34,
  "brightnessraw":86,
  "color":{
    "r":9,
    "g":49,
    "b":9
  }
}

Stefan Schmidhammer 2017
*/

#include <ESP8266WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <NeoPixelAnimator.h>
#include <NeoPixelBrightnessBus.h>
#include <ctype.h>

/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "<yourwifissid>"
#define wifi_password "<yourwifipassword>"
#define mqtt_server "<brokerip>"
#define mqtt_user "<brokeruser>" 
#define mqtt_password "brokerpassword>"
#define mqtt_port 1883
bool dbg = true;

/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define ledcontroller_state_topic "home/ledcontroller1"
#define ledcontroller_set_topic "home/ledcontroller1/set"

/**************************** FOR OTA **************************************************/
#define SENSORNAME "<OTAname>"
#define OTApassword "<OTApassword>"
int OTAport = 8266;



const int BUFFER_SIZE = 300;


//LED Strip Config
const uint16_t PixelCount = 248;
const uint16_t PixelPin   = 2;

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};


NeoPixelAnimator animations(PixelCount);
MyAnimationState animationState[PixelCount];

RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor white(255);
RgbColor black(0);
uint16_t brightnessLevelPercent = 40;
uint16_t brightnessLevel = brightnessLevelPercent*2.55;
String brightnessLevelStr;

bool StopSignReceived           = false;
bool WaitForReset               = false;
unsigned long WaitForResetTimer = false;
bool StartAnimationOnce         = true;
uint16_t ResetAnimationDuration = 300;

String definedAnimation         = "fun";
String definedAnimationNext     = "fun";
String animationType = "";

String AnimTypes[] = {"fun","pulse","cylon","color","fire","aqua","beam","off"};
uint16_t AnimCount = 8;

float startmillis;



WiFiClient espClient;
PubSubClient client(espClient);



// ################################################################################################################
// ################################################################################################################
// FUN
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
// FUN
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// PULSE
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
// PULSE
// ################################################################################################################
// ################################################################################################################





// ################################################################################################################
// ################################################################################################################
// CYLON
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
// CYLON
// ################################################################################################################
// ################################################################################################################



// ################################################################################################################
// ################################################################################################################
// COLOR
// ################################################################################################################
RgbColor ColorOfStrip  = green;
String ColorOfStripStr = "";
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
// COLOR
// ################################################################################################################
// ################################################################################################################



// ################################################################################################################
// ################################################################################################################
// FIRE
// ################################################################################################################

int fire_r = 255;
int fire_g = fire_r-180;
int fire_b = 40;
int fire_flicker;
int fire_r_fadein;
int fire_g_fadein;
int fire_b_fadein;
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
// FIRE
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// AQUA
// ################################################################################################################

int aqua_r = 10;
int aqua_g = aqua_r-180;
int aqua_b = 255;
int aqua_flicker;
int aqua_r_fadein;
int aqua_g_fadein;
int aqua_b_fadein;
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
// AQUA
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// BEAM
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
// BEAM
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// STOP
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
// STOP
// ################################################################################################################
// ################################################################################################################


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







void setup() {

  if(dbg) {
    Serial.begin(115200);
    delay(4);
  }

  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(SENSORNAME);
  ArduinoOTA.setPassword((const char *)OTApassword);

  Serial.println("Starting Node named " + String(SENSORNAME));

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);


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




/********************************** START SETUP WIFI*****************************************/
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
}



/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
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

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }


  if ( root.containsKey("animation") ) {

    animationType = root["animation"].asString();
    if( animationType != definedAnimation || animationType.indexOf("color") == 0 ) {
  
      ColorOfStripStr  = "";
      if( animationType.indexOf("color") == 0 ) {
  
        //Serial.println("Color Animation found");

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

      //Serial.println("Set Animation: " + animationType);

      if( !inArray(AnimTypes, AnimCount, animationType) ) {
        definedAnimationNext = "fire";
        animationType        = "fire";
        Serial.println("Invalid Animation, fallback to Fire Animation");
      }
  
      //Serial.println("Set new animation: " + animationType + " with color: " + ColorOfStripStr);
      StopSignReceived     = true;
      definedAnimationNext = animationType;
      definedAnimation     = "";
      StartAnimationOnce   = true;
  
    }

  }
  
  if ( root.containsKey("brightness") ) {

    //Serial.print("Try to set Brightness: ");
    //Serial.println(root["brightness"].asString());

    brightnessLevelStr    = root["brightness"].asString();
    if( brightnessLevelStr.length() <= 3 && brightnessLevelStr.length() >= 1 && isValidNumber(brightnessLevelStr) ) {
      brightnessLevelPercent = brightnessLevelStr.toInt();
      brightnessLevel = brightnessLevelPercent*2.55;
      if( brightnessLevelPercent >= 0 && brightnessLevelPercent <= 100 ) {
        strip.SetBrightness( brightnessLevel );
        //Serial.print("Set Brightness: ");
        sendState();
        //client.println("{ \"code\":\"OK\", \"info\":\"brightness set to " + String(brightnessLevelPercent) + "%\" }");
      } else {
        //client.println("{ \"code\":\"FAIL\", \"info\":\"brightness was not set, invalid number range passed\" }");
      }
    } else {
      //client.println("{ \"code\":\"FAIL\", \"info\":\"brightness was not set, invalid value passed\" }");
    }
  }

}


/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(ledcontroller_set_topic);
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

/********************************** START SEND STATE*****************************************/
void sendState() {

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root        = jsonBuffer.createObject();
  root["uptime"]          = millis();
  root["uptimeH"]          = millis()/1000/60/60;
  root["animation"]       = definedAnimation+definedAnimationNext;
  root["brightness"]      = brightnessLevelPercent;
  root["brightnessraw"]   = brightnessLevel;

  JsonObject& color = root.createNestedObject("color");
  color["r"]        = ColorOfStrip.R;
  color["g"]        = ColorOfStrip.G;
  color["b"]        = ColorOfStrip.B;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(ledcontroller_state_topic, buffer, true);

}



// ################################################################################################################
// ################################################################################################################
// GLOBAL FUNCTIONS
// ################################################################################################################

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

