#include <ESP8266Ping.h>
#include <ESP8266WiFi.h>


extern "C" {
#include "user_interface.h"
}


//======= Your Settings here .. Add IP addresses to watch MAX 10 =======//
//======================================================================//
//#define STASSID "........"       <<--------  not used, WPS is enabled
//#define STAPSK  "..........."    <<--------  not used as connection is via WPS

char *iplist[] = {"192.168.0.240","192.168.0.241","192.168.0.242","192.168.0.243","192.168.0.244","192.168.0.245","192.168.0.246","192.168.0.247","192.168.0.248","192.168.0.249",0};
//======================================================================//


os_timer_t myTimer;
uint8_t wifistatus;
uint8_t oldstatus;
uint8_t resetstatus=0;
uint8_t resetcounter;
uint16_t lockout=0;

#define MAX_IP 20

const int relay1 = D5;
const int relay2 = D6;
const int button = 16;

char ipIndex=0;


struct iplist_t {
     uint8_t pingfail;
     bool    offline;
};

struct iplist_t statuslist[MAX_IP];




//const char* ssid     = STASSID;
//const char* password = STAPSK;


void reset_iplist(void)
{
  char a;
  for (a=0;a<MAX_IP;a++)
     {
      statuslist[a].offline=true;
      statuslist[a].pingfail=0;
     }
}

// KEY_LED functions
uint8_t key_led( uint8_t level)
{
  uint8_t state;
  digitalWrite(button, 1);
  pinMode(button, INPUT);
  state = digitalRead(button);
  pinMode(button, OUTPUT);
  digitalWrite(button, level);
  return state;
}


void timerCallback(void *pArg) {

     if (lockout > 0)
     {
       lockout--;
       if (lockout == 0)
           Serial.println("Lockout Cleared");
       return;
     }

  
    if (resetstatus == 1)
    {
      digitalWrite(relay2, false);
      digitalWrite(relay1, false);
      resetstatus=2;
      resetcounter=100;
    }

   if (resetstatus==2)
   {
      if (resetcounter > 0)
      {
         resetcounter--; 
      } else
      {
        digitalWrite(relay2, true);
        digitalWrite(relay1, true);
        resetstatus=0;
        lockout=1000;
      }
   }   
}


void user_init(void) {
     os_timer_setfn(&myTimer, timerCallback, NULL);
      os_timer_arm(&myTimer, 100, true);  
}

bool startWPSPBC() {
// from https://gist.github.com/copa2/fcc718c6549721c210d614a325271389
// wpstest.ino
  Serial.println("WPS config start");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess) {
      // Well this means not always success :-/ in case of a timeout we have an empty ssid
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        // WPSConfig has already connected in STA mode successfully to the new station. 
        Serial.printf("WPS finished. Connected successfull to SSID '%s'\n", newSSID.c_str());
      } else {
        wpsSuccess = false;
      }
  }
  return wpsSuccess; 
}

void setup() {

  lockout=0;
  ipIndex=0;
  reset_iplist();
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(relay1, true);
  digitalWrite(relay2, true);

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WiFi.SSID().c_str());

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, 
  
  int r = key_led(0);
  if (r == false)
   {
      bool rc = startWPSPBC();

      if (rc == true)
        {
         Serial.println("WPS Successful");
         WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, 
        } else 
         Serial.println("WPS failed");
   }
  
  while (WiFi.status() == WL_DISCONNECTED) {          // last saved credentials
    delay(500);
    Serial.print(".");
  }

  user_init();
    
}

void loop() {


  if (resetstatus > 0)
  {
     yield();
     return;
  }

  wifistatus = WiFi.status();

  if (wifistatus != oldstatus)
   {

     if (wifistatus == WL_CONNECTED)
      {  
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      } else
      {
        Serial.println("");
        Serial.println("WiFi disconnected");
      }
     oldstatus=wifistatus;
   }

 if (wifistatus != WL_CONNECTED)
 {
  yield();
  return;
 }
 
  if(Ping.ping(WiFi.gatewayIP())){
     Serial.print("Gateway ");
     Serial.print(WiFi.gatewayIP());
     Serial.println(" online");
  } else
  {
     Serial.print("Gateway ");
     Serial.print(WiFi.gatewayIP());
     Serial.println(" offline");
     yield();
     return;
  }


  if (lockout > 0)
   {
      Serial.println("Locked out");
      yield();
      return;
   }

   
   if(Ping.ping(iplist[ipIndex])){
    statuslist[ipIndex].pingfail=0;
    //if (statuslist[ipIndex].offline == false)
    // {
       Serial.print("IP ");
       Serial.print(iplist[ipIndex]);
       Serial.println(" online");
     //}
      statuslist[ipIndex].offline = false;
    } else 
    {
      Serial.print("IP ");
      Serial.print(iplist[ipIndex]);
      Serial.println(" offline");
      if (statuslist[ipIndex].pingfail < 100)
         statuslist[ipIndex].pingfail++;

      if ((statuslist[ipIndex].pingfail > 2) && (statuslist[ipIndex].offline == false))
        {
          resetstatus=1;
          statuslist[ipIndex].offline = true;
        }
    } 
    ipIndex++;
    if (iplist[ipIndex] == 0)
      ipIndex=0; 

}
