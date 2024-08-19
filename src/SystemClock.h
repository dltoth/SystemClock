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

#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <Arduino.h>
#include "Instant.h"
#include "Timestamp.h"
#include "NTPTime.h"
#include "Timer.h"

#define GMT              0.0              // Timezone offset for GMT
#define DEFAULT_SYNC     60               // NTP Synchronization interval in minutes
#define JAN1_2024        3913056000UL     // System Time Initialization 
#define NTP_TIMEOUT      2000UL           // NTP timeout waiting on response

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

/**
 *   SystemClock provides NTP synchronized system time in terms of NTP Instant. 
 *   NTP synchronization happens with an on board Timer (syncTimer) that updates every ntpSync() minutes. The syncTimer can be turned off with setSyncTimerOFF(), 
 *   in which case, NTP synchronization happens on demand with sysTime() if the ntpSync() interval has passed.
 *   SystemClock must be initialized to a time close to (within 68 years of) the actual time UTC. The default initialization time is Jan 1, 2024 00:00:00 UTC.
 *   System time (sysTime()) is internally managed as UTC. For example, the following methods provide:
 *      sysTime()            - Current system time UTC, updating with NTP as necessary
 *      updateSysTime()      - Force NTP update and return current system time UTC
 *      startTime()          - The actual UTC start time of SystemClock
 *      initializationDate() - Initialization date/time in UTC
 *
 *   Additionaly, for convenience SystemClock has a timezone offset so some methods can provide system time as 
 *   Instant in local time:
 *      now()                - System time in local time
 *      lastSync()           - The last NTP synchronization in local time
 *      nextSync()           - The next expected NTP synchronization in local time
 *
 */
class SystemClock {
  public:

    SystemClock();
    virtual ~SystemClock() {}

    virtual Instant   now()                                      {return utcToLocal(sysTime());}   // Return system time in local time zone, synchronizing with NTP as necessary.
    virtual Instant   sysTime();                                                                   // Return system time in UTC, synchronizing with NTP as necessary.
    virtual Instant   updateSysTime();                                                             // Force NTP update to system time and return sysTime in UTC
    Instant           utcToLocal(const Instant& utc) const       {return utc + _tzOffset;}         // Convert utc Instant to local time from timezone offset
    const Timestamp&  startTime()                    const       {return _start;}                  // UTC Timestamp of clock start 

/**
 *    Initialize System Time for first update. As noted above, system time should be initialized to within 68 years
 *    of actual UTC. Default initialization is Jan 1, 2024 00:00:00
 */
    void             initialize(const Instant& ref)               {_sysTime.initialize(ref);_initDate = ref;}  // Initialize SystemClock time UTC
    const Instant&   initializationDate()                         {return _initDate;}                          // Get initialization date/time as Instant UTC
    void             reset()                                      {_lastSync=0;_sysTime=initializationDate();} // Reset SystemClock to its initialization date

/**
 *    Methods for timezone offset and NTP server address/port
 */
    double           tzOffset() const                             {return (double)_tzOffset/3600.0;}           // Get timezone offset in hours
    void             tzOffset( double hours );                                                                 // Set timezone offset in hours between -14.25 to + 14.25
    const IPAddress& serverAddress()  const                       {return _timeServer;}                        // Get timeserver IP address to use
    int              serverPort()     const                       {return _serverPort;}                        // Get timeserver port to use
    void             useNTPService(IPAddress addr,int port)       {_timeServer = addr; _serverPort = port;}    // Set timeserver IP address and port to use

/**
 *   Methods to manage NTP synchronization
 */
    void             ntpSync(unsigned int min);                                                                // Set NTP sync interval in minutes        
    unsigned int     ntpSync()        const                       {return _ntpSync;}                           // Get NTP sync interval in minutes
    Instant          lastSync()       const                       {return utcToLocal(Instant(_lastSync));}     // Local time of last NTP sync
    Instant          nextSync()       const                       {return utcToLocal(Instant(_nextSync));}     // Local time of next NTP sync
    void             setTimerOFF()                                {timerOFF(true);}                            // Turn syncTimer OFF
    void             setTimerON()                                 {timerOFF(false);}                           // Turn syncTimer ON
    boolean          timerOFF()       const                       {return _timerOFF;}                          // True of syncTimer is OFF
    boolean          timerON()        const                       {return !timerOFF();}                        // True if syncTImer is ON

/**
 *   Do a unit of work, in this case update the syncTimer
 */
    void             doDevice()                                   {_syncTimer.doDevice();}


  protected:
    void             timerOFF(boolean flg);
    void             resetSyncTimer();

    Instant         _initDate;                           // Clock initialization date, defaults to Jan 1, 2024
    Timestamp       _start;                              // Start time is first call sysTime()
    Timestamp       _sysTime;                            // System time from last call to sysTime()
    IPAddress       _timeServer;                         // Time server IP address
    int32_t         _tzOffset     = 0;                   // Timezone offset in secs defaults to UTC
    int             _serverPort   = 123;                 // Time server port
    int64_t         _nextSync     = 0;                   // System time of next NTP synchronization
    int64_t         _lastSync     = 0;                   // System time of last NTP synchronization
    unsigned int    _ntpSync      = DEFAULT_SYNC;        // NTP synchronization interval in minutes
    boolean         _timerOFF     = false;               // Turn syncTimer ON/OFF
    Timer           _syncTimer;                          // Timer to sync with NTP on the ntpSync interval

};

} // End of namespace lsc

#endif

