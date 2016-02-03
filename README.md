# MagicMushroom

MagicMushroom is a simple pushbutton containing an ESP8266 chip. The Chip connects to the wifi and provides a webinterface to configure soundfiles. These filenames are sent on a specific mqtt topic which is then played by our music system

## config.h

You have to copy the config.h.example to config.h and replace the strings with your credentials

## dependencies

* PubSubClient
* StandardCPlusPlus
* Arduino/ESP8266
