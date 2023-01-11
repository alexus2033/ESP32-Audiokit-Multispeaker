void startBTSpeaker(){
  a2dp_sink.start(deviceName,false);
  dispText(0,"Bluetooth");
  dispText(1,"Connect me:");
  dispText(2,deviceName);
  auto cfg = kit.defaultConfig();
  cfg.sample_rate = a2dp_sink.sample_rate();
  debugln("update sample rate");
  kit.setAudioInfo(cfg);
  debugln("A2DP On");
  player_mode = ModeBTSpeaker;
}

void startRadioPlayer(){
  dispText(0,"Connecting Radio"); 
  player_mode = ModeWebRadio;
  char info[20];
  
  if (WiFi.status() != WL_CONNECTED && strlen(network)>0 && strlen(passwd)>0){  
      WiFi.mode(WIFI_STA);
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
      WiFi.setHostname(deviceName);
      WiFi.begin(network, passwd);
      //WiFi.setSleep(false);
      byte timeout = 30; 
      while (WiFi.status() != WL_CONNECTED && timeout>0){
          snprintf(info, sizeof(info), "Timeout: %d s", timeout);
          dispText(2,info);
          delay(1000);
          timeout--; 
      }
      debugln();
  }
  if (WiFi.status() != WL_CONNECTED){
    dispText(1,"Network not found");
    dispText(2,"");
    return;
  }
  IPAddress ip = WiFi.localIP();
  snprintf(info, sizeof(info), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  dispText(2,info);
  player = new AudioPlayer(sourceRadio, kit, decoder);
  player->setMetadataCallback(player_metadata_callback);
  decoder.begin();
  player->setVolume(0.8);
  player->begin();
  kit.setMute(false);
  debugln("Radio On");
  dispText(0,"Radio");
}

bool SDCard_Available(){
  int detectSDPin = SD_CARD_INTR_GPIO;
  if(detectSDPin>0 && digitalRead(detectSDPin) == 0){
    return true;
  }
  dispText(0,"Insert SD-Card");
  return false;
}

void startSDCardPlayer(){
  if(!SDCard_Available) return;

  player_mode = ModeSDPlayer;
  dispText(0,"SD-Card");
  player = new AudioPlayer(sourceSD, kit, decoder);
  player->setMetadataCallback(player_metadata_callback);
  decoder.begin();
  player->setVolume(0.8);
  player->begin(sd_index, true);
  debugln(sd_index);
  debugln("SD-Card On");
  SetName(); 
}

void parseInput(){
  debug("parseInput Core: ");
  debugln(xPortGetCoreID());
  if(input == "modSD"){
    player_mode_new = ModeSDPlayer;
  }  
  if(input == "modBT"){
    player_mode_new = ModeBTSpeaker;
  }
  if(input == "modRadio"){
    player_mode_new = ModeWebRadio;
  }
  if(input == "plyPause"){
    btnStopResume(false, 0, NULL);
  }
  if(input == "plyPrev"){
    btnPrevious(false, 0, NULL);
  }
  if(input == "plyNext"){
    btnNext(false, 0, NULL);
  }
}

void btnChangeMode(bool, int, void*) {
  //confim BT Pin Request
  if (player_mode == ModeBTSpeaker && a2dp_sink.pin_code() != 0) {
      a2dp_sink.confirm_pin_code();
      dispText(0,"Bluetooth");
      return;
  }
  //request new Mode
  player_mode_new = player_mode + 1;
  if(player_mode_new > ModeBTSpeaker){
      player_mode_new = ModeWebRadio; //jump back
  }  
  
}

void btnNext(bool, int, void*) {
  debugln("Player next");
  kit.setMute(true);
  if(player && player_mode != ModeBTSpeaker){
    player->next();
  } else {
    a2dp_sink.next();
  }
  SetName();
  kit.setMute(false);
}

void btnPrevious(bool, int, void*) {
  debugln("Player prev");
  kit.setMute(true);
  if(player && player_mode != ModeBTSpeaker){
    player->previous();
  } else {
    a2dp_sink.previous();
  }
  SetName();
  kit.setMute(false);
}

void btnStopResume(bool, int, void*){
  kit.setMute(true);
  if(player_mode == ModeBTSpeaker){
    if(a2dp_sink.get_audio_state()==ESP_A2D_AUDIO_STATE_STARTED){
       a2dp_sink.pause();
    } else {
       a2dp_sink.play();
    }
  }
  if (player && player->isActive()){ 
    player->stop();
  } else if (player){
      player->play();
  }
  kit.setMute(false);
}

void SetName(){
  resetDisplay();
  if(player_mode == ModeSDPlayer){
    sd_index = sourceSD.index();
    char* path = (char*)sourceSD.toStr();
    char delimiter[] = "/";
    char* ptr = strtok(path, delimiter);
    while(ptr != NULL)
    { //extract FileName from Path
      snprintf(displayName, sizeof(displayName), "%d: %s", sd_index+1, ptr);
      ptr = strtok(NULL, delimiter);
    }
    dispText(1,displayName);
  }
  if(player_mode == ModeBTSpeaker && a2dp_sink.is_connected()){
    snprintf(displayName, sizeof(displayName), "%s", a2dp_sink.get_connected_source_name());
  }
  if(player_mode == ModeWebRadio){
    char info[20];
    int radioIdx = sourceRadio.index();
    snprintf(info, sizeof(info), "Stream %d", radioIdx+1);
    dispText(1,info);
  }  
}

void resetDisplay(){
  memset(displayName, 0, sizeof(displayName)); 
  dispText(1,"");
  dispText(2,"");
}

void readWlanFile(){
  if(!SDCard_Available) return;
  
  char line[50];
  SdFat sd;
  SdFile file;
  byte timeout = 30;  
  while (!sd.begin(sdcfg) && timeout>0) {
    Serial.println("sd.begin failed");
    delay(1000);
    timeout--;
  }
  if (!file.open("wlan.sid", O_READ)){
    Serial.println("open failed");
    return;
  }
  if(file.fgets(line, sizeof(line))>0){
    snprintf(network,strlen(line), "%s",line);
  }
  if(file.fgets(line, sizeof(line))>0){
    snprintf(passwd,strlen(line), "%s",line);
  }
  file.close();
  dispCall("btn0.val",false);
  snprintf(line,strlen(line), "Network: %s",network);
  dispText(1,line);
  debug("Passwd: ");
  debugln(strlen(passwd));
}
