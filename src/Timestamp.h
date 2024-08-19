
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

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include "Instant.h"

/**
 *  Timestamp class joins a sysTime Instant with a device millisecond timestamp. Updating the Timestamp folds milliseconds into the Instant, 
 *  while the original stamp is preserved.
 */

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

class Timestamp {
  public:

  Timestamp() : _ntpTime((int64_t)0),_millis(0),_stamp(0)            {}
  Timestamp(const Instant& ntpTime)                                  {initialize(ntpTime);}
  Timestamp(const Timestamp& T)                                      {_ntpTime = T.ntpTime();_millis = T.getMillis();_stamp = T.getStamp();}

  Instant           ntpTime()     const                              {return _ntpTime;}         // Return Instant that this Timestamp refers to
  unsigned long     getMillis()   const                              {return _millis;}          // Return millis since last update
  unsigned long     getStamp()    const                              {return _stamp;}           // Return millisecond timestamp of creation
  void              initialize(const Instant& sysTime)               {_ntpTime = sysTime; _millis = millis();_stamp = _millis;}
  Timestamp         update();                                        // Update ntpTime to current milliseconds

/**
 *  Construct a new Timestamp by updating an input Timestamp and stamping with current milliseconds.
 */
  static Timestamp  stampTime(const Timestamp& t)                    {Timestamp result=t;result.update();result._stamp=result.getMillis();return result;}

/**
 *  Operators to modify Instant but not timestamp. For example, adding a clock offset or adjusting
 *  to timezone.
 */
  friend Timestamp operator+(Timestamp lhs,const Timestamp& rhs)       {lhs += rhs;return lhs;}
  friend Timestamp operator-(Timestamp lhs,const Timestamp& rhs)       {lhs -= rhs;return lhs;}
  Timestamp&       operator+=(const Timestamp& rhs)                    {_ntpTime+=rhs._ntpTime;return *this;}
  Timestamp&       operator-=(const Timestamp& rhs)                    {*this += -rhs;return *this;}
  Timestamp        operator/(const int denom) const                    {Timestamp result = *this;result._ntpTime = _ntpTime/denom;return result;}
  Timestamp&       operator+=(const int rhs)                           {_ntpTime += rhs;return *this;}
  Timestamp&       operator-=(const int rhs)                           {_ntpTime -= rhs;return *this;}
  friend Timestamp operator+(const Timestamp& lhs, const int rhs)      {Timestamp result = lhs;result._ntpTime += rhs;return result;}
  friend Timestamp operator+(const int lhs, const Timestamp& rhs)      {return rhs+lhs;}
  friend Timestamp operator-(const Timestamp& lhs, const int rhs)      {Timestamp result = lhs;result._ntpTime -= rhs;return result;}
  Timestamp        operator-() const                                   {Timestamp result = *this;result._ntpTime = -_ntpTime;return result;}
  Timestamp&       operator+=(const Instant& rhs)                      {_ntpTime += rhs;return *this;}
  Timestamp&       operator-=(const Instant& rhs)                      {_ntpTime -= rhs;return *this;}
  friend Timestamp operator+(const Timestamp& lhs, const Instant& rhs) {Timestamp result = lhs;result._ntpTime += rhs;return result;}
  friend Timestamp operator+(const Instant& lhs, const Timestamp& rhs) {return rhs+lhs;}
  friend Timestamp operator-(const Timestamp& lhs, const Instant& rhs) {Timestamp result = lhs;result._ntpTime -= rhs;return result;}
  friend Instant   operator-(const Instant& lhs, const Timestamp& rhs) {Instant result = lhs - rhs._ntpTime;return result;}
  friend Timestamp abs(const Timestamp& ref)                           {Timestamp result = ref;result._ntpTime = abs(ref.ntpTime());return result;}

  static void printT(int i, const Timestamp& t) {Instant::printT(i,t.ntpTime());Serial.printf("     millis = %lu stamp = %lu\n",t._millis,t._stamp);}

  template<typename T>
  static void    printTs(int i,T t) {printT(i,t);}

  template<typename T, typename... Args>
  static void    printTs(int i,T t,Args... args) {printTs(i,t);printTs(i+1,args...);}

  private:

  Instant         _ntpTime;
  unsigned long   _millis = 0;
  unsigned long   _stamp  = 0;

};

} // End of namespace lsc

#endif
