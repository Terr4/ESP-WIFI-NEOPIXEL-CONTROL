# ESP-WIFI-NEOPIXEL-CONTROL
NodeMCU script to control NeoPixel ws2812b led strips.
Control colors, brightness and some effects with HTTP calls.


Example Calls below</strong>
The http server gets initialzed on port 5001, this can be changed in the code.
You should change the following variables to your needs:
- wifi_ssid
- wifi_password
- PixelCount
- PixelPin (note that the pin is fixed to the RX pin on an ESP board, this config will be ignored on ESP boards)


<strong>Start Effects</strong>
Effects were mainly taken from the standard NeoPixelBus example library and slightly changed

http://<ip>:5001/control?animationid=fun

http://<ip>:5001/control?animationid=beam

http://<ip>:5001/control?animationid=fire

http://<ip>:5001/control?animationid=aqua

http://<ip>:5001/control?animationid=pulse

http://<ip>:5001/control?animationid=cylon

<strong>Set Predefined Colors</strong>

http://<ip>:5001/control?animationid=colorred

http://<ip>:5001/control?animationid=colorblue

http://<ip>:5001/control?animationid=colorgreen

http://<ip>:5001/control?animationid=colorblack

http://<ip>:5001/control?animationid=colorwhite



<strong>Set Custom Colors</strong>
You can add an RGB color code (9 digits) at the end of the color command to set a custom color

http://<ip>:5001/control?animationid=color255255255

<strong>Set Brightness</strong>
Value is a percent value between 0 and 100)

http://<ip>:5001/control?brightness=20



<strong>Turn LED Strip off</strong>
http://<ip>:5001/control?animationid=off



<strong>Get current status with</strong>
http://<ip>:5001/control?status



<strong>Stefan Schmidhammer 2017</strong>

