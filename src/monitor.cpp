#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include "media/Free_Fonts.h"
#include "media/images.h"
#include "mbedtls/md.h"
#include "OpenFontRender.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "mining.h"
#include "utils.h"
#include "monitor.h"

extern unsigned long templates;
extern unsigned long hashes;
extern unsigned long Mhashes;
extern unsigned long totalKHashes;
extern unsigned long elapsedKHs;

extern unsigned long halfshares; // increase if blockhash has 16 bits of zeroes
extern unsigned int shares; // increase if blockhash has 32 bits of zeroes
extern unsigned int valids; // increased if blockhash <= targethalfshares

extern OpenFontRender render;
extern TFT_eSprite background;
extern monitor_data mMonitor;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup_monitor(void){
    /******** TIME ZONE SETTING *****/

    timeClient.begin();
    
    // Adjust offset depending on your zone
    // GMT +2 in seconds (zona horaria de Europa Central)
    timeClient.setTimeOffset(7200);

    Serial.println("TimeClient setup done");
}

String printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "00:00";
  }
  char LocalHour[80];
  strftime (LocalHour, 80, "%H:%M", &timeinfo); //4 digit year, 2 digit month
  String mystring(LocalHour); 
  return LocalHour;
}

unsigned long mTriggerUpdate = 0;
unsigned long initialMillis = millis();
unsigned long initialTime = 0;

String getTime(void){
  
  //Check if need an NTP call to check current time
  if((mTriggerUpdate == 0) || (millis() - mTriggerUpdate > UPDATE_PERIOD_h * 60 * 1000)){
    if(WiFi.status() != WL_CONNECTED) return "";
    timeClient.update(); //NTP call to get current time
    mTriggerUpdate = millis();
    initialTime = timeClient.getEpochTime(); // Guarda la hora inicial (en segundos desde 1970)
    Serial.print("TimeClient NTPupdateTime ");
  }

  unsigned long elapsedTime = (millis() - mTriggerUpdate) / 1000; // Tiempo transcurrido en segundos
  unsigned long currentTime = initialTime + elapsedTime; // La hora actual

  // convierte la hora actual en horas, minutos y segundos
  unsigned long currentHours = currentTime % 86400 / 3600;
  unsigned long currentMinutes = currentTime % 3600 / 60;
  unsigned long currentSeconds = currentTime % 60;

  char LocalHour[10];
  sprintf(LocalHour, "%02d:%02d", currentHours, currentMinutes);
  
  String mystring(LocalHour);
  return LocalHour;
}

void show_MinerScreen(unsigned long mElapsed){

    //Print background screen
    background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen); 

    char CurrentHashrate[10] = {0};
    sprintf(CurrentHashrate, "%.2f", (1.0*(elapsedKHs*1000))/mElapsed);

    //Serial.println("[runMonitor Task] -> Printing results on screen ");
    
     Serial.printf(">>> Completed %d share(s), %d Khashes, avg. hashrate %s KH/s\n",
      shares, totalKHashes, CurrentHashrate);

    //Hashrate
    render.setFontSize(70);
    render.setCursor(19, 118);
    render.setFontColor(TFT_BLACK);
    
    render.rdrawString(CurrentHashrate, 118, 114, TFT_BLACK);
    //Total hashes
    render.setFontSize(36);
    render.rdrawString(String(Mhashes).c_str(), 268, 138, TFT_BLACK);
    //Block templates
    render.setFontSize(36);
    render.drawString(String(templates).c_str(), 186, 20, 0xDEDB);
    //16Bit shares
    render.setFontSize(36);
    render.drawString(String(halfshares).c_str(), 186, 48, 0xDEDB);
    //32Bit shares
    render.setFontSize(36);
    render.drawString(String(shares).c_str(), 186, 76, 0xDEDB);
    //Hores
    unsigned long secElapsed=millis()/1000;
    int hr = secElapsed/3600;                                                        //Number of seconds in an hour
    int mins = (secElapsed-(hr*3600))/60;                                              //Remove the number of hours and calculate the minutes.
    int sec = secElapsed-(hr*3600)-(mins*60);   
    render.setFontSize(36);
    render.rdrawString(String(hr).c_str(), 208, 99, 0xDEDB);
    //Minutss
    render.setFontSize(36);
    render.rdrawString(String(mins).c_str(), 253, 99, 0xDEDB);
    //Segons
    render.setFontSize(36);
    render.rdrawString(String(sec).c_str(), 298, 99, 0xDEDB);
    //Valid Blocks
    render.setFontSize(48);
    render.drawString(String(valids).c_str(), 285, 56, 0xDEDB);

    //Print Temp
    String temp = String(temperatureRead(), 0);
    render.setFontSize(20);
    render.rdrawString(String(temp).c_str(), 239, 1, TFT_BLACK);

    render.setFontSize(7);
    render.rdrawString(String(0).c_str(), 244, 3, TFT_BLACK);

    //Print Hour
    render.setFontSize(20);
    render.rdrawString(getTime().c_str(), 286, 1, TFT_BLACK);

    //Push prepared background to screen
    background.pushSprite(0,0);
}