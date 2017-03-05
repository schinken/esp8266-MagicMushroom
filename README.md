# MagicMushroom

MagicMushroom is a simple pushbutton containing an ESP8266 chip. The Chip connects to the wifi and provides a webinterface to configure soundfiles. These filenames are sent on a specific mqtt topic which is then played by our music system

## settings.h

You have to copy the settings.h.example to settings.h and replace the strings with your credentials

## dependencies

* PubSubClient
* StandardCPlusPlus
* Arduino/ESP8266
