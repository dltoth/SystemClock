
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

#include "Instant.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

const char* Instant::MONTHS[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

void initializeDate(Date& date,int m, int d, int y) {
	date.month=((m<13)?((m>0)?(m):(1)):(12));
	date.year=((y>=0)?(y):(0));
	date.day=((d>0)?((d>MONTH_DAYS(date.month,date.year))?(MONTH_DAYS(date.month,date.year)):(d)):(1));
}

void initializeTime(Time& t,int h,int m, int s,uint32_t f) {
	t.hour=((h<24)?((h>=0)?(h):(0)):(23));
	t.sec=((s<59)?((s>=0)?(s):(0)):(59));
	t.min=((m<59)?((m>=0)?(m):(0)):(59));
	t.fraction=f;
}

void Instant::initialize(double sysd) {
	_sysTime = (int64_t)sysd;
	double diff = sysd-(double)_sysTime;
	_fraction = diff*POW2_32;
	if(sysd<0 && (_fraction>0)) _sysTime -=1;
}
void Instant::addMillis(uint32_t millis) {
   uint32_t mfrac = millis%1000;
   uint32_t secs  = millis/1000;
   uint64_t frac  = (mfrac*POW2_32)/1000;      // convert millis to fraction
   uint64_t fraction = frac + _fraction;
   _sysTime  += secs;
	if( fraction >= POW2_32 ) {
		_fraction = (uint32_t)(fraction - POW2_32);
		_sysTime += 1;
	}
	else _fraction = (uint32_t) fraction;
}

/**
void Instant::addmillis(uint32_t millis) {
   double    dsecs    = millis/1000.0;
   uint32_t  secs     = (uint32_t)dsecs;
   double    dfrac    = dsecs-secs;
   uint64_t  fraction = (uint64_t)(dfrac*POW2_32) + (uint64_t)_fraction;
   _sysTime  += secs;
	if( fraction >= POW2_32 ) {
		_fraction = (uint32_t)(fraction - POW2_32);
		_sysTime += 1;
	}
	else _fraction = (uint32_t) fraction;
}
**/

/**
 *   Convert the input double precision timezone offset into a proper number of seconds. A proper timezone offset has hours 
 *   between -14 and +14, and minutes either 0, 15, 30, or 45. 
 */
int32_t Instant::tzOffset( double hours ) {
  int32_t result = 0;
  int h = ((hours<=-14)?(-14):((hours>14)?(14):((int)hours)));
  double fraction = hours - h;
  if( fraction < 0 ) {
     fraction = ((fraction<=-.75)?(-.75):((fraction<=-.5)?(-.5):(-.25)));
  }
  else if (fraction > 0 ) {
     fraction = ((fraction>=.75)?(.75):((fraction>=.5)?(.5):(.25)));
  }
  result = (int32_t)(3600.0*(double)h + 3600.0*fraction);
  return result;
}

Instant& Instant::operator+=(const Instant& rhs) {
	_sysTime    = _sysTime + rhs._sysTime;
	uint64_t fraction  = (uint64_t)_fraction + (uint64_t)rhs._fraction;
	if( fraction >= POW2_32 ) {
		_fraction = (uint32_t)(fraction - POW2_32);
		_sysTime += 1;
	}
	else _fraction = (uint32_t) fraction;
  return *this;
}

Instant Instant::operator/(const int denom) const {
  double sysT = sysTimed()/(double)denom;
	Instant result(sysT);
	return result;
}

Instant Instant::operator-() const {
	Instant result;
	result._sysTime  = -_sysTime;
	result._fraction = (uint32_t)(POW2_32 - (int64_t)_fraction);
	if( result._fraction != 0 ) result._sysTime -=1;
	return result;
}

int Instant::cmp(const Instant& rhs) {
	if(_sysTime < rhs.secs()) return -1;
	else if(_sysTime > rhs._sysTime) return 1;
	else if(_fraction < rhs._fraction) return -1;
	else if(_fraction > rhs._fraction ) return 1;
	else return 0;
}

Date Instant::toDate(int64_t s) {
	Date     result;
  int64_t  sysT              = s;
	bool     beforePrimeEpoch  = (sysT<0);
  int      year              = ((beforePrimeEpoch)?(1899):(1900));

	if( beforePrimeEpoch ) {
		while( sysT <  -SECS_IN_YEAR(year) ) {sysT += SECS_IN_YEAR(year);year--;}
	}
	else {
		while( sysT >=  SECS_IN_YEAR(year) ) {sysT -= SECS_IN_YEAR(year);year++;}
	}

/**
 *  At this point year represents the year containing sysTime. If before the prime epoch, days
 *  represents the number of days before Dec 31 and sysTime represents the number of seconds from
 *  00:00:00 of year-1. If after prime epoch, days represents the number of days from Jan 1 of year
 *  and sysTime represents the number of seconds from 00:00:00 of that year.
 *  If before prime epoch, the date and time can either be computed backward from Jan 1, or forward
 *  from the seconds compliment in the year. We choose the forward computation.
 */
	int           y    = year;
	unsigned long secs = (unsigned long) ((beforePrimeEpoch)?(SECS_IN_YEAR(year)+sysT):(sysT));

/**
 *   Now secs represents the number of seconds from 00:00:00 of year so we can process in a forward
 *   manner
 */
    uint32_t days = secs/SECS_IN_DAY;

/**
 *  At this point, days represents the number of days in year y, starting with 0
 */
    int  month = 0;
  	for( ; (month<11) && (days>=MONTH_DAYS(month,y)); month++ ) {days -= MONTH_DAYS(month,y);}

    result.month = month + 1;          // Month in the year
    result.day = days + 1;             // Day in the month
    result.year = y;
	return result;
}

Time Instant::toTime(int64_t secs) {
	Time     result;
	int64_t  seconds  = secs;

/**
 *  When seconds are negative, we have to move backward from 00:00:00. It's always easier
 *  to move forward from a positive number of seconds. Start with seconds in the day.
 */
	seconds = seconds%SECS_IN_DAY;
	seconds = ((seconds<0)?(SECS_IN_DAY+seconds):(seconds));

  result.hour   = ((seconds/3600) % 24);      // Hour in the day
  result.min    = ((seconds/60) % 60);        // Minute in the hour
  result.sec    = (seconds%60);               // Second in the minute
	return result;
}

Instant Instant::toInstant(const Date& d, const Time& t) {
	Instant    result;
	bool       beforePrimeEpoch  = (d.year < 1900);
	int64_t    sysTime           = 0;
	int        year              = ((beforePrimeEpoch)?(1899):(1900));

	if( beforePrimeEpoch ) {for( ;year > d.year; year-- ) {sysTime -= SECS_IN_YEAR(year);}}
	else                   {for( ;year < d.year; year++ ) {sysTime += SECS_IN_YEAR(year);}}

/**
 *  sysTime is now seconds up to but not including year y. At this point it's easier to count seconds forward
 *  from 00:00:00 Jan 1 up to and including h:min:sec m/d. If after the prime epoch, add this number to sysTime.
 *  If prior to the prime epoch, subtract this number from the number of seconds in year y, and then subtract
 *  from sysTime.
 *  Count seconds each month up to but not including m
 */
	int       month      = 1;
	long      secsInYear = 0;
	for(;month < d.month; month++) {secsInYear += SECS_IN_MONTH(month,d.year);}

/**
 *  Count seconds in days, up to but not including d, then add seconds from h:min:sec
 */
	secsInYear += (d.day-1)*SECS_IN_DAY;
	secsInYear += t.hour*3600;
	secsInYear += t.min*60;
	secsInYear += t.sec;

	long yearsComplimentSeconds = SECS_IN_YEAR(d.year) - secsInYear;
	sysTime += ((beforePrimeEpoch)?(-yearsComplimentSeconds):(secsInYear));
	result.setSysTime(sysTime);

	return result;
}

void Instant::printDateTime(Instant ref) {
  Date d = ref.toDate();
	Time t = ref.toTime();
	printDateTime(d,t);
}

void Instant::printDateTime(const Date& d, const Time& t) {
	Serial.printf("%02d:%02d:%02d %s %d, %d\n",t.hour,t.min,t.sec,Instant::MONTHS[d.month-1],d.day,d.year);
}

void Instant::printDateTime(const Date& d, const Time& t, char buffer[], unsigned int buffLen) {
	snprintf(buffer,buffLen,"%02d:%02d:%02d %s %d, %d",t.hour,t.min,t.sec,Instant::MONTHS[d.month-1],d.day,d.year);
}

void Instant::printDateTime(char buffer[], unsigned int buffLen) const {
  Date d = toDate();
  Time t = toTime();
  Instant::printDateTime(d,t,buffer,buffLen);
}

void Instant::printTime(const Time& t, char buffer[], unsigned int buffLen) {
  snprintf(buffer,buffLen,"%02d:%02d:%02d",t.hour,t.min,t.sec);
}
void Instant::printTime(char buffer[], unsigned int buffLen) const {
  Time t = toTime();
  Instant::printTime(t,buffer,buffLen);
}

void Instant::printDate(const Date& d, char buffer[], unsigned int buffLen) {
  snprintf(buffer,buffLen,"%s %d, %d",Instant::MONTHS[d.month-1],d.day,d.year);
}

void Instant::printDate(char buffer[], unsigned int buffLen) const {
  Date d = toDate();
  Instant::printDate( d, buffer, buffLen);
}

/**
 *  Format running time into buffer as xx days hh:mm:ss
 */
void Instant::printElapsedTime(const Instant& ref, char buffer[], int size) const {
  char buff[64];
  uint64_t  runningSecs = elapsedTime(ref);
  int32_t   runningDays = runningSecs/SECS_IN_DAY;                        // Running time in days
  Time      runningTime = Instant::toTime(runningSecs%SECS_IN_DAY);       // Time after days factored out
  Instant::printTime(runningTime,buff,64);
  snprintf(buffer,size,"%d Days %s",runningDays,buff);
}


} // End of namespace lsc
