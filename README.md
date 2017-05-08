# ESP-WIFI-NEOPIXEL-CONTROL
NodeMCU script to control NeoPixel ws2812b led strips.
Control colors, brightness and some effects with HTTP calls.
<br/><br/>
<strong>Video of Script in Action</strong><br/>https://www.youtube.com/watch?v=9A8iUpCb1MI
<br/><br/>
Example Calls below</strong>
The http server gets initialzed on port 5001, this can be changed in the code.
You should change the following variables to your needs:
- wifi_ssid
- wifi_password
- PixelCount
- PixelPin (note that the pin is fixed to the RX pin on an ESP board, this config will be ignored on ESP boards)
<br/><br/>
<h1>WIFI VERSION</h1>
<br/><br/><br/>
<strong>Start Effects</strong><br/>Effects were mainly taken from the standard NeoPixelBus example library and slightly changed

http://[ip]:5001/control?animationid=fun

http://[ip]:5001/control?animationid=beam

http://[ip]:5001/control?animationid=fire

http://[ip]:5001/control?animationid=aqua

http://[ip]:5001/control?animationid=pulse

http://[ip]:5001/control?animationid=cylon

<br/><br/><br/>
<strong>Set Predefined Colors</strong>

http://[ip]:5001/control?animationid=colorred

http://[ip]:5001/control?animationid=colorblue

http://[ip]:5001/control?animationid=colorgreen

http://[ip]:5001/control?animationid=colorblack

http://[ip]:5001/control?animationid=colorwhite



<br/><br/><br/>
<strong>Set Custom Colors</strong><br/>You can add an RGB color code (9 digits) at the end of the color command to set a custom color

http://[ip]:5001/control?animationid=color255255255

<br/><br/><br/>
<strong>Set Brightness</strong><br/>Value is a percent value between 0 and 100)

http://[ip]:5001/control?brightness=20



<br/><br/><br/>
<strong>Turn LED Strip off</strong>

http://[ip]:5001/control?animationid=off



<br/><br/><br/>
<strong>Get current status as JSON with</strong>

http://[ip]:5001/control?status


<br/><br/>
<h1>MQTT VERSION</h1>
<pre>
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
</pre>

<br/><br/>
<h1>OpenHAB2 Config</h1>


<br/><br/>
<h3>ITEMS</h3>


<br/><br/>
<h3>SITEMAP</h3>


<br/><br/>
<h3>RULES</h3>




<br/><br/><br/>
<strong>Stefan Schmidhammer 2017</strong>

