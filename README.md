# ESP32-Audiokit-Multispeaker
ESP32-Audiokit Webradio, SD-Player and Bluetooth-Speaker

## Features
Digital Music Player with 3 Modes:
* Listen to [Web Radio Stations](https://wiki.ubuntuusers.de/Internetradio/Stationen) MP3
* Play MP3 from SD-Card
* Connect and play music from Bluetooth-Devices (Receiver)
* Display Status and Song Information

## Used Libraries
* [Arduino-Audiokit](https://github.com/pschatzmann/arduino-audiokit) Amplifier Board, [ES8388](http://www.everest-semi.com/pdf/ES8388%20DS.pdf)
* [Arduino-Audiotools](https://github.com/pschatzmann/arduino-audio-tools) Combine Audio Sources, Decoders via I2S
* [Arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) MP3 Decoder for SD-Card and Radio
* [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) Bluetooth Receiver
* [SdFat](https://github.com/greiman/SdFat.git) Reading Data from SD-Card

## Used Hardware
* [ESP32 LyraT](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat.html) V4.3 OR [Audio Kit Board](https://docs.ai-thinker.com/en/esp32-a1s) V2.2 A237
* [Nextion Display](https://nextion.tech) NX4024T032 (Basic)
