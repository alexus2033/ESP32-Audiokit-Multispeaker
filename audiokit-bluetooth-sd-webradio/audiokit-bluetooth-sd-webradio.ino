/**
 * @file audiokit-bluetooth-sd-webradio.ino
 * @author Alexus2033
 * @brief Multimode AMP Board with ESP32-A1S
 * @version 0.1
 * @date 2023-01-03
 * 
 * @copyright (c) 2023 
 */

// install https://github.com/greiman/SdFat.git

#define AUDIOKIT_BOARD 5
#define SD_CARD_INTR_GPIO 34
#define USE_NEXTION_DISPLAY 1
#define HELIX_LOGGING_ACTIVE false
  
//additional serial output
#define DEBUG 0
#define ModeWebRadio 1
#define ModeSDPlayer 2
#define ModeBTSpeaker 3

#if DEBUG == 1
  #define debug(x) Serial.print(x)
  #define debugfm(x,y) Serial.print(x,y)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x)
  #define debugln(x)
#endif

#if USE_NEXTION_DISPLAY == 0
  #define dispText(x,y)
#endif

#include "AudioTools.h"
#include "AudioLibs/AudioA2DP.h"
#include "AudioLibs/AudioKit.h"
#include "AudioLibs/AudioSourceSDFAT.h"
#include "AudioCodecs/CodecMP3Helix.h"

const char *urls[] = {
  "http://stream.electroradio.fm/192k",
  "http://stream.srg-ssr.ch/m/rsj/mp3_128",
  "http://stream.srg-ssr.ch/m/drs3/mp3_128",
  "http://sunshineradio.ice.infomaniak.ch/sunshineradio-128.mp3",
  "http://streaming.swisstxt.ch/m/drsvirus/mp3_128"
};

//Prototypes
void btnChangeMode(bool, int, void*);
void btnPrevious(bool, int, void*);
void btnNext(bool, int, void*);

AudioKitStream kit; //Board

//SD-Player
const char* startFilePath="/";
const char* ext="mp3";
SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS, DEDICATED_SPI, SD_SCK_MHZ(10) , &AUDIOKIT_SD_SPI);
AudioSourceSDFAT sourceSD(startFilePath, ext, sdcfg);
char displayName[40];

//Web-Radio
ICYStream inetStream(2048);
AudioSourceURL sourceRadio(inetStream, urls, "audio/mp3");
char network[50];
char passwd[50]; //read details from SD-Card

//MP3 & Player
MP3DecoderHelix decoder;
AudioPlayer *player = nullptr;
BluetoothA2DPSink a2dp_sink;

byte player_mode = ModeWebRadio;
bool pin_request = false;

////// SETUP
void setup() {
  Serial.begin(9600);
  AudioLogger::instance().begin(Serial, AudioLogger::Error);
  // LOGLEVEL_AUDIOKIT = AudioKitError;
  
  // provide a2dp data
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_metadata_callback(bt_metadata_callback);
  a2dp_sink.activate_pin_code(true);
  //a2dp_sink.set_discoverability(ESP_BT_NON_DISCOVERABLE);
  
  // setup output
  // volume can be controlled on the board (kit) and player (Source)
  auto cfg = kit.defaultConfig(TX_MODE);
  kit.setVolume(10); //max:100
  kit.begin(cfg);

  // setup buttons 
  kit.addAction(PIN_KEY1, btnChangeMode);
  // kit.addAction(PIN_KEY2, previous); //GPIO 19 not working
  kit.addAction(PIN_KEY3, btnPrevious);
  kit.addAction(PIN_KEY4, btnNext);

  int detectSDPin = SD_CARD_INTR_GPIO;
  if (detectSDPin>0){
    pinMode(detectSDPin, INPUT_PULLUP);
  }
  readWlanFile();
  startRadioPlayer();
}

// Write data to AudioKit in callback
void read_data_stream(const uint8_t *data, uint32_t length) {
    kit.write(data, length);
}

void bt_metadata_callback(uint8_t id, const uint8_t *text) { 
  if(id == 2){ //Artist
    dispText(1,(char*)text);
    return;
  }
  if(id == 1){ //Title
    dispText(2,(char*)text);
    return;
  }
  debug("META ");
  debug(id); //1,8,16
  debug(": "); 
}

void player_metadata_callback(MetaDataType type, const char* str, int len){
  if(len < 1) return;
  if(type == 0){ //Title
    dispText(1,(char*)str);
    return;
  }
  if(type == 1){ //Album
    dispText(2,(char*)str);
    return;
  }
  if(type == 2){ //Artist
    dispText(2,(char*)str);
    return;
  }
  if(type == 4){ //name
    snprintf(displayName, sizeof(displayName), "%s", str);
    return;    
  }
  debug("META ");
  debug(type);
  debug(" (");
  debug(toStr(type));
  debug("): ");
  debugln(str);
}

// write to Nextion-Display
void dispText(uint8_t id, char* str) {
  String cmd = "\"";
  Serial.print("t");
  Serial.print(id);
  Serial.print(".txt=");
  Serial.print(cmd + str + cmd);
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);
}

//////// LOOP
void loop() {
  if (a2dp_sink.pin_code() != 0) {
    if(pin_request == false){
      dispText(0,"Connect Bluetooth");
      dispText(1,"Confirm PIN");
      char pin[17]; //add leading zero
      snprintf(pin, sizeof(pin), "%06d", a2dp_sink.pin_code());
      dispText(2,pin);
    }
    pin_request = true;
  } else {
    pin_request = false;
  }
  if (player && player_mode != ModeBTSpeaker) {
    player->copy();
  } else {
    // feed watchdog
    delay(10);
  }
  kit.processActions();
}
