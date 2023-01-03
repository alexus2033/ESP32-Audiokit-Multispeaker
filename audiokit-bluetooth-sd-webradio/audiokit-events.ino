// switch between a2dp and sd player
void btnChangeMode(bool, int, void*) {
  if (a2dp_sink.pin_code() != 0) {
      a2dp_sink.confirm_pin_code();
      dispText(0,"Bluetooth");
      return;
  }

  if(player){ //stop and kill all sources
    player->stop();
    player->end();
  }
  inetStream.end();
  if (WiFi.status() == WL_CONNECTED){
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
  }
  a2dp_sink.stop();        
  a2dp_sink.end();
  decoder.end();
  
  player_mode ++;
  if(player_mode > ModeBTSpeaker){
      player_mode = ModeSDPlayer;
  }
  
  if (player_mode == ModeSDPlayer){     
      startSDCardPlayer();
  }
  if (player_mode == ModeWebRadio){
      startRadioPlayer();
  }
  if (player_mode == ModeBTSpeaker){
      startBTSpeaker();     
  }
}

void btnStopResume(bool, int, void*){
  if(player_mode == ModeBTSpeaker){
    if(a2dp_sink.get_audio_state()==ESP_A2D_AUDIO_STATE_STARTED){
       a2dp_sink.pause();
    } else {
       a2dp_sink.play();
    }
  }
  if (!player){ return; } 
  if (player->isActive()){
    player->stop();
  } else{
    player->play();
  } 
}

void startBTSpeaker(){
  dispText(0,"Bluetooth");
  a2dp_sink.start("Boombox-23",false);
  auto cfg = kit.defaultConfig();
  cfg.sample_rate = a2dp_sink.sample_rate();
  debugln("update sample rate");
  kit.setAudioInfo(cfg);
  debugln("A2DP On");
}

void startRadioPlayer(){
  dispText(0,"Connecting Radio"); 
  if (WiFi.status() != WL_CONNECTED && network!=nullptr && password != nullptr){
        WiFi.begin(network, password);
        //WiFi.setSleep(false);
        while (WiFi.status() != WL_CONNECTED){
            Serial.print(".");
            delay(500); 
        }
        Serial.println();
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  player = new AudioPlayer(sourceRadio, kit, decoder);
  player->setMetadataCallback(player_metadata_callback);
  decoder.begin();
  player->setVolume(0.8);
  player->begin();
  debugln("Radio On");
  dispText(0,"Radio");
  player_mode == ModeWebRadio;
}

void startSDCardPlayer(){
  dispText(0,"SD-Card");
  int detectSDPin = SD_CARD_INTR_GPIO;
  if (detectSDPin>0 && digitalRead(detectSDPin) == 1) {
    dispText(0,"Insert SD-Card");
    return;
  }
  player = new AudioPlayer(sourceSD, kit, decoder);
  player->setMetadataCallback(player_metadata_callback);
  decoder.begin();
  player->setVolume(0.8);
  player->begin();
  debugln("SD-Card On"); 
}

void btnNext(bool, int, void*) {
  debugln("Player next");
  if(player && player_mode != ModeBTSpeaker){
    player->next();
  } else {
    a2dp_sink.next();
  }
}

void btnPrevious(bool, int, void*) {
  debugln("Player prev");
  //if(player_mode == ModeSDPlayer && sourceSD.index() < 1){
  //  return; //bugfix
  //}
  if(player && player_mode != ModeBTSpeaker){
    player->previous();
  } else {
    a2dp_sink.previous();
  }
}

void resetDisplay(){
  dispText(1,"");
  dispText(2,"");
}
