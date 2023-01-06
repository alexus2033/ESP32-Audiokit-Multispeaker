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

//don´t forget to change AudioKitSettings.h
#define AUDIOKIT_BOARD 1  //LyraT
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
  "http://s8-webradio.antenne.de/chillout",
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

char* deviceName = "Boombox-23";
AudioKitStream kit; //Board

//SD-Player
const char* startFilePath="/";
const char* ext="mp3";
SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS, DEDICATED_SPI, SD_SCK_MHZ(10) , &AUDIOKIT_SD_SPI);
AudioSourceSDFAT sourceSD(startFilePath, ext, sdcfg);
char displayName[40];

//Web-Radio
ICYStream inetStream(4096);
AudioSourceURL sourceRadio(inetStream, urls, "audio/mp3");
char network[50];
char passwd[50]; //read details from SD-Card

//MP3 & Player
MP3DecoderHelix decoder;
AudioPlayer *player = nullptr;
BluetoothA2DPSink a2dp_sink;

byte player_mode = ModeWebRadio;
bool player_active = false;
bool pin_request = false;
int sd_index = 0;

////// SETUP
void setup() {
  Serial.begin(9600);
  AudioLogger::instance().begin(Serial, AudioLogger::Error);
  // LOGLEVEL_AUDIOKIT = AudioKitError;
  
  resetDisplay();
  
  // setup A2DP
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_metadata_callback(bt_metadata_callback);
  a2dp_sink.set_on_connection_state_changed(bt_connection_state_changed);
  a2dp_sink.set_on_audio_state_changed(bt_play_state_changed);
  a2dp_sink.activate_pin_code(true);
  //a2dp_sink.set_discoverability(ESP_BT_NON_DISCOVERABLE);
  
  // setup output
  // volume can be controlled on the board (kit) and player (Source)
  auto cfg = kit.defaultConfig(TX_MODE); 
  kit.setVolume(10); //max:100
  kit.begin(cfg);

#if AUDIOKIT_BOARD == 1
  // setup buttons for LyraT Board
  kit.addAction(PIN_KEY1, kit.actionVolumeDown);
  kit.addAction(PIN_KEY2, kit.actionVolumeUp);
  kit.addAction(BUTTON_PLAY_ID, btnChangeMode);
  kit.addAction(BUTTON_SET_ID, btnPrevious);
//kit.addAction(BUTTON_VOLDOWN_ID, ??); Not available(!)
  kit.addAction(BUTTON_VOLUP_ID, btnNext);
#else
  // setup buttons for ai_thinker board (ES8388) 
  kit.addAction(PIN_KEY1, btnChangeMode);
//kit.addAction(PIN_KEY2, ??);          Not available(!)
  kit.addAction(PIN_KEY3, btnPrevious);
  kit.addAction(PIN_KEY4, btnNext);
#endif

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
  byte nextLine=1;
  if(strlen(displayName)>0){
    dispText(1,displayName);
    nextLine = 2;
  }
  if(type == 0){ //Title
    dispText(nextLine,(char*)str);
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
  if(type == 3){ //Genre
    dispText(3,(char*)str);
    return;
  }
  if(type == 4){ //name
    dispText(1,(char*)str);
    snprintf(displayName, sizeof(displayName), "%s", str);
    return;    
  }
}

void bt_connection_state_changed(esp_a2d_connection_state_t state, void *ptr){
  if(state == ESP_A2D_CONNECTION_STATE_DISCONNECTED){
    dispText(1,"Not connected");
    dispText(2,":-(");
  }
}

void bt_play_state_changed(esp_a2d_audio_state_t state, void *ptr){
  player_active = (state == ESP_A2D_AUDIO_STATE_STARTED);
  dispCall("bt0.val",player_active);
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

void dispCall(char* obj, bool val){
  if(val == true){
    dispCall(obj,"1");
  } else {
    dispCall(obj,"0");
  }
}

void dispCall(char* obj, char* val){
  Serial.print(obj);
  Serial.print("=");
  Serial.print(val);
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
    player_stateCheck();
  } else {
    // feed watchdog
    delay(10);
  }
  kit.processActions();
}

//report changed value
void player_stateCheck(){
  if (!player){ return; } 
  if (player->isActive() != player_active){
      player_active = player->isActive();
      dispCall("bt0.val",player_active);  
  }
}
