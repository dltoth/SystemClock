
/**
 * 
 *  SystemClock Library
 *  Copyright (C) 2024  Daniel L Toth
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published 
 *  by the Free Software Foundation, either version 3 of the License, or any 
 *  later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 *  The author can be contacted at dan@leelanausoftware.com  
 *
 */

#include <math.h>
#include "NTPTime.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

IPAddress     NTPTime::NTP_SERVER  = NTPTime::getTimeServerAddress();
int           NTPTime::NTP_PORT    = 123;
unsigned long NTPTime::NTP_TIMEOUT = 2000;

IPAddress NTPTime::getTimeServerAddress() {
  IPAddress result;
  int err = WiFi.hostByName("time.google.com", result);
  if( err != 1 ) {
    Serial.printf("NTPTime::getTimeServerAddress: Error resolving time.google.com: %d\n", err);
    err = WiFi.hostByName("time.apple.com", result);
    if( err != 1 ) {
       Serial.printf("NTPTime::getTimeServerAddress: Error resolving time.apple.com: %d using time-a.nist.gov: 129.6.15.28\n", err);
       result = IPAddress(129,6,15,28);  // time-a-g.nist.gov
    }
  }
  return result;
}

/**
 *  Request timestamps from the NTP Time Server located at ipAddress:port, returns NTP Timestamps:
 *     uint32_t rcvSecs      - NTP clock seconds the reqest arrived
 *     uint32_t rcvFraction  - Fraction of second the request arrived
 *     uint32_t tsmSecs      - NTP clock seconds the response was transmitted
 *     uint32_t tsmFraction  - Fraction of second the response was transmitted
 *  All return values are initialized to 0 at entry, even in the event of error
 *
 *  Other header values collected but not returned
 *     int      LI           - Leap Indicator
 *     int      VER          - NTP Version
 *     int      MODE         - Mode (Client = 3, Server = 4)
 *     int      STRATUM      - Stratum (0 = KoD, 1 = Primary Server, 2.. Secondary)
 *     int      POLL         - Poll Interval (Log base2)
 *     int      PRECISION    - Clock Precision (Log base2)
 *
 *  Returns 1 on success otherwise:
 *     status -1: Error initializing udp channel on begin()
 *     status -2: Error writing udp packet to the channel
 *     status -3: Time limit exceeded waiting for NTP response
 *  On error, return values are initialized to 0;
 */
int  NTPTime::getNTPTimestamp(uint32_t& rcvSecs, uint32_t& rcvFraction, uint32_t& tsmSecs, uint32_t& tsmFraction, unsigned long timeout, IPAddress timeServer, int port ) {

/** Return values
 */
  rcvSecs       = 0;
  rcvFraction   = 0;
  tsmSecs       = 0;
  tsmFraction   = 0;

/** Other interesting header values
 */
  int  LI       = 0;
  int  VER      = 0; 
  int  MODE     = 0; 
  int  STRATUM  = 0; 
  int  POLL     = 0; 
  int  PREC     = 0;
  char REFID[5] = {0,0,0,0,0};  

/** Set up the UDP channel
 *    
 */ 
  WiFiUDP udpChannel;
  int status = udpChannel.begin(0);
  if( status != 1 ) {
    Serial.printf("Error initializing UDP channel (on udpChannel.begin)\n");
    status = -1;
    udpChannel.stop();
    return status;
  }
  
  byte packetBuffer[NTP_PACKET_SIZE];     //buffer to hold incoming & outgoing packets  
  while (udpChannel.parsePacket() > 0) ;  // discard any previously received packets
    
/**
 *    Send the NTP Request Packet
 *    Initialize values needed to form NTP request
 */
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b00100011;   // LI 0 (00), Version 4 (100), Mode 3  (011)  Fixed from 0b11100011
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision

  // 8 bytes of zero for Root Delay & Root Dispersion
/**
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
 */
  
  packetBuffer[12]  = 'L';
  packetBuffer[13]  = 'S';
  packetBuffer[14]  = 'C';

/** All NTP fields have been given values, now send a packet on port 123 requesting a timestamp  
 */
  udpChannel.beginPacket(timeServer, port);
  udpChannel.write(packetBuffer, NTP_PACKET_SIZE);
  status = udpChannel.endPacket();
  if( status != 1 ) {
    Serial.printf("Error writing UDP packet to channel\n");
    status = -2;
  }
  else {


/**
 *   Read NTP Response
 */
    unsigned long beginWait  = millis();
    bool          done       = false;
    while (((millis() - beginWait) < timeout) && !done) {
      int size = udpChannel.parsePacket();
      if (size >= NTP_PACKET_SIZE) {

        udpChannel.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer

/**
 *    Parse the header: Leap Indicator (2 bits), Version (3 bits), Mode (3 bits), Stratum (8 bits), Poll (8 bits), Precision (8 bits), and RefID (char[5])
 */
        LI        = (packetBuffer[0] >> 6) & 3;
        VER       = (packetBuffer[0] >> 3) & 7;
        MODE      = packetBuffer[0] & 7;
        STRATUM   = packetBuffer[1];
        POLL      = (signed char) packetBuffer[2];
        PREC      = (signed char) packetBuffer[3];
        REFID[0]  = (char) packetBuffer[12];
        REFID[1]  = (char) packetBuffer[13];
        REFID[2]  = (char) packetBuffer[14];
        REFID[3]  = (char) packetBuffer[15];

/**
 *    Get recieve time from NTP Server, seconds and fraction (T2)
 */
      // convert four bytes starting at location 32 to an unsigned long integer secs since 1900
        rcvSecs        = (unsigned long)packetBuffer[32] << 24;
        rcvSecs       |= (unsigned long)packetBuffer[33] << 16;
        rcvSecs       |= (unsigned long)packetBuffer[34] << 8;
        rcvSecs       |= (unsigned long)packetBuffer[35];
              
        // convert four bytes starting at location 44 to an unsigned long integer fraction of a second.
        rcvFraction    = (unsigned long)packetBuffer[36] << 24;
        rcvFraction   |= (unsigned long)packetBuffer[37] << 16;
        rcvFraction   |= (unsigned long)packetBuffer[38] << 8;
        rcvFraction   |= (unsigned long)packetBuffer[39];


/**
*    Get transmit time from NTP Server, seconds and fraction (T3)
*/
        // convert four bytes starting at location 40 to an unsigned long integer secs since 1900
        tsmSecs  = (unsigned long)packetBuffer[40] << 24;
        tsmSecs |= (unsigned long)packetBuffer[41] << 16;
        tsmSecs |= (unsigned long)packetBuffer[42] << 8;
        tsmSecs |= (unsigned long)packetBuffer[43];

        // convert four bytes starting at location 44 to an unsigned long integer fraction of a second.
        tsmFraction    = (unsigned long)packetBuffer[44] << 24;
        tsmFraction   |= (unsigned long)packetBuffer[45] << 16;
        tsmFraction   |= (unsigned long)packetBuffer[46] << 8;
        tsmFraction   |= (unsigned long)packetBuffer[47];

        done = true;

/**
        Serial.printf("\nNTPTime::getNTPTimestamp: LI = %d VI = %d MODE = %d STRATUM = %d POLL = %d PREC = %d\n",LI,VER,MODE,STRATUM,POLL,PREC);
        Serial.printf("                          REFID = %s\n",REFID);
        Serial.printf("                          rcvSecs = %lu rcvFraction = %lu\n",rcvSecs,rcvFraction);
        Serial.printf("                          tsmSecs = %lu tsmFraction = %lu\n",tsmSecs,tsmFraction);
**/

      }
    }
    if( !done ) status = -3;
  }

/** 
 *  Tear down the UDP channel
 */
  udpChannel.stop();
  return status; 
}

Timestamp NTPTime::updateSysTime( Instant& clockOffset, const Timestamp& ref, unsigned long timeout, IPAddress timeServer, int port ) {
  Timestamp t1,t2,t3,t4;
  clockOffset = NTPTime::getNTPOffset(t1,t2,t3,t4,ref,timeout,timeServer,port);
  t4 += clockOffset;
  return t4;
}

Instant NTPTime::ntpClockOffset(const Timestamp& ref, unsigned long timeout, IPAddress timeServer, int port) {
  Timestamp t1,t2,t3,t4;
  Instant result = NTPTime::getNTPOffset(t1,t2,t3,t4,ref,timeout,timeServer,port);
  return result;
}

Instant NTPTime::getNTPOffset(Timestamp& t1, Timestamp& t2, Timestamp& t3, Timestamp& t4, const Timestamp& ref, unsigned long timeout, IPAddress timeServer, int port) {
  uint32_t rcvSecs, rcvFraction, tsmSecs, tsmFraction;
  Instant zero, T2, T3;

/**
 *  Stamp millis prior to NTP call, t2 initialized to zero Instant
 */
  t1 = Timestamp::stampTime(ref);
  t2.initialize(zero);

  int status = getNTPTimestamp(rcvSecs, rcvFraction, tsmSecs, tsmFraction, timeout, timeServer, port);

/**
*  Stamp millis after NTP call, t3 initialized to zero Instant
*/
  t3.initialize(zero);
  t4 = Timestamp::stampTime(t1);

  Instant   T1     = t1.ntpTime();
  Instant   T4     = t4.ntpTime();
/**
 *  Era is computed for both T1 and T4 in case era rolled between timestamps
 */
  int32_t  T1Era   = T1.era();
  int32_t  T4Era   = T4.era();
  
/**
 *   At this point we have offsets for T2 and T3 but still require era for a complete
 *   system time. We check client and server offsets. Assuming client and server system 
 *   time are within 68 years, if their respective era offsets exceed 68 years, clocks 
 *   clocks span an era boundary, and server eras are either 1 greater than or 1 less than 
 *   client era. Otherwise, client and server are in the same era, and T2 and T3 can be 
 *   given the same era as T1 and T4.
 *   On error with timestamp request, set T2 = T1 and T3 = T4, so clockOffset is 0.
 *   In other words, an error retrieving NTP timestamps will have net zero affect.
 */
  if( status == 1) {

/**
 *   Compute difference between client and server NTP offsets, both recieve and transmit
 */
     int64_t  rcvDiff = (int64_t)T1.eraOffset() - (int64_t)rcvSecs;
     int64_t  tsmDiff = (int64_t)T4.eraOffset() - (int64_t)tsmSecs;

/**
 *   In most cases, system time will have rolled rignt along with the NTP server and
 *   the diff will be within 68 years (era offsets are being compared). The main reason
 *   for this check is to assure proper calculation at startup, in the case where initialized 
 *   system time and NTP server are in adjoining ERAs
 *
 *   Note that while it is possible for the difference to be greater than 68 years with both 
 *   offsets in the same era, if we assume system time is within 68 years, the only way for the 
 *   offset difference to be greater than 68 years (or less than -68) is for clocks to straddle 
 *   an era boundary.
 */
     if( rcvDiff > SECS_IN_68_YEARS ) T2.initialize(T1Era+1,rcvSecs,rcvFraction);        // Server rolled first
     else if( rcvDiff < -SECS_IN_68_YEARS ) T2.initialize(T1Era-1,rcvSecs,rcvFraction);  // Client rolled first
     else T2.initialize(T1Era,rcvSecs,rcvFraction);                                      // Client and server in same era
     if( tsmDiff > SECS_IN_68_YEARS ) T3.initialize(T4Era+1,tsmSecs,tsmFraction);        // Server rolled first
     else if( tsmDiff < -SECS_IN_68_YEARS ) T3.initialize(T4Era-1,tsmSecs,tsmFraction);  // Client rolled first
     else T3.initialize(T4Era,tsmSecs,tsmFraction);                                      // Client and server in same era
  }
  else {
    T2 = T1;
    T3 = T4;
  }

/**
 *   
 */
  t2 += T2;
  t3 += T3;

/**
  Serial.printf("NTPTime::getNTPOffset: Timestamps...\n");
  Timestamp::printTs(1,t1,t2,t3,t4);
  Serial.printf("\n");

  Serial.printf("\nNTPTime::getNTPOffset: Instants...\n");
  Instant::printTs(1,T1,T2,T3,T4);
  Serial.printf("\n");
**/

  Instant clockOffset = ((T2-T1)+(T3-T4))/2;
  return clockOffset;
}

} // End of namespace lsc

