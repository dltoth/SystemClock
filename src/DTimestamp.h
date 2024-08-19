
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

#ifndef DTIMESTAMP_H
#define DTIMESTAMP_H

#include "DInstant.h"

/**
 *  DTimestamp class connects a sysTime DInstant with a device millisecond timestamp.
 */

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

class DTimestamp {
  public:

  DTimestamp() : _ntpTime((int64_t)0),_millis(0)                     {}
  DTimestamp(const DInstant& ntpTime)                                {initialize(ntpTime);}

  DInstant          ntpTime()   const                                {return _ntpTime;}
  unsigned long     getMillis() const                                {return _millis;}                                           
  void              initialize(const DInstant& sysTime)              {_ntpTime = sysTime; _millis = millis();}
  void              update();                                                                    // Update ntpTime to current milliseconds

/**
 *  Stamp an DInstant with a new timestamp and return the result
 */
  static DTimestamp  stampTime(const DTimestamp& t)                    {DTimestamp result=t;result.update();return result;}

/**
 *  Operators to modify DInstant but not timestamp. For example, adding a clock offset or adjusting
 *  to timezone.
 */
  friend DTimestamp operator+(DTimestamp lhs,const DTimestamp& rhs)     {lhs += rhs;return lhs;}
  friend DTimestamp operator-(DTimestamp lhs,const DTimestamp& rhs)     {lhs -= rhs;return lhs;}
  DTimestamp&       operator+=(const DTimestamp& rhs)                   {_ntpTime+=rhs._ntpTime;return *this;}
  DTimestamp&       operator-=(const DTimestamp& rhs)                   {*this += -rhs;return *this;}
  DTimestamp        operator/(const int denom) const                    {DTimestamp result = *this;result._ntpTime = _ntpTime/denom;return result;}
  DTimestamp&       operator+=(const int rhs)                           {_ntpTime += rhs;return *this;}
  DTimestamp&       operator-=(const int rhs)                           {_ntpTime -= rhs;return *this;}
  friend DTimestamp operator+(const DTimestamp& lhs, const int rhs)     {DTimestamp result = lhs;result._ntpTime += rhs;return result;}
  friend DTimestamp operator+(const int lhs, const DTimestamp& rhs)     {return rhs+lhs;}
  friend DTimestamp operator-(const DTimestamp& lhs, const int rhs)     {DTimestamp result = lhs;result._ntpTime -= rhs;return result;}
  DTimestamp        operator-() const                                   {DTimestamp result = *this;result._ntpTime = -_ntpTime;return result;}
/***/
  DTimestamp&       operator+=(const DInstant& rhs)                      {_ntpTime += rhs;return *this;}
  DTimestamp&       operator-=(const DInstant& rhs)                      {_ntpTime -= rhs;return *this;}
  friend DTimestamp operator+(const DTimestamp& lhs, const DInstant& rhs) {DTimestamp result = lhs;result._ntpTime += rhs;return result;}
  friend DTimestamp operator+(const DInstant& lhs, const DTimestamp& rhs) {return rhs+lhs;}
  friend DTimestamp operator-(const DTimestamp& lhs, const DInstant& rhs) {DTimestamp result = lhs;result._ntpTime -= rhs;return result;}
  friend DInstant   operator-(const DInstant& lhs, const DTimestamp& rhs) {DInstant result = lhs - rhs._ntpTime;return result;}

  private:

  DInstant        _ntpTime;
  unsigned long   _millis = 0;

};

} // End of namespace lsc

#endif
