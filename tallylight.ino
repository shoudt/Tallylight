/*
*
* Tallylight voor VMIX
*
*/
#include <Ticker.h>  //Ticker Library
#include <ESP8266HTTPClient.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WebServerClass = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
using WebServerClass = WebServer;
#endif
#include <FS.h>
#include <AutoConnect.h>
#define PARAM_FILE      "/params.json"
#define LED 12  

Ticker blinker;


String urlVMIX;
String cameraNaam;
String cameraKey;
String ledStat ="O";



WebServerClass  server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux  paramsAux;
AutoConnectAux  saveAux;

// Invul page parameters
static const char PAGE_PARAMS[] PROGMEM = R"(
{
  "uri": "/params",
  "title": "Tally parameters",
  "menu": true,
  "element": [
    {
      "name": "text",
      "type": "ACText",
      "value": "Parameters voor Tally-light",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "vmixhost",
      "type": "ACInput",
      "label": "VMIX host",
      "placeholder": "This area accepts hostname patterns",
      "pattern": ""
    },
    {
      "name": "cameranaam",
      "type": "ACInput",
      "label": "VMIX camera naam",
      "placeholder": "This area accepts hostname patterns",
      "pattern": ""
    },
    {
      "name": "load",
      "type": "ACSubmit",
      "value": "Load",
      "uri": "/params"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/save"
    },
    {
      "name": "adjust_width",
      "type": "ACElement",
      "value": "<script type=\"text/javascript\">window.onload=function(){var t=document.querySelectorAll(\"input[type='text']\");for(i=0;i<t.length;i++){var e=t[i].getAttribute(\"placeholder\");e&&t[i].setAttribute(\"size\",e.length*.8)}};</script>"
    }
  ]
}
)";

static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Parameters",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Elements have been saved to %s",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "validated",
      "type": "ACText",
      "style": "color:red"
    },
    {
      "name": "echo",
      "type": "ACText",
      "style": "font-family:monospace;font-size:small;white-space:pre;"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/params"
    }
  ]
}
)";



//=======================================================================
//                              Zet de led uit/aan
//=======================================================================
void changeState()
{
  digitalWrite(LED, !(digitalRead(LED)));  //Invert Current State of LED  
}

//=======================================================================
//                              Zet de led op flash
//=======================================================================
void ledFlash()
{
  if (ledStat!="F") {
    blinker.detach();
    blinker.attach(0.10, changeState);
    ledStat="F";     
  }
}

//=======================================================================
//                              Zet de led op blink
//=======================================================================
void ledBlink()
{
  if (ledStat!="B") {
    blinker.detach();
    blinker.attach(0.50, changeState);
    ledStat="B";
  }
}

//=======================================================================
//                              Zet de led aan
//=======================================================================
void ledOn()
{
  if (ledStat!="N") {
    blinker.detach();
    digitalWrite(LED,HIGH );
    ledStat="N";
  }  
}

//=======================================================================
//                              Zet de led uit
//=======================================================================
void ledOff()
{
  if (ledStat!="O") {
    blinker.detach();
    digitalWrite(LED,LOW );
    ledStat="O";
  }  
}

//=======================================================================
//                              Restart als er geen wifi is 
//=======================================================================
void onDisconnect(const WiFiEventStationModeDisconnected& event)
{
    ESP.restart();         
}

//=======================================================================
//                              Lees de camera status uit 
//=======================================================================

void HandleCameraStatus(String ck )
{
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
       HTTPClient http;  //Declare an object of class HTTPClient
       http.begin(urlVMIX+"/tallyupdate/?key="+ck);
       int httpCode = http.GET();                                                                  //Send the request
       if (httpCode > 0 and httpCode == 200) { //Check the returning code
         String payload = http.getString();   //Get the request response payload
         //Serial.println(payload);                     //Print the response payload
         if (payload=="tallyChange(\"#ff8c00\");") {
            ledBlink();
         } else 
         if (payload=="tallyChange(\"#006400\");") {
            ledOn(); 
         } else 
         if (payload=="tallyChange(\"#1a3c75\");") {
            ledOff();
         }
       } else {
         Serial.println(httpCode);
         ledFlash();        
       }
       http.end();   //Close connection
    }
  
}

//=======================================================================
//                              Zoek de key van de camera op 
//=======================================================================
String getCameraKey()
{
   String ck;
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
       HTTPClient http;  //Declare an object of class HTTPClient
       http.begin(urlVMIX+"/api");  //Specify request destination
       int httpCode = http.GET();                                                                  //Send the request

       if (httpCode > 0) {  
         String payload = http.getString();   //Get the request response payload
         int eol=0;
         String regel;
         boolean found = false;
         while (found==false) {
		   // Zoek een regel met input key 
           payload = payload.substring(payload.indexOf("<input key=\""));
           eol   = payload.indexOf("</input>");
           regel = payload.substring(0,eol+8);
           // Als cameraNaam in de regel voorkomt dan moetn we de key van de input hebben
           if (regel.indexOf("title=\""+cameraNaam+"\"")>0){
             found=true;        
             //String ck = 
             ck = regel.substring(regel.indexOf("\"")+1,regel.indexOf("\"",regel.indexOf("\"")+1));
           } else {
             payload = payload.substring(eol+8);
           }
         }         
       }
       http.end();   //Close connection
    }
    return ck; 
}

// haal de opgeslagen gegevens op 
void laadparams(AutoConnectAux& aux) {
      SPIFFS.begin();
      File param = SPIFFS.open(PARAM_FILE, "r");
      if (param) {
        aux.loadElement(param, { "vmixhost", "cameranaam" } );
        param.close();
      }
      SPIFFS.end();  
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  // Responder of root page handled directly from WebServer class.
  server.on("/", []() {
    String content = "Place the root page with the sketch application.&ensp;";
    content += AUTOCONNECT_LINK(COG_24);
    server.send(200, "text/html", content);
  });

  // Load a custom web page described in JSON as PAGE_ELEMENT and
  // register a handler. This handler will be invoked from
  // AutoConnectSubmit named the Load defined on the same page.
  paramsAux.load(FPSTR(PAGE_PARAMS));
  paramsAux.on([] (AutoConnectAux& aux, PageArgument& arg) {
    if (portal.where() == "/params") {
      // Use the AutoConnect::where function to identify the referer.
      // Since this handler only supports AutoConnectSubmit called the
      // Load, it uses the uri of the custom web page placed to
      // determine whether the Load was called me or not.
      laadparams(aux);
    }
    return String();
  });

  saveAux.load(FPSTR(PAGE_SAVE));
  saveAux.on([] (AutoConnectAux& aux, PageArgument& arg) {
    // You can validate input values ​​before saving with
    // AutoConnectInput::isValid function.
    // Verification is using performed regular expression set in the
    // pattern attribute in advance.
    AutoConnectInput& input = paramsAux["input"].as<AutoConnectInput>();
    //aux["validated"].value = input.isValid() ? String() : String("Input data pattern missmatched.");

    // The following line sets only the value, but it is HTMLified as
    // formatted text using the format attribute.
    aux["caption"].value = PARAM_FILE;

#if defined(ARDUINO_ARCH_ESP8266)
    SPIFFS.begin();
#elif defined(ARDUINO_ARCH_ESP32)
    SPIFFS.begin(true);
#endif
    File param = SPIFFS.open(PARAM_FILE, "w");
    if
    (param) {
      // Save as a loadable set for parameters.
      paramsAux.saveElement(param, { "vmixhost", "cameranaam" });
      param.close();
      // Read the saved elements again to display.
      param = SPIFFS.open(PARAM_FILE, "r");
      aux["echo"].value = param.readString();
      param.close();
    }
    else {
      aux["echo"].value = "SPIFFS failed to open.";
    }
    SPIFFS.end();
    return String();
  });

  portal.join({ paramsAux, saveAux });
  config.ticker = true;
  portal.config(config);
  portal.begin();
  
  // Laad de parameters
  laadparams(paramsAux);
  paramsAux.fetchElement();
  urlVMIX    = paramsAux["vmixhost"].value;
  cameraNaam = paramsAux["cameranaam"].value;
  Serial.println("vh "+urlVMIX);
  Serial.println("cn "+cameraNaam);

  pinMode(LED,OUTPUT);
  ledFlash();

  while (cameraKey=="") {
     cameraKey=getCameraKey();   
  }      
  
}

void loop() {
  portal.handleClient();
  HandleCameraStatus(cameraKey);
}
