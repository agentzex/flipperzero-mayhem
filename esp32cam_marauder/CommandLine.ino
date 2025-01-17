#include "CommandLine.h"

bool configESPCamera_initialized = false;

void configESPCamera() {
  if(configESPCamera_initialized)
    esp_camera_deinit();

  // Object to store the camera configuration parameters
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Choices are YUV422, GRAYSCALE, RGB565, JPEG

  // Select lower framesize if the camera doesn't support PSRAM
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10; //10-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  configESPCamera_initialized = true;

  // Camera quality adjustments
  sensor_t * s = esp_camera_sensor_get();

  // BRIGHTNESS (-2 to 2)
  s->set_brightness(s, 0);
  // CONTRAST (-2 to 2)
  s->set_contrast(s, 0);
  // SATURATION (-2 to 2)
  s->set_saturation(s, 0);
  // SPECIAL EFFECTS (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_special_effect(s, 0);
  // WHITE BALANCE (0 = Disable , 1 = Enable)
  s->set_whitebal(s, 1);
  // AWB GAIN (0 = Disable , 1 = Enable)
  s->set_awb_gain(s, 1);
  // WB MODES (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_wb_mode(s, 0);
  // EXPOSURE CONTROLS (0 = Disable , 1 = Enable)
  s->set_exposure_ctrl(s, 1);
  // AEC2 (0 = Disable , 1 = Enable)
  s->set_aec2(s, 0);
  // AE LEVELS (-2 to 2)
  s->set_ae_level(s, 0);
  // AEC VALUES (0 to 1200)
  s->set_aec_value(s, 300);
  // GAIN CONTROLS (0 = Disable , 1 = Enable)
  s->set_gain_ctrl(s, 1);
  // AGC GAIN (0 to 30)
  s->set_agc_gain(s, 0);
  // GAIN CEILING (0 to 6)
  s->set_gainceiling(s, (gainceiling_t)0);
  // BPC (0 = Disable , 1 = Enable)
  s->set_bpc(s, 0);
  // WPC (0 = Disable , 1 = Enable)
  s->set_wpc(s, 1);
  // RAW GMA (0 = Disable , 1 = Enable)
  s->set_raw_gma(s, 1);
  // LENC (0 = Disable , 1 = Enable)
  s->set_lenc(s, 1);
  // HORIZ MIRROR (0 = Disable , 1 = Enable)
  s->set_hmirror(s, 0);
  // VERT FLIP (0 = Disable , 1 = Enable)
  s->set_vflip(s, 0);
  // DCW (0 = Disable , 1 = Enable)
  s->set_dcw(s, 1);
  // COLOR BAR PATTERN (0 = Disable , 1 = Enable)
  s->set_colorbar(s, 0);
}

void initMicroSDCard() {
  Serial.println("Mounting MicroSD Card");
  if (!SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT)) {
    Serial.println("MicroSD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No MicroSD Card found");
    return;
  }
}

void takeNewPhoto(String path, bool flash) {
  // Take Picture with Camera

  // Setup frame buffer
  if (flash)
  {
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    delay(100);
  }

  esp_camera_fb_get();
  delay(300);

  camera_fb_t  * fb = esp_camera_fb_get();
  if (flash)
  {
    digitalWrite(4, LOW);
  }
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Save picture to microSD card
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in write mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  // Close the file
  file.close();

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
  fb = NULL;
}

CommandLine::CommandLine() {
}

void CommandLine::RunSetup() {
  //Serial.println(this->ascii_art);

  Serial.println(F("\n\n--------------------------------\n"));
  Serial.println(F("         ESP32 Marauder      \n"));
  Serial.println("            " + version_number + "\n");
  Serial.println(F("       By: justcallmekoko\n"));
  Serial.println(F("--------------------------------\n\n"));
  
  Serial.print("> ");
}

String CommandLine::getSerialInput() {
  String input = "";

  if (Serial.available() > 0)
    input = Serial.readStringUntil('\n');

  input.trim();
  return input;
}

void CommandLine::main(uint32_t currentTime) {
  String input = this->getSerialInput();

  this->runCommand(input);

  if (input != "")
    Serial.print("> ");
}

LinkedList<String> CommandLine::parseCommand(String input, char* delim) {
  LinkedList<String> cmd_args;

  bool inQuote = false;
  String buffer = "";

  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    // Do not break parameters that are enclosed in quotes
    if (c == '"') {
      inQuote = !inQuote;
    } else if (!inQuote && strchr(delim, c) != NULL) {
      cmd_args.add(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }
  cmd_args.add(buffer);

  return cmd_args;
}

int CommandLine::argSearch(LinkedList<String>* cmd_args_list, String key) {
  for (int i = 0; i < cmd_args_list->size(); i++) {
    if (cmd_args_list->get(i) == key)
      return i;
  }

  return -1;
}

bool CommandLine::checkValueExists(LinkedList<String>* cmd_args_list, int index) {
  if (index < cmd_args_list->size() - 1)
    return true;
    
  return false;
}

bool CommandLine::inRange(int max, int index) {
  if ((index >= 0) && (index < max))
    return true;

  return false;
}

bool CommandLine::apSelected() {
  for (int i = 0; i < access_points->size(); i++) {
    if (access_points->get(i).selected)
      return true;
  }

  return false;
}

bool CommandLine::hasSSIDs() {
  if (ssids->size() == 0)
    return false;

  return true;
}

void CommandLine::runCommand(String input) {
  if (input != "")
    Serial.println("#" + input);
  else
    return;

  LinkedList<String> cmd_args = this->parseCommand(input, " ");

  //// Admin commands

  // Help
  if (cmd_args.get(0) == HELP_CMD) {
    Serial.println(HELP_HEAD);
    Serial.println(HELP_CH_CMD);
    Serial.println(HELP_SETTINGS_CMD);
    Serial.println(HELP_CLEARAP_CMD_A);
    Serial.println(HELP_REBOOT_CMD);
    Serial.println(HELP_UPDATE_CMD_A);
    
    // WiFi sniff/scan
    Serial.println(HELP_SCANAP_CMD);
    Serial.println(HELP_SCANSTA_CMD);
    Serial.println(HELP_SNIFF_RAW_CMD);
    Serial.println(HELP_SNIFF_BEACON_CMD);
    Serial.println(HELP_SNIFF_PROBE_CMD);
    Serial.println(HELP_SNIFF_PWN_CMD);
    Serial.println(HELP_SNIFF_ESP_CMD);
    Serial.println(HELP_SNIFF_DEAUTH_CMD);
    Serial.println(HELP_SNIFF_PMKID_CMD);
    Serial.println(HELP_STOPSCAN_CMD);
    
    // WiFi attack
    Serial.println(HELP_ATTACK_CMD);
    
    // WiFi Aux
    Serial.println(HELP_LIST_AP_CMD_A);
    Serial.println(HELP_LIST_AP_CMD_B);
    Serial.println(HELP_LIST_AP_CMD_C);
    Serial.println(HELP_SEL_CMD_A);
    Serial.println(HELP_SSID_CMD_A);
    Serial.println(HELP_SSID_CMD_B);
    
    // Bluetooth sniff/scan
    Serial.println(HELP_BT_SNIFF_CMD);
    Serial.println(HELP_BT_SKIM_CMD);
    Serial.println(HELP_FOOT);
    return;
  }

  // Stop Scan
  if (cmd_args.get(0) == STOPSCAN_CMD) {
    if (wifi_scan_obj.currentScanMode == OTA_UPDATE) {
      wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
      //#ifdef HAS_SCREEN
      //  menu_function_obj.changeMenu(menu_function_obj.updateMenu.parentMenu);
      //#endif
      WiFi.softAPdisconnect(true);
      web_obj.shutdownServer();
      return;
    }
    
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);

    Serial.println("Stopping WiFi tran/recv");

    // If we don't do this, the text and button coordinates will be off
    #ifdef HAS_SCREEN
      display_obj.tft.init();
      menu_function_obj.changeMenu(menu_function_obj.current_menu);
    #endif
  }
  // Channel command
  else if (cmd_args.get(0) == CH_CMD) {
    // Search for channel set arg
    int ch_set = this->argSearch(&cmd_args, "-s");
    
    if (cmd_args.size() == 1) {
      Serial.println("Current channel: " + (String)wifi_scan_obj.set_channel);
    }
    else if (ch_set != -1) {
      wifi_scan_obj.set_channel = cmd_args.get(ch_set + 1).toInt();
      wifi_scan_obj.changeChannel();
      Serial.println("Set channel: " + (String)wifi_scan_obj.set_channel);
    }
  }
  else if (cmd_args.get(0) == CAM_FLASHLIGHT) {
    pinMode(4, OUTPUT);
    digitalWrite(4, !digitalRead(4));
  }

  else if (cmd_args.get(0) == CAM_PHOTO) {
    Serial.println("Camera capture start");
    // Initialize the camera
    configESPCamera(); Serial.println("Camera OK!");

    // Initialize the MicroSD
    Serial.print("Initializing the MicroSD card module... ");
    //initMicroSDCard();

    int i = 0;
    while (true)
    {
      String path = "/photo_" + String(i++) + ".jpg";
      if (!SD_MMC.exists(path.c_str()))
      {
        takeNewPhoto(path, true);
        break;
      }
    }
    Serial.println("Camera capture finish");
  }
  
  // Clear APs
  else if (cmd_args.get(0) == CLEARAP_CMD) {
    int ap_sw = this->argSearch(&cmd_args, "-a"); // APs
    int ss_sw = this->argSearch(&cmd_args, "-s"); // SSIDs
    int cl_sw = this->argSearch(&cmd_args, "-c"); // Stations

    if (ap_sw != -1) {
      #ifdef HAS_SCREEN
        menu_function_obj.changeMenu(&menu_function_obj.clearAPsMenu);
      #endif
      wifi_scan_obj.RunClearAPs();
    }

    if (ss_sw != -1) {
      #ifdef HAS_SCREEN
        menu_function_obj.changeMenu(&menu_function_obj.clearSSIDsMenu);
      #endif
      wifi_scan_obj.RunClearSSIDs();
    }

    if (cl_sw != -1) {
      #ifdef HAS_SCREEN
        menu_function_obj.changeMenu(&menu_function_obj.clearAPsMenu);
      #endif
      wifi_scan_obj.RunClearStations();
    }
  }

  else if (cmd_args.get(0) == SETTINGS_CMD) {
    int ss_sw = this->argSearch(&cmd_args, "-s"); // Set setting
    int re_sw = this->argSearch(&cmd_args, "-r"); // Reset setting
    int en_sw = this->argSearch(&cmd_args, "enable"); // enable setting
    int da_sw = this->argSearch(&cmd_args, "disable"); // disable setting

    if (re_sw != -1) {
      settings_obj.createDefaultSettings(SPIFFS);
      return;
    }

    if (ss_sw == -1) {
      settings_obj.printJsonSettings(settings_obj.getSettingsString());
    }
    else {
      bool result = false;
      String setting_name = cmd_args.get(ss_sw + 1);
      if (en_sw != -1)
        result = settings_obj.saveSetting<bool>(setting_name, true);
      else if (da_sw != -1)
        result = settings_obj.saveSetting<bool>(setting_name, false);
      else {
        Serial.println("You did not properly enable/disable this setting.");
        return;
      }

      if (!result) {
        Serial.println("Could not successfully update setting \"" + setting_name + "\"");
        return;
      }
    }
  }

  else if (cmd_args.get(0) == REBOOT_CMD) {
    Serial.println("Rebooting...");
    ESP.restart();
  }

  //// WiFi/Bluetooth Scan/Attack commands
  if (!wifi_scan_obj.scanning()) {

    // AP Scan
    if (cmd_args.get(0) == SCANAP_CMD) {
      int full_sw = -1;
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif

      if (full_sw == -1) {
        Serial.println("Starting AP scan. Stop with " + (String)STOPSCAN_CMD);
        wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP, TFT_MAGENTA);
      }
      else {
        Serial.println("Starting Full AP scan. Stop with " + (String)STOPSCAN_CMD);
        wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP_FULL, TFT_MAGENTA);
      }
    }
    // Raw sniff
    else if (cmd_args.get(0) == SNIFF_RAW_CMD) {
      Serial.println("Starting Raw sniff. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_RAW_CAPTURE, TFT_WHITE);
    }
    // Scan stations
    else if (cmd_args.get(0) == SCANSTA_CMD) {    
      Serial.println("Starting Station scan. Stop with " + (String)STOPSCAN_CMD);  
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_STATION, TFT_ORANGE);
    }
    // Beacon sniff
    else if (cmd_args.get(0) == SNIFF_BEACON_CMD) {
      Serial.println("Starting Beacon sniff. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_AP, TFT_MAGENTA);
    }
    // Probe sniff
    else if (cmd_args.get(0) == SNIFF_PROBE_CMD) {
      Serial.println("Starting Probe sniff. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_PROBE, TFT_MAGENTA);
    }
    // Deauth sniff
    else if (cmd_args.get(0) == SNIFF_DEAUTH_CMD) {
      Serial.println("Starting Deauth sniff. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_DEAUTH, TFT_RED);
    }
    // Pwn sniff
    else if (cmd_args.get(0) == SNIFF_PWN_CMD) {
      Serial.println("Starting Pwnagotchi sniff. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_PWN, TFT_MAGENTA);
    }
    // Espressif sniff
    else if (cmd_args.get(0) == SNIFF_ESP_CMD) {
      Serial.println("Starting Espressif device sniff. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(WIFI_SCAN_ESPRESSIF, TFT_MAGENTA);
    }
    // PMKID sniff
    else if (cmd_args.get(0) == SNIFF_PMKID_CMD) {
      int ch_sw = this->argSearch(&cmd_args, "-c");
      int d_sw = this->argSearch(&cmd_args, "-d"); // Deauth for pmkid
      
      if (ch_sw != -1) {
        wifi_scan_obj.set_channel = cmd_args.get(ch_sw + 1).toInt();
        wifi_scan_obj.changeChannel();
        Serial.println("Set channel: " + (String)wifi_scan_obj.set_channel);
        
      }

      if (d_sw == -1) {
        Serial.println("Starting PMKID sniff on channel " + (String)wifi_scan_obj.set_channel + ". Stop with " + (String)STOPSCAN_CMD);
        wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL, TFT_VIOLET);
      }
      else {
        Serial.println("Starting PMKID sniff with deauthentication on channel " + (String)wifi_scan_obj.set_channel + ". Stop with " + (String)STOPSCAN_CMD);
        wifi_scan_obj.StartScan(WIFI_SCAN_ACTIVE_EAPOL, TFT_VIOLET);
      }
    }

    //// WiFi attack commands
    // attack
    if (cmd_args.get(0) == ATTACK_CMD) {
      int attack_type_switch = this->argSearch(&cmd_args, "-t"); // Required
      int list_beacon_sw = this->argSearch(&cmd_args, "-l");
      int rand_beacon_sw = this->argSearch(&cmd_args, "-r");
      int ap_beacon_sw = this->argSearch(&cmd_args, "-a");
      int src_addr_sw = this->argSearch(&cmd_args, "-s");
      int dst_addr_sw = this->argSearch(&cmd_args, "-d");
      int targ_sw = this->argSearch(&cmd_args, "-c");
  
      if (attack_type_switch == -1) {
        Serial.println("You must specify an attack type");
        return;
      }
      else {
        String attack_type = cmd_args.get(attack_type_switch + 1);
  
        // Branch on attack type
        // Deauth
        if (attack_type == ATTACK_TYPE_DEAUTH) {
          // Default to broadcast
          if ((dst_addr_sw == -1) && (targ_sw == -1)) {
            Serial.println("Sending to broadcast...");
            wifi_scan_obj.dst_mac = "ff:ff:ff:ff:ff:ff";
          }
          // Dest addr specified
          else if (dst_addr_sw != -1) {
            wifi_scan_obj.dst_mac = cmd_args.get(dst_addr_sw + 1);
            Serial.println("Sending to " + wifi_scan_obj.dst_mac + "...");
          }
          // Station list specified
          else if (targ_sw != -1)
            Serial.println("Sending to Station list");

          // Source addr not specified
          if (src_addr_sw == -1) {
            if (!this->apSelected()) {
              Serial.println("You don't have any targets selected. Use " + (String)SEL_CMD);
              return;
            }
            #ifdef HAS_SCREEN
              display_obj.clearScreen();
              menu_function_obj.drawStatusBar();
            #endif
            Serial.println("Starting Deauthentication attack. Stop with " + (String)STOPSCAN_CMD);
            // Station list not specified
            if (targ_sw == -1)
              wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH, TFT_RED);
            // Station list specified
            else
              wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH_TARGETED, TFT_ORANGE);
          }
          // Source addr specified
          else {
            String src_mac_str = cmd_args.get(src_addr_sw + 1);
            sscanf(src_mac_str.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
              &wifi_scan_obj.src_mac[0], &wifi_scan_obj.src_mac[1], &wifi_scan_obj.src_mac[2], &wifi_scan_obj.src_mac[3], &wifi_scan_obj.src_mac[4], &wifi_scan_obj.src_mac[5]);

            #ifdef HAS_SCREEN
              display_obj.clearScreen();
              menu_function_obj.drawStatusBar();
            #endif
            Serial.println("Starting Manual Deauthentication attack. Stop with " + (String)STOPSCAN_CMD);
            wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH_MANUAL, TFT_RED);            
          }
        }
        // Beacon
        else if (attack_type == ATTACK_TYPE_BEACON) {
          // spam by list
          if (list_beacon_sw != -1) {
            if (!this->hasSSIDs()) {
              Serial.println("You don't have any SSIDs in your list. Use " + (String)SSID_CMD);
              return;
            }
            #ifdef HAS_SCREEN
              display_obj.clearScreen();
              menu_function_obj.drawStatusBar();
            #endif
            Serial.println("Starting Beacon list spam. Stop with " + (String)STOPSCAN_CMD);
            wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_LIST, TFT_RED);
          }
          // spam with random
          else if (rand_beacon_sw != -1) {
            #ifdef HAS_SCREEN
              display_obj.clearScreen();
              menu_function_obj.drawStatusBar();
            #endif
            Serial.println("Starting random Beacon spam. Stop with " + (String)STOPSCAN_CMD);
            wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_SPAM, TFT_ORANGE);
          }
          // Spam from AP list
          else if (ap_beacon_sw != -1) {
            if (!this->apSelected()) {
              Serial.println("You don't have any targets selected. Use " + (String)SEL_CMD);
              return;
            }
            #ifdef HAS_SCREEN
              display_obj.clearScreen();
              menu_function_obj.drawStatusBar();
            #endif
            Serial.println("Starting Targeted AP Beacon spam. Stop with " + (String)STOPSCAN_CMD);
            wifi_scan_obj.StartScan(WIFI_ATTACK_AP_SPAM, TFT_MAGENTA);
          }
          else {
            Serial.println("You did not specify a beacon attack type");
          }
        }
        else if (attack_type == ATTACK_TYPE_PROBE) {
          if (!this->apSelected()) {
            Serial.println("You don't have any targets selected. Use " + (String)SEL_CMD);
            return;
          }
          Serial.println("Starting Probe spam. Stop with " + (String)STOPSCAN_CMD);
          #ifdef HAS_SCREEN
            display_obj.clearScreen();
            menu_function_obj.drawStatusBar();
          #endif
          wifi_scan_obj.StartScan(WIFI_ATTACK_AUTH, TFT_RED);
        }
        else if (attack_type == ATTACK_TYPE_RR) {
          Serial.println("Starting Rick Roll Beacon spam. Stop with " + (String)STOPSCAN_CMD);
          #ifdef HAS_SCREEN
            display_obj.clearScreen();
            menu_function_obj.drawStatusBar();
          #endif
          wifi_scan_obj.StartScan(WIFI_ATTACK_RICK_ROLL, TFT_YELLOW);
        }
        else {
          Serial.println("Attack type not properly defined");
          return;
        }
      }
    }

    //// Bluetooth scan/attack commands
    // Bluetooth scan
    if (cmd_args.get(0) == BT_SNIFF_CMD) {
      Serial.println("Starting Bluetooth scan. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(BT_SCAN_ALL, TFT_GREEN);
    }
    // Bluetooth CC Skimmer scan
    else if (cmd_args.get(0) == BT_SKIM_CMD) {
      Serial.println("Starting Bluetooth CC Skimmer scan. Stop with " + (String)STOPSCAN_CMD);
      #ifdef HAS_SCREEN
        display_obj.clearScreen();
        menu_function_obj.drawStatusBar();
      #endif
      wifi_scan_obj.StartScan(BT_SCAN_SKIMMERS, TFT_MAGENTA);
    }

    // Update command
    if (cmd_args.get(0) == UPDATE_CMD) {
      int w_sw = this->argSearch(&cmd_args, "-w"); // Web update
      int sd_sw = this->argSearch(&cmd_args, "-s"); // SD Update

      // Update via OTA
      if (w_sw != -1) {
        Serial.println("Starting Marauder OTA Update. Stop with " + (String)STOPSCAN_CMD);
        wifi_scan_obj.currentScanMode = OTA_UPDATE;
        //#ifdef HAS_SCREEN
        //  menu_function_obj.changeMenu(menu_function_obj.updateMenu);
        //#endif
        web_obj.setupOTAupdate();
      }
      // Update via SD
      else if (sd_sw != -1) {
        #ifndef WRITE_PACKETS_SERIAL
          if (!sd_obj.supported) {
            Serial.println("SD card is not connected. Cannot perform SD Update");
            return;
          }
          wifi_scan_obj.currentScanMode = OTA_UPDATE;
          sd_obj.runUpdate();
        #else
          Serial.println("SD card not initialized. Cannot perform SD Update");
        #endif
      }
    }
  }


  //// WiFi aux commands
  // List access points
  if (cmd_args.get(0) == LIST_AP_CMD) {
    int ap_sw = this->argSearch(&cmd_args, "-a");
    int ss_sw = this->argSearch(&cmd_args, "-s");
    int cl_sw = this->argSearch(&cmd_args, "-c");

    // List APs
    if (ap_sw != -1) {
      for (int i = 0; i < access_points->size(); i++) {
        if (access_points->get(i).selected)
          Serial.println("[" + (String)i + "] " + access_points->get(i).essid + " " + (String)access_points->get(i).rssi + " (selected)");
        else
          Serial.println("[" + (String)i + "] " + access_points->get(i).essid + " " + (String)access_points->get(i).rssi);
      }
    }
    // List SSIDs
    else if (ss_sw != -1) {
      for (int i = 0; i < ssids->size(); i++) {
        if (ssids->get(i).selected)
          Serial.println("[" + (String)i + "] " + ssids->get(i).essid + " (selected)");
        else
          Serial.println("[" + (String)i + "] " + ssids->get(i).essid);
      }
    }
    // List Stations
    else if (cl_sw != -1) {
      char sta_mac[] = "00:00:00:00:00:00";
      for (int x = 0; x < access_points->size(); x++) {
        Serial.println("[" + (String)x + "] " + access_points->get(x).essid + " " + (String)access_points->get(x).rssi + ":");
        for (int i = 0; i < access_points->get(x).stations->size(); i++) {
          wifi_scan_obj.getMAC(sta_mac, stations->get(access_points->get(x).stations->get(i)).mac, 0);
          if (stations->get(access_points->get(x).stations->get(i)).selected) {
            Serial.print("  [" + (String)access_points->get(x).stations->get(i) + "] ");
            Serial.print(sta_mac);
            Serial.println(" (selected)");
          }
          else {
            Serial.print("  [" + (String)access_points->get(x).stations->get(i) + "] ");
            Serial.println(sta_mac);
          }
        }
      }
    }
    else {
      Serial.println("You did not specify which list to show");
      return;
    }
  }
  // Select access points or stations
  else if (cmd_args.get(0) == SEL_CMD) {
    // Get switches
    int ap_sw = this->argSearch(&cmd_args, "-a");
    int ss_sw = this->argSearch(&cmd_args, "-s");
    int cl_sw = this->argSearch(&cmd_args, "-c");

    // select Access points
    if (ap_sw != -1) {
      // Get list of indices
      LinkedList<String> ap_index = this->parseCommand(cmd_args.get(ap_sw + 1), ",");

      // Select ALL APs
      if (cmd_args.get(ap_sw + 1) == "all") {
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).selected) {
            // Unselect "selected" ap
            AccessPoint new_ap = access_points->get(i);
            new_ap.selected = false;
            access_points->set(i, new_ap);
          }
          else {
            // Select "unselected" ap
            AccessPoint new_ap = access_points->get(i);
            new_ap.selected = true;
            access_points->set(i, new_ap);
          }
        }
      }
      // Select specific APs
      else {
        // Mark APs as selected
        for (int i = 0; i < ap_index.size(); i++) {
          int index = ap_index.get(i).toInt();
          if (!this->inRange(access_points->size(), index)) {
            Serial.println("Index not in range: " + (String)index);
            continue;
          }
          if (access_points->get(index).selected) {
            // Unselect "selected" ap
            AccessPoint new_ap = access_points->get(index);
            new_ap.selected = false;
            access_points->set(index, new_ap);
          }
          else {
            // Select "unselected" ap
            AccessPoint new_ap = access_points->get(index);
            new_ap.selected = true;
            access_points->set(index, new_ap);
          }
        }
      }
    }
    else if (cl_sw != -1) {
      LinkedList<String> sta_index = this->parseCommand(cmd_args.get(cl_sw + 1), ",");
      
      // Select all Stations
      if (cmd_args.get(cl_sw + 1) == "all") {
        for (int i = 0; i < stations->size(); i++) {
          if (stations->get(i).selected) {
            // Unselect "selected" ap
            Station new_sta = stations->get(i);
            new_sta.selected = false;
            stations->set(i, new_sta);
          }
          else {
            // Select "unselected" ap
            Station new_sta = stations->get(i);
            new_sta.selected = true;
            stations->set(i, new_sta);
          }
        }
      }
      // Select specific Stations
      else {
        // Mark Stations as selected
        for (int i = 0; i < sta_index.size(); i++) {
          int index = sta_index.get(i).toInt();
          if (!this->inRange(stations->size(), index)) {
            Serial.println("Index not in range: " + (String)index);
            continue;
          }
          if (stations->get(index).selected) {
            // Unselect "selected" ap
            Station new_sta = stations->get(index);
            new_sta.selected = false;
            stations->set(index, new_sta);
          }
          else {
            // Select "unselected" ap
            Station new_sta = stations->get(index);
            new_sta.selected = true;
            stations->set(index, new_sta);
          }
        }
      }
    }
    // select ssids
    else if (ss_sw != -1) {
      // Get list of indices
      LinkedList<String> ss_index = this->parseCommand(cmd_args.get(ss_sw + 1), ",");

      // Mark APs as selected
      for (int i = 0; i < ss_index.size(); i++) {
        int index = ss_index.get(i).toInt();
        if (!this->inRange(ssids->size(), index)) {
          Serial.println("Index not in range: " + (String)index);
          continue;
        }
        if (ssids->get(index).selected) {
          // Unselect "selected" ap
          ssid new_ssid = ssids->get(index);
          new_ssid.selected = false;
          ssids->set(index, new_ssid);
        }
        else {
          // Select "unselected" ap
          ssid new_ssid = ssids->get(index);
          new_ssid.selected = true;
          ssids->set(index, new_ssid);
        }
      }
    }
    else {
      Serial.println("You did not specify which list to select from");
      return;
    }
  }
  // SSID stuff
  else if (cmd_args.get(0) == SSID_CMD) {
    int add_sw = this->argSearch(&cmd_args, "-a");
    int gen_sw = this->argSearch(&cmd_args, "-g");
    int spc_sw = this->argSearch(&cmd_args, "-n");
    int rem_sw = this->argSearch(&cmd_args, "-r");

    // Add ssid
    if (add_sw != -1) {
      // Generate random
      if (gen_sw != -1) {
        int gen_count = cmd_args.get(gen_sw + 1).toInt();
        wifi_scan_obj.generateSSIDs(gen_count);
      }
      // Add specific
      else if (spc_sw != -1) {
        String essid = cmd_args.get(spc_sw + 1);
        wifi_scan_obj.addSSID(essid);
      }
      else {
        Serial.println("You did not specify how to add SSIDs");
      }
    }
    // Remove SSID
    else if (rem_sw != -1) {
      int index = cmd_args.get(rem_sw + 1).toInt();
      if (!this->inRange(ssids->size(), index)) {
        Serial.println("Index not in range: " + (String)index);
        return;
      }
      ssids->remove(index);
    }
    else {
      Serial.println("You did not specify whether to add or remove SSIDs");
      return;
    }
  }
}
