//https://www.rapidtables.com/web/color/RGB_Color.html
// IMPORTANT:  Partition MUST be set to "No OTA (Large APP)" or there will not be enough program space.

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include "ajaone.h"


//  Create the various "clients"
WiFiClient espClient;
PubSubClient client(espClient);
BleKeyboard bleKeyboard;  
TinyPICO tp = TinyPICO();

//class bleKeyboard;

IRrecv IrReceiver(IR_RECEIVE_PIN);
IRsend IrSender; //default GPIO4


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
//==================================================================================
//==================================================================================
//==================================================================================


//===== Setup ======================================================================
//===== Setup ======================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  client.setBufferSize(bufferSize);

  pinMode(25,INPUT_PULLUP);
  pinMode(26,INPUT_PULLUP);

  tp.DotStar_SetBrightness(16);
  tp.DotStar_SetPixelColor( dot_booting );  
  

  // Get the last 2 bytes of the MAC address and set our MQTT name to AJAOne-<last 4 of MAC>
  WiFi.macAddress(mac);
  sprintf(boardname,"%s%02X%02X",BOARDNAME,mac[4],mac[5]);
  Serial.print("Hello, my name is ");
  Serial.println(boardname);    

  IrReceiver.enableIRIn();  // Start the receiver   
  
//===== WiFI Manager items =====
  loadconfig();  //read the config.json from SPIFFS
  
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 15);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 15);  
  WiFiManagerParameter custom_boardname("boardname", "boardname", boardname, 20);    

  tp.DotStar_SetPixelColor( dot_wifi );
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);  
  wifiManager.addParameter(&custom_boardname); 
  wifiManager.setTimeout(120);
  
  if (!wifiManager.autoConnect(WIFIPORTAL, "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }  
  
  Serial.println("connected...");
  
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());  
  strcpy(boardname, custom_boardname.getValue());  
  
  if (shouldSaveConfig) {
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(boardname, custom_boardname.getValue());    
    saveconfig();  //save back to config.json 
  }  
//===== WiFI Manager items =====  

  Serial.println("Starting BT");
  bleKeyboard.begin();  
 
  // Setup the MQTT client
  Serial.println("Starting MQTT");
  client.setServer(mqtt_server, strtol (mqtt_port,NULL,10 ));
  client.setCallback(callback);

   // Write the Wi-Fi details to the OLED
   Serial.println(WiFi.localIP() );
 
   busy = false;

  tp.DotStar_SetPixelColor( dot_goodboot  );
  delay(1000);
  tp.DotStar_Clear();

  Serial.print("Hello, my name is ");
  Serial.println(boardname);   

}
//===== Setup =====================================================================^
//===== Setup =====================================================================^


//===== Loop =======================================================================
//===== Loop =======================================================================
void loop() {
 
  // Reconnect MQTT if needed wil reboot after 2.5 minutes
  if (!client.connected()) {
     if (mqtt_server != "") {
      reconnect();
     }
  }  
  
   //  MQTT task
   client.loop();  

   long now = millis();

   //===== key press loop ====
    byte key = digitalRead(0);
    if (key == LOW) { // that means the key is pressed, reset after 10 seconds.  Don't really like this but it works....
       pressTime++;
       delay(1000); // works like a timer with one second steps
       if (pressTime <= 10){
       }else {
         //Reset everything and restart 
         hardreset();
         }    
        }
        else {  
        //reset PressTime if 2 seconds elapsed with no key press  
        if (now - lastkeyMsg > 2000) {
           pressTime = 0;
           lastkeyMsg = now;
         }    
        }  
   //===== key press loop ====

    if (IrReceiver.decode()) {  // Grab an IR code
         dumpInfo();             // Output the results
        //Serial.println();
        IrReceiver.resume();    // Prepare for the next value
    }   

}
//===== Loop ======================================================================^
//===== Loop ======================================================================^


//===== Dump the decoded info =====================================================
void dumpInfo() {
  char buffer[bufferSize];

  Serial.println("DumpInfo:");
  
    // Check if the buffer overflowed  
    if (IrReceiver.results.overflow) {
        Serial.println("IR code too long. Edit IRremoteInt.h and increase RAW_BUFFER_LENGTH");
        sendMQTTMessage("ERROR","IR code too long");
        return;
    }

    if (IrReceiver.results.bits == 0) {
      Serial.println("No bits found");
      return;
    }
    
    DynamicJsonDocument json(bufferSize);    
        
    json["protocol"] = IrReceiver.getProtocolString();
    sprintf(buffer,"0x%X",IrReceiver.results.value);
    json["data"] = buffer;
    sprintf(buffer,"%d",IrReceiver.results.bits);
    json["bits"] = buffer;

    char outgoingJsonBuffer[bufferSize];
    serializeJson(json, outgoingJsonBuffer);   
    sendMQTTMessage("irdecoder",outgoingJsonBuffer); 

    json.clear();

    String szpronto;
    IrReceiver.dumpPronto(&szpronto);
    const char* bufferp = szpronto.c_str();

    json["protocol"] = "PRONTO";
    json["data"] = bufferp;
    json["bits"] = "0";
    serializeJson(json, outgoingJsonBuffer);    
    sendMQTTMessage("irpronto",outgoingJsonBuffer);   
}
//===== Dump the decoded info ====================================================^


//===== MQTT Callback function =====================================================
void callback(char* topic, byte* message, unsigned int length) {
  
  tp.DotStar_SetPixelColor( dot_mqtttraffic );
  char buff[15];
      
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {    
    messageTemp += (char)message[i];
  }

  Serial.println(messageTemp);
  
  if (String(topic) == String(boardname)+"/cmd/status") {
    utilityloop();
  }

  if (String(topic) == String(boardname)+"/cmd/reboot") {
     ESP.restart();
  }  


//===== Bluetooth code to process ========================================
  if (String(topic) == String(boardname)+"/cmd/sendbtcode") {
   Serial.println("Processing Bluetooth command"); 
   DynamicJsonDocument json(bufferSize);         
   deserializeJson(json, messageTemp);

   const char* buff1 = json["key"];
   if (buff1) {
     uint8_t key = getkeys(buff1);
     Serial.println(buff1);
     bleKeyboard.write(key);
   }

   const char* buff2 = json["media"];
   if (buff2) {
     MediaKeyReport mymedia;
     if (buff2 == "KEY_MEDIA_NEXT_TRACK")     { memcpy(mymedia, KEY_MEDIA_NEXT_TRACK,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_PREVIOUS_TRACK") { memcpy(mymedia, KEY_MEDIA_PREVIOUS_TRACK,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_STOP")           { memcpy(mymedia, KEY_MEDIA_STOP,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_PLAY_PAUSE")     { memcpy(mymedia, KEY_MEDIA_PLAY_PAUSE,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_MUTE")           { memcpy(mymedia, KEY_MEDIA_MUTE,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_VOLUME_UP")      { memcpy(mymedia, KEY_MEDIA_VOLUME_UP,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_VOLUME_DOWN")    { memcpy(mymedia, KEY_MEDIA_VOLUME_DOWN,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_WWW_HOME")       { memcpy(mymedia, KEY_MEDIA_WWW_HOME,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_LOCAL_MACHINE_BROWSER") { memcpy(mymedia, KEY_MEDIA_LOCAL_MACHINE_BROWSER,sizeof(mymedia));  }

     if (buff2 == "KEY_MEDIA_CALCULATOR")     { memcpy(mymedia, KEY_MEDIA_CALCULATOR,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_WWW_BOOKMARKS")  { memcpy(mymedia, KEY_MEDIA_WWW_BOOKMARKS,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_WWW_SEARCH")     { memcpy(mymedia, KEY_MEDIA_WWW_SEARCH,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_WWW_STOP")       { memcpy(mymedia, KEY_MEDIA_WWW_STOP,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_WWW_BACK")       { memcpy(mymedia, KEY_MEDIA_WWW_BACK,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION") { memcpy(mymedia, KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION,sizeof(mymedia));  }
     if (buff2 == "KEY_MEDIA_EMAIL_READER")   { memcpy(mymedia, KEY_MEDIA_EMAIL_READER,sizeof(mymedia));  }     
     Serial.println(buff2);
     bleKeyboard.write(mymedia);
   }   

   const char* buff3 = json["print"];
   if (buff3) {
     Serial.println(buff3);
     bleKeyboard.print(buff3);
   }     
   
 } 
//===== Bluetooth code to process =======================================^  


  if (String(topic) == String(boardname)+"/cmd/sendircode") {
   
   Serial.println("Processing infrared command");
   DynamicJsonDocument json(bufferSize);         
   deserializeJson(json, messageTemp);
   unsigned int  data;
   data = strtol(json["data"],0,16);

   const char* myprotocol = json["protocol"];
   Serial.print("Sending ");
   Serial.print(myprotocol);
   Serial.print(" ");
   Serial.println(data);
    
   if (json["protocol"] == "NEC") {
    IrSender.sendNEC(data, json["bits"]);
   } //NEC
   else

   if (json["protocol"] == "PANASONIC") {
    IrSender.sendPanasonic(data, json["bits"]);
   } 
   else   

   if (json["protocol"] == "JVC") {
    IrSender.sendJVC(data, json["bits"]);
   } 
   else 

   if (json["protocol"] == "SAMSUNG") {
    IrSender.sendSAMSUNG(data, json["bits"]);
   } 
   else    

   if (json["protocol"] == "DENON") {
    IrSender.sendDenon(data, json["bits"]);
   } 
   else      

   if (json["protocol"] == "DISH") {
    IrSender.sendDISH(data, json["bits"]);
   } 
   else      
   
   if (json["protocol"] == "UNKNOWN") {  
    IrSender.sendNEC(data, json["bits"]);
   }  //UNKNOWN 
   else

   if (json["protocol"] == "SONY") {  
    IrSender.sendSony(data, json["bits"]);
   }  //Sony    
   else

   if (json["protocol"] == "PRONTO") {  
    const char* mydata = json["data"];
    IrSender.sendPronto(mydata,1);
   }  //PRONTO    

   
   
  }  //sendircode

  tp.DotStar_Clear(); 
  Serial.println("Done");    
}
//===== Callback function ==========================================================^


//===== Subscribe to a topic =======================================================
void doSubscribe(const char* topic, const char* item) {
   char tempc[60] = "";

   sprintf(tempc, "%s/%s",topic,item);  
   client.subscribe(tempc);   
}
//===== Subscribe to a topic =======================================================


//===== Send an MQTT message =======================================================
void sendMQTTMessage(const char* minortopic, const char* payload ) {
 char fulltopic[60] = "";  
 tp.DotStar_SetPixelColor( dot_mqtttraffic );    
 
 //===== Send the message
 sprintf(fulltopic, "%s/%s",boardname,minortopic);  
 client.publish(fulltopic, payload  );

 //debug
 Serial.print(fulltopic);
 Serial.print(" : ");
 Serial.println(payload);

 tp.DotStar_Clear();  
}
//===== Send an MQTT message ======================================================^


//===== Peform hard reset ===========================================================
void hardreset() {
 tp.DotStar_SetPixelColor( dot_reset );
 Serial.println("RESETTING!!");
 esp_wifi_restore();
 delay(2000); //wait a bit          
 SPIFFS.format();
 tp.DotStar_Clear();
 ESP.restart();  
}
//===== Peform hard reset ===========================================================


//===== Reconnect to the MQTT server ================================================
void reconnect() {
  // Loop until we're reconnected
  tp.DotStar_SetPixelColor( dot_mqttconnect );
  int retries = 0;
  while (!client.connected()) {
   if (retries < 15) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect(boardname,mqtt_user,mqtt_password)) {
      Serial.println("connected!");
      utilityloop();
      
      doSubscribe(boardname,"cmd/sendircode");
      doSubscribe(boardname,"cmd/sendbtcode");
      doSubscribe(boardname,"cmd/reboot");      

      sendMQTTMessage("status/booted","hello");   
      
      tp.DotStar_Clear(); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" Rebooting soon if a connection is not made. . . ");
      Serial.print(retries);
      Serial.println(" (10)");
      // Wait 10 seconds before retrying
      delay(10000);
      retries++;

     byte key = digitalRead(0);
     if (key == LOW) {
       hardreset();
         } 
    }
   }
   if(retries > 9)
   {
   tp.DotStar_Clear();  
   ESP.restart();
   }   
  } 
}
//===== Reconnect to the MQTT server ===============================================^


//===== Just some house keeping items =============================
void utilityloop() {
 
 char buf[16];

 //===== Send the IP address
 sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );   
 sendMQTTMessage("status/ipadress",buf);

 //===== Sent our version
 sendMQTTMessage("status/firmware",myversion);
   
}
//===== Just some house keeping items ============================^


//===== Config file routines ====================================
void loadconfig() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  //===== if we cannot mount the SPIFFS, prob means it's not formatted
  if (!SPIFFS.begin(true)) {
   Serial.println("Formatting SPIFFS"); 
   SPIFFS.format(); 
  }
  
  if (SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
       DynamicJsonDocument json(1024);
       auto error = deserializeJson(json, buf.get());
       
       // serializeJson(json, Serial);
        if (!error) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);   
          strcpy(boardname, json["boardname"]);  

          Serial.print("MQTT Server ");
          Serial.println(mqtt_server);                  
          
        } else {
          Serial.println("failed to load json config");
          Serial.println(error.c_str());
        }
      }
    }
    else {
     Serial.println("/config.json not found"); 
     noconfig = true;
    }
    
  } else {
    Serial.println("failed to mount FS");
  }
}

void saveconfig () {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);    
    Serial.println("Saving . . . . ");
    
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["boardname"] = boardname;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    //serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
}
//===== Config file routines ===================================^

uint8_t getkeys(const char* akey) {
 if (akey == "KEY_RETURN") {return KEY_RETURN;} 
 else
 if (akey == "KEY_UP_ARROW") {return KEY_UP_ARROW;} 
 else
 if (akey == "KEY_DOWN_ARROW") {return KEY_DOWN_ARROW;} 
 else
 if (akey == "KEY_LEFT_ARROW") {return KEY_LEFT_ARROW;} 
 else
 if (akey == "KEY_RIGHT_ARROW") {return KEY_RIGHT_ARROW;} 
 else
 if (akey == "KEY_BACKSPACE") {return KEY_BACKSPACE;}  
 else
 if (akey == "KEY_TAB") {return KEY_TAB;} 
 else
 if (akey == "KEY_ESC") {return KEY_ESC;} 
 else
 if (akey == "KEY_INSERT") {return KEY_INSERT;} 
 else
 if (akey == "KEY_DELETE") {return KEY_DELETE;} 
 else
 if (akey == "KEY_PAGE_UP") {return KEY_PAGE_UP;}  
 else
 if (akey == "KEY_PAGE_DOWN") {return KEY_PAGE_DOWN;} 
 else
 if (akey == "KEY_HOME") {return KEY_HOME;} 
 else
 if (akey == "KEY_END") {return KEY_END;}  

}
