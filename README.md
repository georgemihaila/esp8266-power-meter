# ESP8266 Power meter
# What is this?
Got one of those power meters at home that blink an LED? You can track and monitor your home power consumption using an ESP8266 and a DSMR Reader server instance. 
# Requirements

  - a local [DSMR Reader](https://github.com/dennissiemensma/dsmr-reader) instance
  - 1x ESP8266
  - 1x 4.7k ohm resistor
  - 1x photoresistor
  - 1x SSD1306 OLED Display
  - 1x DS1307 RTC


# Setup
- ESP8266
1. Wiring
 <br/>
 
  Wiring should be pretty straightforward. [Pull the photoresistor down](https://cdn.instructables.com/F8S/AYSW/J4YFZ2UB/F8SAYSWJ4YFZ2UB.LARGE.jpg?auto=webp&frame=1&width=831&fit=bounds) using the resistor and use A0 as input. The display and the RTC both use I2C, so they can be wired in parallel. For the NodeMCU, the SCL pin is D1 and the SDA pin is D2.
  <br/>
  Final result:
  ![alt text](https://i.imgur.com/3xcmRjk.jpg "Final result")
 <br/>
 
2. Code

 <br/>
 
  Upload the code on the ESP8266 and you're good to go.
 <br/>
 
**How does the code work?**
<br/>
 
 At startup,
1. The SSD1306 is initialized
2. A wireless connection is established
3. The latest meter position is taken from the DSMR Reader server
4. [World Time API](http://worldtimeapi.org) is used for synchronizing the DS1307 if necessary
5. A calibration is done.

During runtime, the ESP8266 calculates the consumption based on the interval between two LED blinks, assuming a 1W / blink base meter level. It then logs the reading to the server and displays it.
<br/>

# Results

I've been running this for quite a while. This is how my DSMR Reader dashboard looks like. It's ~~pretty~~ very accurate. 

![alt text](https://i.imgur.com/denbdtQ.png "Results")
