# AJARemote

**Update:**
 AJARemote now includes a web server for changing WiFi or MQTT credentials without the need of a full reset. More importantly it also provides the ability to upgrade firmware in the event of updates. Your AJARemote is now future proofed!!

The btLE keyboard functionality has been removed. Please contact me if this is needed.


**AJARemote** is part of the AJAOne family of devices that use the ESP32 and custom firmware to interface with an MQTT Broker. The AJARemote is used to control most devices that use Infrared commands such as TVs, Blue ray players, AV Receivers, LED lighting, automated curtains and so on.

Products such as Home Assistant, Node-Red and Mosquitto MQTT broker running on a Raspberry PI, and one or more AJARemotes, make an excellent platform to automate anything controllable via Infrared.

Each AJARemote has a built-in super bright 5MM 940nm infrared LED as well as an 38KHz infrared receiver. In addition, there is a 3.5mm jack to allow for additional infrared LEDs to be attached. These are available from Amazon

The AJARemote can also capture IR codes from your existing IR Remotes. Just point the remote at the AJARemote and press a button. The IR Codes as well as the PRONTO codes will be sent to your MQTT Broker. Couldn't be easier to replace all of your numerous remotes.


Please not that although the InfraRed library is very extensive there is no guarantee that every device can be controlled.


## Sample
Sample captured IRCode:
`{"protocol":"SONY","data":"0x42B47","bits":"20"}`

And the associated Pronto codes:
`{"protocol":"PRONTO","data":"0000 006D 0015 0000 005E 0015 0019 0015 002E 0015 0019 0015 0019 0015 0019 0015 0019 0015 0030 0015 0019 0013 0030 0015 0019 0015 0030 0015 0030 0015 0019 0015 002E 0015 0019 0015 0019 0015 0019 0015 0030 0015 0030 0013 0030 06C3 ","bits":"0"}`

These codes can be directly used in Node-Red as JSON values to create sequences to control everything.

An AJARemote, via Node-Red can also be used as a repeater. For example if your audio rack is at the rear of the room, an AJARemote at the front of the room can forward messages to an AJARemote at the rear of the room.


Join our Discord server : https://discord.gg/SV6XM7mU3p if you have questions.

[Tindie Store](https://www.tindie.com/products/nicktucker42/ajaremote-mqtt-driven-ir-blaster/)

