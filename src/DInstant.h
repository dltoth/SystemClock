
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

#ifndef DINSTANT_H
#define DINSTANT_H
#include "Instant.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

class DInstant {
  public:
	DInstant()                                                     {setSysTime(0.0);}
	DInstant(const DInstant& ref)                                  {*this = ref;}
	DInstant(int32_t e, uint32_t o, uint32_t f)                    {initialize(e,o,f);}
	DInstant(double sysd)                                          {setSysTime(sysd);}
  DInstant(int64_t secs)                                         {setSysTime((double)secs);}
	DInstant(Date& d, Time& t)                                     {initialize(d,t);}

	void           setSysTime(double secs)                        {_sysTime = secs;}
	void           initialize(const Date& d, const Time& t)       {*this = DInstant::toDInstant(d,t);}
	void           initialize(int32_t e,uint32_t o,uint32_t f=0)  {double fraction = (double)f/DPOW2_32;_sysTime = (double)e*DPOW2_32+(double)o + fraction;}

	int32_t        era()       const                              {int64_t sysTime = (int64_t)_sysTime;return ((sysTime%POW2_32 < 0)?((sysTime/POW2_32)-1):(sysTime/POW2_32));}
	uint32_t       eraOffset() const                              {int64_t sysTime = (int64_t)_sysTime;int32_t result = sysTime%POW2_32;return((result<0)?(result+POW2_32):(result));}
	int64_t        secs()      const                              {return (int64_t)_sysTime;}
	uint32_t       fraction()  const                              {return (uint32_t)((_sysTime-(double)secs())*DPOW2_32);}
	double         sysTimed()  const                              {return _sysTime;}
  void           addMillis(uint32_t millis)                     {_sysTime += (double)millis/1000.0;}


/**
 *  Conversion to and from NTP time scale
 */
	Date            toDate()    const                              {return Instant::toDate(secs());}
	Time            toTime()    const                              {return Instant::toTime(secs());}

	static DInstant toDInstant(const Date& d, const Time& t)       {Instant ref = Instant::toInstant(d,t); return DInstant(ref.secs());}

/**
 * 3-way compare, return -1 for less than, 0 for equal, and +1 for greater than
 */
  int             cmp(const DInstant& rhs)                       {return((sysTimed()<rhs.sysTimed())?(-1):((sysTimed()>rhs.sysTimed())?(1):(0)));} 

  static void     printDateTime(const DInstant& ref, char buffer[], unsigned int buffLen);
  static void     printTime(const DInstant& ref, char buffer[], unsigned int buffLen);
  static void     printDate(const DInstant& ref, char buffer[], unsigned int buffLen);

/**
 *  Convenience method to print the contents of an DInstant to Serial with an index argument.
 *  Useful in printing multiple DInstants as:
 *    T1: ...
 *    T2: ...
 *  and so on.
 */
  static void    printT(int i, DInstant t) {Serial.printf("T%d:  era = %d eraOffset = %u secs = %lld fraction = %u sysTimed = %f\n",i,t.era(),t.eraOffset(),t.secs(),t.fraction(),t.sysTimed());}

/**
 *  Templated version to print multiple DInstants as
 *    T1: era = %d eraOffset = %u secs = %lld fraction = %u sysTimed = %f\n
 *    ...
 *    Tn: era = %d eraOffset = %u secs = %lld fraction = %u sysTimed = %f\n
 */
  template<typename T>
  static void    printTs(int i,T t) {printT(i,t);}

  template<typename T, typename... Args>
  static void    printTs(int i,T t,Args... args) {printTs(i++,t);printTs(i,args...);}

/**
*   Print Date/Time to Serial as Month Day,Year hour:min:sec
*/
  static void    printDateTime(DInstant t);

/**
 *  Templated version to print Date/Time for multiple Instances as
 *    Month day, year hh:mm:ss
 *    ...
 *    Month day, year hh:mm:ss
 */
  template<typename T>
  static void    printDateTimes(T t) {printDateTime(t);}

  template<typename T, typename... Args>
  static void    printDateTimes(T t, Args... args) {printDateTimes(t);printDateTimes(args...);}

/**
 *  Arithmetic Operators
 */
  friend DInstant operator+(DInstant lhs,const DInstant& rhs)          {lhs += rhs;return lhs;}
  friend DInstant operator-(DInstant lhs,const DInstant& rhs)          {lhs -= rhs;return lhs;}
  DInstant&       operator+=(const DInstant& rhs)                      {_sysTime += rhs.sysTimed(); return *this;}
  DInstant&       operator-=(const DInstant& rhs)                      {*this += -rhs;return *this;}
  DInstant        operator/(const int denom) const                     {DInstant result(sysTimed()/(double)denom); return result;}
  DInstant&       operator+=(const int rhs)                            {_sysTime += rhs;return *this;}
  DInstant&       operator-=(const int rhs)                            {_sysTime -= rhs;return *this;}
  friend DInstant operator+(const DInstant& lhs, const int rhs)        {DInstant result = lhs;result._sysTime += rhs;return result;}
  friend DInstant operator+(const int lhs, const DInstant& rhs)        {return rhs+lhs;}
  friend DInstant operator-(const DInstant& lhs, const int rhs)        {DInstant result = lhs;result._sysTime -= rhs;return result;}
  DInstant        operator-() const                                    {DInstant result(-sysTimed());return result;}

  friend DInstant abs(const DInstant& ref)                             {return((ref.sysTimed()<0)?(-ref):ref);}

/**
 *  Increment/Decrement Operators
 */
  DInstant&       operator++()        {_sysTime++;return *this;}                          // Prefix increment, add 1 second to sysTime
  DInstant        operator++(int)     {DInstant old = *this; operator++(); return old;}   // Postfix increment, add 1 second to sysTime
  DInstant&       operator--()        {_sysTime--;return *this;}                          // Prefix decrement, subtract 1 second from sysTime
  DInstant        operator--(int)     {DInstant old;operator--();return old;}             // Postfix dectement, subtract 1 second from sysTime

  inline bool operator==(const DInstant& rhs) { return cmp(rhs) == 0; }
  inline bool operator!=(const DInstant& rhs) { return cmp(rhs) != 0; }
  inline bool operator< (const DInstant& rhs) { return cmp(rhs) <  0; }
  inline bool operator> (const DInstant& rhs) { return cmp(rhs) >  0; }
  inline bool operator<=(const DInstant& rhs) { return cmp(rhs) <= 0; }
  inline bool operator>=(const DInstant& rhs) { return cmp(rhs) >= 0; }

  private:

	double         _sysTime;

};

} // End of namespace lsc

#endif
