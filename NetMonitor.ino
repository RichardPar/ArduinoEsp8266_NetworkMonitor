#include <ESP8266Ping.h>
#include <ESP8266WiFi.h>


extern "C" {
#include "user_interface.h"
}


//======= Your Settings here .. Add IP addresses to watch MAX 10 =======//
//======================================================================//
#define STASSID "YourAPHere"
#define STAPSK  "password here"

char *iplist[] = {"192.168.2.30","192.168.2.254","fred",0};
//======================================================================//


os_timer_t myTimer;
uint8_t wifistatus;
uint8_t oldstatus;
uint8_t resetstatus=0;
uint8_t resetcounter;
uint16_t lockout=0;

#define MAX_IP 10

const int relay1 = D5;
const int relay2 = D6;
const int button = 16;

char ipIndex=0;


struct iplist_t {
     uint8_t pingfail;
     bool    offline;
};

struct iplist_t statuslist[MAX_IP];




const char* ssid     = STASSID;
const char* password = STAPSK;


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
        resetstatus=0;
        lockout=1000;
      }
   }   
}


void user_init(void) {
     os_timer_setfn(&myTimer, timerCallback, NULL);
      os_timer_arm(&myTimer, 100, true);  
}

void setup() {


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
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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
 
  int r = key_led(0);
  if (r == true)
   {
    digitalWrite(relay2, true);
   } else
   digitalWrite(relay2, false);

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
      yield();
      return;
   }

   
   if(Ping.ping(iplist[ipIndex])){
    statuslist[ipIndex].pingfail=0;
    if (statuslist[ipIndex].offline == false)
     {
       Serial.print("IP ");
       Serial.print(iplist[ipIndex]);
       Serial.println(" online");
     }
    statuslist[ipIndex].offline = false;
    delay(600);
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
