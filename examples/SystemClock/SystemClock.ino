
#include "SystemClock.h"
using namespace lsc;

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif

#define AP_SSID "MySSID"
#define AP_PSK  "MyPSK"

#define SERVER_PORT 80
#define EST         -5.0

#ifdef ESP8266
#define           BOARD "ESP8266"
ESP8266WebServer  server(SERVER_PORT);
#elif defined(ESP32)
#define          BOARD "ESP32"
WebServer        server(SERVER_PORT);
#endif

const char html_template[]   PROGMEM = "<!DOCTYPE html><html><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                                       "<head><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles.css\"></head>"
                                       "<body style=\"font-family: Arial\">"
                                         "<H3 align=\"center\"> SystemClock Test </H3><br><br>"
                                         "<div align=\"center\">"
                                            "<table>"
                                                 "<tr><td><b>Current Time:</b></td><td>&ensp;%s</td></tr>"
                                                 "<tr><td><b>Start Time:</b></td><td>&ensp;%s</td></tr>"
                                                 "<tr><td><b>Running Time:</b></td><td>&ensp;%s</td></tr>"
                                                 "<tr><td><b>Clock Offset:</b></td><td>&ensp;%f</td></tr>"
                                                 "<tr><td><b>Last NTP Sync:</b></td><td>&ensp;%s</td></tr>"
                                                 "<tr><td><b>Next NTP Sync:</b></td><td>&ensp;%s</td></tr>"
                                              "</table><br>"
                                         "</div>"
                                      "</body></html>";

Timer         timer;
SystemClock   c;

typedef struct NTPReport {
  char   startBuff[64];
  char   currentBuff[64];
  char   runningBuff[64];
  char   lastSync[64];
  char   nextSync[64];
  double clockOffset;
} NTPReport;


void runReport(NTPReport& report) {
  Timestamp current(c.sysTime());                                         // Current time as UTC Timestamp
  Instant   start       = c.startTime().ntpTime();                        // Clock start time
  Instant   offset      = NTPTime::ntpClockOffset(current);               // Note for offset computation, ref Timestamp has to be UTC
  start.toTimezone(EST).printDateTime(report.startBuff,64);
  current.ntpTime().toTimezone(EST).printDateTime(report.currentBuff,64);
  current.ntpTime().printElapsedTime(start, report.runningBuff,64);
  c.lastSync().printDateTime(report.lastSync,64);
  c.nextSync().printDateTime(report.nextSync,64);
  report.clockOffset = offset.sysTimed();
}

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println();
  Serial.printf("Starting NTP Test for Board %s\n",BOARD);

  WiFi.begin(AP_SSID,AP_PSK);
  Serial.printf("Connecting to Access Point %s\n",AP_SSID);
  while(WiFi.status() != WL_CONNECTED) {Serial.print(".");delay(500);}
  Serial.printf("\n\nWiFi Connected to %s with IP address: %s\n",WiFi.SSID().c_str(),WiFi.localIP().toString().c_str());

  server.begin(SERVER_PORT);
  Serial.printf("Web Server started on %s:%d\n",WiFi.localIP().toString().c_str(),SERVER_PORT);

/**
 *  Once the reference Timestamp has been synchronized with NTP, every update will track offset from the internal millisecond
 *  timer to NTP. 
 */

  c.ntpSync(1);                // Synchronize every 1 minutes
  c.tzOffset(-5.0);            // Set the timezone offset to EST

  char dateBuff[64];
  Instant start = c.now();
  start.printDateTime(dateBuff,64);     
  Serial.printf("SystemClock test started at %s \n",dateBuff);

/*
 *  Prime the pump once before reporting stats
 */
   c.updateSysTime();

/**
 *   Compute NTP offset from internal millisecond timer every 15 seconds
 */
  timer.set(0,0,15);        // Set an interval of 15 seconds
  timer.setHandler([]{
       NTPReport report;
       runReport(report);
       Serial.printf("\nCurrent Time:       %s\n",report.currentBuff);
       Serial.printf("Start Time:         %s\n",report.startBuff);
       Serial.printf("Running Time:       %s\n",report.runningBuff);
       Serial.printf("NTP Clock Offset:   %f\n",report.clockOffset);
       Serial.printf("Last NTP Sync:      %s\n",report.lastSync);
       Serial.printf("Next NTP Sync:      %s\n",report.nextSync);
       timer.start();
       });
  timer.start();

  server.on("/",[](){handleRequest();});

}

void loop() {
     timer.doDevice();
     server.handleClient();
     c.doDevice();
}

void handleRequest() {
  char htmlBuffer[1500];
  size_t buffLen = sizeof(htmlBuffer);
  NTPReport r;
  runReport(r);
  snprintf_P(htmlBuffer,buffLen,html_template,r.currentBuff,r.startBuff,r.runningBuff,r.clockOffset,r.lastSync,r.nextSync);
  server.send(200,"text/html",htmlBuffer); 
}

