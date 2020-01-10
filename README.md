# Tally light
Diy wireless tally-light for VMIX based on nodemcu

At our sports club we use VMIX to report live (via You Tube) of matches of our first team.
In the 'real' television world the cameras have a red light so that the cameraman knows that 'his' camera is live.
We use three cameras for this reporting and so it is handy for our camera people to have this application.

VMIX itself provides a solution with which you can, for example, use a telephone as a tally light. An inexpensive but not very handy ; 
- battery live 
- screen saver 
- Size 
- attachment to camera

That is why I started looking for an alternative. Now there are various systems on the market but these all have a decent size
price tag. That is why I started experimenting with a raspberry pi. I thought: What the telephone version can do must also be on one
raspberry can. Later on I saw the possibilities of the nodemcu. Much cheaper (under 3 euro). I ported the python script to arduino

# What do you need :
- Nodemcu board
- 1 led
- 1 resistor 220 ohm

Connect the LED and Resistor to the NODEMCU. Upload the Sketch. This sketch makes use of autoconnect. While the nodemcu is not connected to a router is start as an accesspoint. Connect your phone to the accesspoint (esp8266AP) an then you can configure ssid and password of your own router. Besides you can configure the parameters for the tally light which are :
- url of the VMIX host (see documentation of webcontroller VMIX)
- Name of camera in your vmix preset

