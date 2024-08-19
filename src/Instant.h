
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

#include <Arduino.h>
#include <cstdint>
#include <initializer_list>

#ifndef INSTANT_H
#define INSTANT_H

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

#define     POW2_32              4294967296LL
#define     DPOW2_32             4294967296.0
#define     SECS_IN_68_YEARS     2144448000LL   
#define     IS_LEAP_YEAR(y)      ((((((y)%400)==0) || (((y)%100)!=0)) && ((((y)%4)==0)))?(true):(false))
#define     SECS_IN_DAY          86400
#define     SECS_IN_YEAR(y)      (DAYS_IN_YEAR(y)*SECS_IN_DAY)
#define     DAYS_IN_YEAR(y)      ((IS_LEAP_YEAR(y))?(366):(365))
#define     SECS_IN_MONTH(m,y)   (MONTH_DAYS(m-1,y)*SECS_IN_DAY)
#define     MONTH_DAYS(m,y)      ((m==1)?((IS_LEAP_YEAR(y))?(daysPerMonth[m]+1):(daysPerMonth[m])):((daysPerMonth[m])))
const int   daysPerMonth[12] =   {31,28,31,30,31,30,31,31,30,31,30,31};

typedef struct Time {
	Time()                              {hour=0;min=0;sec=0;fraction=0;}
	Time(int h,int m, int s)            {hour=((h<59)?((h>=0)?(h):(0)):(59));sec=((s<59)?((s>=0)?(s):(0)):(59));min=((m<59)?((m>=0)?(m):(0)):(59));fraction=0;}
	Time(int h,int m, int s,uint32_t f) {hour=((h<24)?((h>=0)?(h):(0)):(23));sec=((s<59)?((s>=0)?(s):(0)):(59));min=((m<59)?((m>=0)?(m):(0)):(59));fraction=f;}
	Time(const Time& t)                 {hour=t.hour;min=t.min;sec=t.sec;fraction=t.fraction;}
	int hour;
	int min;
	int sec;
	uint32_t fraction;
} Time;

typedef struct Date {
	Date()                              {month=0;day=0;year=0;}
	Date(int m, int d, int y)           {month=((m<13)?((m>0)?(m):(1)):(12));year=((y>=0)?(y):(0));day=((d>0)?((d>MONTH_DAYS(month,year))?(MONTH_DAYS(month,year)):(d)):(1));}
	Date(const Date& d)                 {month=d.month;day=d.day;year=d.year;}
	int month;
	int day;
	int year;
} Date;

/**
 *   Instant is a point in time on the NTP time scale consisting of a signed 64 bit seconds (from the Prime epoch) field,
 *   and a 32 bit unsigned offset fraction. The choice to use a combination of 64 and 32 bit integers rather than double
 *   precision is two-fold: First, to maintain the same precision as NTP, and second to take advantage of the speed of
 *   integer computation over double precision. The 64-bit seconds field is a combination of 32-bit signed era and 32-bit
 *   unsigned era offset pre-computed to the sysTime as era*POW2_32+eraOffset. The unsigned offset fraction is a little
 *   tricker in the arithmetic when sysTime is negative but still only requires 32-bit addition and subtraction.
 *   Instant has a full compliment of operators to make the clock offset calculation simpler.
 *   For example, if making an NTP request and:
 *       T1 := Instant the request was sent on the client
 *       T2 := Instant the request was received on the NTP server
 *       T3 := Instant the server sends the response and
 *       T4 := Instant the client receives the response
 *   then the clock offset can be computed as:
 *       Instant clockOffset = ((T2-T1)+(T3-T4))/2;
 *   and the system time can be updated as:
 *       Instant sysTime = T4 + clockOffset;
 *
 *   Instant can be converted between NTP system time and date/time or visa versa. For example, the first second of era -1 occurs 
 *   on Dec 31, 1899 at 23:59:59, and thus era offset is 4294967295, so:
 *       Date d = {12,31,1899};
 *       Time t = {23,59,59};
 *       Instant T(d,t).secs()           = -1 or
 *       Instant(-1,4294967295,0).secs() = -1 or
 *       Instant(-1).toDate()            = {12,31,1899} and
 *       Instant(-1).toTime()            = {23,59,59}
 *
 *   Note that Instant::fraction() is the unsigned 32-bit offset from Instant::secs(), so prior to the Prime epoch, when seconds are
 *   negative, the fraction is still positive and:
 *      double Instant::sysTimed() = (double)Instant::secs()+((double)Instant::fraction()/(double)POW2_32));
 *
 */
class Instant {
  public:
	Instant()                                                     {setSysTime(0);setFraction(0);}
	Instant(const Instant& ref)                                   {_sysTime = ref._sysTime;_fraction = ref._fraction;}
	Instant(int64_t sysTime, uint32_t fraction=0)                 {setSysTime(sysTime);setFraction(fraction);}
	Instant(int32_t e, uint32_t o, uint32_t f)                    {initialize(e,o,f);}
	Instant(double sysd)                                          {initialize(sysd);}
	Instant(Date& d, Time& t)                                     {initialize(d,t);}

/**
 *   Initialization methods
 */
	void           setSysTime(int64_t secs)                       {_sysTime = secs;}
	void           setFraction(uint32_t fraction)                 {_fraction = fraction;}
	void           initialize(int32_t e,uint32_t o,uint32_t f=0)  {setSysTime(e*POW2_32+o);setFraction(f);}
	void           initialize(const Date& d, const Time& t)       {*this = Instant::toInstant(d,t);}
	void           initialize(double sysd);

	int32_t        era()                           const          {return ((_sysTime%POW2_32 < 0)?((_sysTime/POW2_32)-1):(_sysTime/POW2_32));}
	uint32_t       eraOffset()                     const          {int32_t result = _sysTime%POW2_32;return((result<0)?(result+POW2_32):(result));}
	int64_t        secs()                          const          {return _sysTime;}
	uint32_t       fraction()                      const          {return _fraction;}
	double         sysTimed()                      const          {return((double)_sysTime+((double)_fraction/(double)POW2_32));}
  uint64_t       elapsedTime(const Instant& t)   const          {return abs(*this-t).secs();}                              
  Instant        toTimezone(double hours)                       {return *this + tzOffset(hours);}   
  void           addMillis(uint32_t millis);

/**
 *  Conversion to and from NTP time scale
 */
	Date           toDate() const                                 {return Instant::toDate(secs());}
	Time           toTime() const                                 {return Instant::toTime(secs());}

/**
 *   Formating and printing
 */
  void           printDateTime(char buffer[], unsigned int buffLen)           const;   // Format Instant to Date/Time into input buffer
  void           printTime(char buffer[], unsigned int buffLen)               const;   // Format Instant to Time only into input buffer
  void           printDate(char buffer[], unsigned int buffLen)               const;   // Format Instant to Date only into inpur buffer
  void           printElapsedTime(const Instant& ref,char buffer[], int size) const;   // Print elapsed time between this Instant and ref as xx Days, hh:mm:ss

  static const char* MONTHS[12];

  static int32_t tzOffset(double hours);                                                             // Convert the input timezone into a valid timezone offset seconds
	static Instant toInstant(const Date& d, const Time& t);                                            // Convert date and time to Instant
  static Date    toDate(int64_t secs);                                                               // Convert seconds since (0h jan 1, 1900) to Date
  static Time    toTime(int64_t secs);                                                               // Convert seconds since (0h jan 1, 1900) to Time
  static void    printDateTime(const Date& d, const Time& t);                                        // Print Date/Time to Serial as Month day, year hh:mm:ss
  static void    printDateTime(const Date& d, const Time& t, char buffer[], unsigned int buffLen);   // Format Date/Time to input buffer
  static void    printTime(const Time& t, char buffer[], unsigned int buffLen);                      // Format Time into input buffer
  static void    printDate(const Date& d, char buffer[], unsigned int buffLen);                      // Format Date into input buffer

/**
 *  Convenience method to print the contents of an Instant to Serial with an index argument.
 *  Useful in printing multiple Instants as:
 *    T1: ...
 *    T2: ...
 *  and so on.
 */
  static void    printT(int i, Instant t) {Serial.printf("T%d:  era = %ld eraOffset = %lu secs = %lld fraction = %lu sysTimed = %f\n",i,t.era(),t.eraOffset(),t.secs(),t.fraction(),t.sysTimed());}

/**
 *  Templated version to print multiple Instants as
 *    T1: era = %d eraOffset = %u secs = %lld fraction = %u sysTimed = %f\n
 *    ...
 *    Tn: era = %d eraOffset = %u secs = %lld fraction = %u sysTimed = %f\n
 */
  template<typename T>
  static void    printTs(int i,T t) {printT(i,t);}

  template<typename T, typename... Args>
  static void    printTs(int i,T t,Args... args) {printTs(i,t);printTs(i+1,args...);}

/**
*   Print Date/Time to Serial as Month Day,Year hour:min:sec
*/
  static void    printDateTime(Instant t);

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
 *   Three-way comparison used in the operators below. Returns -1 for less than, 0 for equal, and +1 for greater than
 */
  int            cmp(const Instant& rhs);  

/**
 *  Arithmetic Operators
 */
  friend Instant operator+(Instant lhs,const Instant& rhs)           {lhs += rhs;return lhs;}
  friend Instant operator-(Instant lhs,const Instant& rhs)           {lhs -= rhs;return lhs;}
  Instant&       operator+=(const Instant& rhs);
  Instant&       operator-=(const Instant& rhs)                      {*this += -rhs;return *this;}
  Instant        operator/(const int denom) const;
  Instant&       operator+=(const int rhs)                           {_sysTime += rhs;return *this;}
  Instant&       operator-=(const int rhs)                           {_sysTime -= rhs;return *this;}
  friend Instant operator+(const Instant& lhs, const int rhs)        {Instant result = lhs;result._sysTime += rhs;return result;}
  friend Instant operator+(const int lhs, const Instant& rhs)        {return rhs+lhs;}
  friend Instant operator-(const Instant& lhs, const int rhs)        {Instant result = lhs;result._sysTime -= rhs;return result;}
  Instant        operator-() const;

  friend Instant abs(const Instant& ref)                             {return((ref.secs()<0)?(-ref):ref);}

/**
 *  Increment/Decrement Operators
 */
  Instant&       operator++()                {_sysTime++;return *this;}                          // Prefix increment, add 1 second to sysTime
  Instant        operator++(int)             {Instant old = *this;operator++();return old;}      // Postfix increment, add 1 second to sysTime
  Instant&       operator--()                {_sysTime--;return *this;}                          // Prefix decrement, subtract 1 second from sysTime
  Instant        operator--(int)             {Instant old = *this;operator--();return old;}      // Postfix dectement, subtract 1 second from sysTime

  inline bool operator==(const Instant& rhs) { return cmp(rhs) == 0; }
  inline bool operator!=(const Instant& rhs) { return cmp(rhs) != 0; }
  inline bool operator< (const Instant& rhs) { return cmp(rhs) <  0; }
  inline bool operator> (const Instant& rhs) { return cmp(rhs) >  0; }
  inline bool operator<=(const Instant& rhs) { return cmp(rhs) <= 0; }
  inline bool operator>=(const Instant& rhs) { return cmp(rhs) >= 0; }

  private:

	int64_t         _sysTime;
	uint32_t        _fraction;

  bool checkAdd(Instant ntpTime, uint32_t millis);
};

} // End of namespace lsc

#endif
