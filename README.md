# ESP8266 Power meter
# What is this?
Got one of those power meters at home that blink an LED? You can track and monitor your home power consumption using an ESP8266 and a DSMR Reader server instance. 
# Requirements

  - any version of [ASP.NET Core](https://dotnet.microsoft.com/download)
  - a local [DSMR Reader](https://github.com/dennissiemensma/dsmr-reader) instance
  - an ESP8266, a 4.7k ohm resistor (or any other value) and a photoresistor


# Setup
- ESP8266
1. Wiring
Wire the ESP8266 like this and place the photoresistor over the power meter's LED. If the meter is not in a place where there's relatively constant light, you'll want to somehow seal it or there will be interference.  
![alt text](https://cdn.instructables.com/F8S/AYSW/J4YFZ2UB/F8SAYSWJ4YFZ2UB.LARGE.jpg?auto=webp&frame=1&width=831&fit=bounds "Wiring")
2. Code
Upload the code on the ESP8266 and you're good to go.
**How does the code work?**
At startup, the ESP8266 does a calibration. This means that it should be powered up only when sure the meter's LED will not be blinking. 
**How does it calibrate?**
The meter's LED off state is considered a base reading. Given that we are using an analog input, there will be noise. The ESP8266 does 100 readings at an interval of 10 ms and calculates the mean and the standard deviation of the readings. Whenever the meter's LED blinks, it gives a reading that is beyond + or - 2 standard deviations (depending on whether the photoresistor is pulled up or down by the resistor - the code accounts for either of these possibilities). When this happens, a POST request is made to the local ASP.NET Core middleware, which logs the reading, groups readings into days and forwards the request to the DSMR Reader server instance (yes, the middleware can be removed, but this requires some timing calculations on the ESP8266 which were 1000x easier to write in C#). The ESP8266 then waits for the reading to come back within normal range (<2 SDs) and waits for the next LED blink.

# Results

I've been running this power meter for the past 2 weeks and I've had no problems with it. This is an example of how my DSMR Reader dashboard looks like. Here, you can see the interval I turned my microwave oven on, when my fridge runs etc. It's ~~pretty~~ very accurate. 

![alt text](https://i.imgur.com/denbdtQ.png "Results")
