# ESP-WIFI-NEOPIXEL-CONTROL
NodeMCU script to control NeoPixel ws2812b led strips.<br/>
Control colors, brightness and some effects with HTTP or MQTT calls.

<h3>Video of Script in Action</h3>
https://www.youtube.com/watch?v=9A8iUpCb1MI
<br/>Alexa control in the video is not part of this description.

<h3>Config</h3>
The http server gets initialzed on port 5001, this can be changed in the code.<br/>
You should change the following variables to your needs:<br/>
- wifi_ssid<br/>
- wifi_password<br/>
- mqtt_server<br/>
- mqtt_user<br/>
- mqtt_password<br/>
- OTA_hostname<br/>
- OTA_password<br/>
- PixelCount

<br/><br/>
<h1>MQTT VERSION</h1>

The MQTT version uses 3 topics:<br/>
home/ledcontroller/ -> you will receive status changes on this topic <br/>
home/ledcontroller/set -> control effects, brightness, animation, color on this topic <br/>
home/ledcontroller/log -> receive log messages and errors on this topic 

<h2>Control LED effects</h2>
Send JSON commands to the MQTT SET topic to change the effect and brightness:

Set animation FUN:
<pre>{"animation":"fun"}</pre>
Possible values: off, beam, fun, cylon, pulse, fire, aqua

Set brightness (0-100):
<pre>{"brightness":20}</pre>

Set static color with predefined color values:
<pre>{"animation":"colorred"}
{"animation":"colorblue"}
{"animation":"colorgreen"}
{"animation":"colorwhite"}
{"animation":"colorblack"}</pre>

Set static color with RGB values:
<pre>{"animation":"color","color":{"r":200,"g":200,"b":10}}</pre>

Set all at once:
<pre>{"animation":"color","brightness":34,"color":{"r":200,"g":200,"b":10}}</pre>

Note: color values only get considered when animation=color is selected

<h2>Status of LED controller</h2>
During startup and in case an effect changes the controller sends a status message to the MQTT topic.
The status in a JSON message and looks like the following.
The "uptime" value are the milliseconds since the controller was started, "uptimeH" are the hours since it was started.
The "brightness" value is the percent value between 0 and 100, brightnessraw is the actual set value between 0 and 255
<pre>
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
</pre>


<br/><br/>
<h1>Example OpenHAB2 Config</h1>


<h3>ITEMS</h3>
<pre>
String  Light_L_Digiledstrip_Anim       "Current Animation [%s]"        <light>  {mqtt="<[openhab2:home/ledcontroller:state:JSONPATH($.animation)]"}
String  Light_L_Digiledstrip_Bright     "Brightness [%s]"               <dimmablelight>  {mqtt="<[openhab2:home/ledcontroller:state:JSONPATH($.brightness)]"}
String  Light_L_Digiledstrip_Uptime     "Uptime [%s h]"                 <clock>  {mqtt="<[openhab2:home/ledcontroller:state:JSONPATH($.uptimeH)]"}
Color   Light_L_Digiledstrip_Color      "Color"                         <colorwheel>

String  Light_L_Digiledstrip            "LED Animation"                <light> { mqtt=">[openhab2:home/ledcontroller/set:command:*:{animation\\:${command}}]" }
</pre>


<h3>SITEMAP</h3>
<pre>
Frame label="LED Light" icon="light" {
  Selection item=Light_L_Digiledstrip mappings=[off="off", colorblue="Movie", beam="Beam", fun="Party", cylon="Cylon", pulse="Pulse", fire="Fire", aqua="Aqua"]
  Colorpicker item=Light_L_Digiledstrip_Color
  Slider item=Light_L_Digiledstrip_Bright
  Text item=Light_L_Digiledstrip_Uptime
}

</pre>


<h3>RULES</h3>
<pre>
import java.awt.Color

rule "LED controller Brightness"
when
  Item Light_L_Digiledstrip_Bright received command
then
  logInfo( "FILE", "RULE: LED controller Brightness triggered")
  publish("openhab2","home/ledcontroller/set","{brightness:" + Light_L_Digiledstrip_Bright.state + "}")
end

rule "LED controller Color"
when
  Item Light_L_Digiledstrip_Color received command
then
  logInfo( "FILE", "RULE: LED controller Color triggered")
  var hsbValue = Light_L_Digiledstrip_Color.state as HSBType
  var Color color = Color::getHSBColor(hsbValue.hue.floatValue / 360, hsbValue.saturation.floatValue / 100, hsbValue.brightness.floatValue / 100)

  var String redValue   = String.format("%03d", ((color.red.floatValue / 2.55).intValue))
  var String greenValue = String.format("%03d", ((color.green.floatValue / 2.55).intValue))
  var String blueValue  = String.format("%03d", ((color.blue.floatValue / 2.55).intValue))
  logInfo("FILE", "RED: "+ redValue + " GREEN: "+ greenValue +  " BLUE: "+ blueValue + "")

  publish("openhab2","home/ledcontroller/set","{animation:color,color:{r:" + redValue + ",g:" + greenValue + ",b:" + blueValue + "}}")
end
</pre>


<h1>3D Printed Case</h1>

See included STL file for a case for the Node MCU V2 (small version)



<h1>WIFI VERSION (legacy)</h1>

<h3>Start Effects</h3>
Effects were mainly taken from the standard NeoPixelBus example library and slightly changed

<pre>http://[ip]:5001/control?animationid=fun
http://[ip]:5001/control?animationid=beam
http://[ip]:5001/control?animationid=fire
http://[ip]:5001/control?animationid=aqua
http://[ip]:5001/control?animationid=pulse
http://[ip]:5001/control?animationid=cylon</pre>


<h3>Set Predefined Colors</h3>

<pre>http://[ip]:5001/control?animationid=colorred
http://[ip]:5001/control?animationid=colorblue
http://[ip]:5001/control?animationid=colorgreen
http://[ip]:5001/control?animationid=colorblack
http://[ip]:5001/control?animationid=colorwhite</pre>

<h3>Set Custom Colors</h3>
You can add an RGB color code (9 digits) at the end of the color command to set a custom color

<pre>http://[ip]:5001/control?animationid=color255255255</pre>

<h3>Set Brightness</h3>
Value is a percent value between 0 and 100)

<pre>http://[ip]:5001/control?brightness=20</pre>

<h3>Turn LED Strip off</h3>

<pre>http://[ip]:5001/control?animationid=off</pre>

<h3>Get current status as JSON with</h3>

<pre>http://[ip]:5001/control?status</pre>
The returned message is the same as in the MQTT version




<br/><br/>
<strong>Stefan Schmidhammer 2017</strong>

