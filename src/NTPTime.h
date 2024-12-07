
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

#ifndef NTPTIME_H
#define NTPTIME_H

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#define     NTP_PACKET_SIZE 48

#include <WiFiUdp.h>
#include "Instant.h"
#include "Timestamp.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

/** 
 *   Background:
 *   Network Time Protocol (NTP) is used for synchronizing clocks over the Internet. The wire protocol provides an unsigned 
 *   64 bit timestamp consisting of 32 bit seconds and 32 bit fraction representing seconds from 0h Jan 1, 1900 (refered to 
 *   as the prime epoch). Note that in representing seconds with only 32 bits, the timestamp will roll over every 136 years. 
 *   For this reason, NTP also specifies a 128 bit Datestamp. A Datestamp has the form:
 *      [signed 32 bit era][unsigned 32 bit era offset][64 bit fraction]
 *   The high order 32 bits of the seconds field represents a signed integer era, where each era represents 2**32 (2 raised 
 *   to the power 32) seconds, or 136 years. Positive era for timestamps after the prime epoch, and negative values for before. 
 *   The lower order 32 bits represents the era offset. Era offset is always a positive number of seconds offset from the era, 
 *   ranging fom 0 to 2**32-1. Era 0 begins moving forward from Jan 1, 1900 00:00:00, and era -1 begins moving backward from 
 *   that same point. So, the first second of era 0 is offset 1 on Jan 1, at 1900 00:00:01, and the first second of era -1
 *   is offset 4294967295 on Dec 31 1899 at 23:59:59.
 *   System time is then defined as a 64 bit signed integer seconds and 64 bit fraction, spanning 584 billion years with 
 *   attosecond accuracy. Positive values represent time after the prime epoch and negative values represent time prior to  
 *   the epoch.  
 *
 *   Here are a few examples:
 *   Era    Offset          System Time          Date/Time
 *    0     3913056000       3913056000          Jan 1,  2024  00:00:00
 *    0     4294967295       4294967295          Feb 7,  2036  06:28:15
 *    1     0                4294967296          Feb 7,  2036  06:28:16
 *    2     0                8589934592          Mar 15, 2172  12:56:32   
 *   -1     4294967295      -1                   Dec 31, 1899  23:59:59
 *   -1     0               -4294967296          Nov 24, 1763  17:31:44
 *   -2     0               -8589934592          Oct 18, 1627  11:03:28 
 *  
 *   Note:
 *      1. NTP time will roll from era 0 to era 1 on Feb 7, 2036 at 06:28:15.
 *      2. The relationship between era, offset, and system time is as follows:
 *            system time  = era*(2**32) + offset
 *            era          = (system time - offset)/(2**32)
 *                         = ((system time < 0)?((system time)/(2**32) - 1):((system time)/(2**32))) 
 *            era offset   = system time - era*(2**32)
 *      3. Era and era offset can be computed directly from the system time as:
 *            remainder    = system time % 2**32
 *            era offset   = ((remainder<0)?(remainder+2**32):(remainder))
 *            era          = ((remainder<0)?((system time/2**32)-1):(system time/2**32))
 *                                      
 *                                                                          |----offset--->|
 *            |---offset--->|<-remainder--|                |                |--remainder-->|
 *            |------------><-------(sysTime < 0)----------|--------(sysTime>0)----------->|
 *   _________+___________________________+_________________________________+_____________________+____________...
 *        era*2**32               (era+1)*2**32      ...   0   ...      era*2**32          (era+1)*2**32
 *                      ...    era < 0                     |              era > 0    ...
 *
 *
 *   Clock Synchronization:
 *   NTP is a request/response protocol over UDP, where a client computer sends a request for time to an NTP server. The NTP
 *   response headers provide two 64 bit timestamps, each containing a 32 bit seconds from 0h Jan 1, 1900 (UTC) and a 32 bit 
 *   fraction. Call these timestamps T2 and T3 respectively where:
 *      T2 - the timestamp on the NTP server that the request was received 
 *      T3 - the timestamp on the NTP server that the response was sent
 *   Define two additional timestamps:
 *      T1 - the timestamp on the client computer that the request was sent
 *      T4 - the timstamp on the client computer that the response was received
 *   The clock offset between client and NTP server can then be computed as:
 *      clock offset = ((T2-T1+(T3-T4)))/2
 *   where each of the timestamps T1,...T4 are 64 bit (seconds and fraction) unsigned integers. The client clock is then
 *   updated with the clock offset as follows:
 *      sysTime = T4 + clock offset;
 *   Note that even though timestamps are unsigned integers with 64 bits of precision, the offset is a signed integer with 63 
 *   bits of precision, so the timestamps must be within 68 years of each other to prevent overflow. This means that the client 
 *   clock must be initialized to within 68 years of the NTP server for the offset calculation. It also provides a means to determin 
 *   era roll over; if the clock offset is greater than 68 years (or less than -68 years) then the NTP timestamp rolled over and the 
 *   client must update era. NTP timestamps are given in terms of era offset, so the client timestamps T1 and T4 must also be given 
 *   as era offset and rolled over appropriately.
 * 
 *   For more detail on NTP see: https://www.rfc-editor.org/rfc/rfc5905
 *
 *   Class Description:
 *   NTPTime is a utility class to provide UTC time from an NTP server. UTC should only fetched on a defined interval, otherwise
 *   time should be returned relative to the internal ESP system clock (millis()). The following time servers are queried in this
 *   order, depending on success of host address resolution:
 *      time.google.com
 *      time.apple.com
 *      time-a.nist.gov
 *   System time is computed on the NTP timescale in seconds since Jan 1, 1900 00:00:00, in signed 64-bit integer seconds and unsigned 32-bit 
 *   fraction. It is assumed that the system clock runs forward from an initialized date/time within 68 years of NTP UTC. In other words, when 
 *   era rolls over it goes from n to n+1, and the era offset goes to 0. This means era and era offset can always be computed from system time as:
 *      era        = (system time)/(2**32)  and
 *      era offset = (system time)%(2**32)
 *
 *
 */
class NTPTime {
  public:

    NTPTime() {};

/**
 *   Resolve the IPAddress of one of the standard NTP Time Servers in the following order:
 *      time.google.com
 *      time.apple.com   or
 *      time-a.nist.gov: 129.6.15.28
 */
    static IPAddress   getTimeServerAddress();

/**
 *  Request NTP timestamps from the NTP Time Server located at ipAddress:port. Each timestamp is supplied in 2 parts as 
 *  unsigned 32 bit seconds and fraction.
 *   Input:   IPAddress        timeServer - IP Address of NTP time server 
 *            int              port       - Time server port
 *            unsigned long    timeout    - Time limit in milliseconds to wait for reponse from NTP server
 *   Output:  Timestamp&       t1         - Timestamp of NTP request, computed by stamping ref
 *            Timestamp&       t2         - Timestamp request was received on the NTP server
 *            Timestamp&       t3         - Timestamp response was sent on the NTP server
 *            Timestamp&       t4         - Timestamp NTP response was received
 *   Returns 1 on success otherwise:
 *     status -1: Error initializing udp channel on begin()
 *     status -2: Error writing udp packet to the channel
 *     status -3: Time limit exceeded waiting on response from NTP server
 *
 *  On error, return values are initialized to 0
 */
    static int        getNTPTimestamp(uint32_t& rcvSecs, uint32_t& rcvFraction, uint32_t& tsmSecs, uint32_t& tsmFraction, unsigned long timeout = NTP_TIMEOUT, IPAddress timeServer = NTP_SERVER, int port = NTP_PORT);

/**
 *    Calculate NTP clock offset based on the input timestamp and return a Timestamp updated with NTP clock offset, and
 *    the clock offset. Timestamps are requested from an NTP Time Service located at the input IPAddress and port.
 *   Input:   IPAddress        timeServer - IP Address of NTP time server 
 *            int              port       - Time server port
 *            const Timestamp& ref        - current sysTime
 *            unsigned long    timeout    - Time limit in milliseconds to wait for reponse from NTP server
 *   Output:  Instant&         osft       - NTP clock offset
 *   Returns: Updated system time as Timestamp
 */
    static Timestamp   updateSysTime(Instant& ofst, const Timestamp& ref, unsigned long timeout = NTP_TIMEOUT, IPAddress timeServer = NTP_SERVER, int port = NTP_PORT );

/**
 *   Calculate the NTP clock offset from the internal millisecond timer using the input UTC Timestamp ref as a template. 
 *   Note that the input ref Timestamp is used only as an initialized template, and once synchronized to NTP is used
 *   to calculate drift between the internal millisecond timer and NTP. The offset is computed from uint32_t timestamps 
 *   requested from an NTP Time Service located at the input IPAddress and port. For example, the following code will loop 
 *   for an hour, printing the offset between the internal millisecond timer and NTP every 15 seconds:
 *     Instant    initTime(0,JAN1_2024,0);            // Create an Instant initialized to Jan 1, 2024 00:00:00
 *     Timestamp  ref(initTime);                      // Stamp initTime with current millis()
 *     Timestamp  ref = NTPTime::updateSysTime(ref);  // Call out to an NTP server and update ref to current NTP UTC
 *     int        iteration = 0;
 *     int        waitTime  = 15000;                  // Loop every 15 seconds
 *     while(iteration++ < 240) {                     // Loop for an hour
 *        Instant offset = NTPTime::ntpClockOffset(ref);
 *        Serial.printf("Offset is %f\n",ref.ntpTime().sysTimed());
 *        delay(waitTime);
 *     }                       
 *   
 *   Most ESP8266 devices seem to drift about .1 second per hour
 * 
 *   Input:   IPAddress        timeServer - IP Address of NTP time server 
 *            int              port       - Time server port
 *            unsigned long    timeout    - Time limit in milliseconds to wait for reponse from NTP server
 *            const Timestamp& ref        - current sysTime
 *   Returns: NTP clock offset as Instant
 */
    static Instant     ntpClockOffset(const Timestamp& ref, unsigned long timeout = NTP_TIMEOUT, IPAddress timeServer = NTP_SERVER, int port = NTP_PORT);

    private:

/** 
 *   Convenience method to compute clock offset and return intermediate computations
 *   Input:   IPAddress        timeServer - IP Address of NTP time server 
 *            int              port       - Time server port
 *            const Timestamp& ref        - current sysTime
 *            unsigned long    timeout    - Time limit in milliseconds to wait for reponse from NTP server
 *   Output:  Timestamp&       t1         - Timestamp of NTP request, computed by stamping ref
 *            Timestamp&       t2         - Timestamp request was received on the NTP server
 *            Timestamp&       t3         - Timestamp response was sent on the NTP server
 *            Timestamp&       t4         - Timestamp NTP response was received
 *   Returns: NTP clock offset as Instant
 *
 *   Note that the Timestamp Instants of T2 and T3 come from the NTP server but their actual millisecond timestamps come 
 *   from this client, stamped just before and just after the call to NTPTime::getNTPTimestamp()
 *
 *   Updated system time can then be computed as: Timestamp sysTime = t4 + clockOffset
 *   On error, clock offset is set to Instant(0,0) so t4 will not be affected by applying the offset.
 */
    static Instant     getNTPOffset(Timestamp& t1,  Timestamp& t2,  Timestamp& t3,  Timestamp& t4, const Timestamp& ref, unsigned long timeout = NTP_TIMEOUT, IPAddress timeServer = NTP_SERVER, int port = NTP_PORT);

    static IPAddress       NTP_SERVER;
    static int             NTP_PORT;
    static unsigned long   NTP_TIMEOUT;

};

} // End of namespace lsc

#endif
