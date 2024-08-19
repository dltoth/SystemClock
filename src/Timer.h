
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

#ifndef TIMER_H
#define TIMER_H
#include <Arduino.h>
#include <ctype.h>
#include <functional>

typedef std::function<void(void)> TimerCallback;


/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

/** Timer class
 *  Measure elapsed time or trigger a unit of work after some interval. The unit of work is performed by a handler function (TimerCallback)
 *  that executes once when the Timer's setPoint has expired. The unit of work can be made perpetual by calling Timer.start() from within
 *  the handler.
 *  Once Timer is started it can be stopped and started again, or paused for a duration.
 *  The following methods are supported:
 *     void          start()                        // Start the Timer; if Timer is paused, pause is cancelled
 *     bool          started()                      // Returns true if Timer started
 *     bool          stopped()                      // Returns true if Timer is stopped
 *     void          stop()                         // Stop Timer; Timer remains stopped until start() is called to run out remaining time
 *     void          reset()                        // Stop Timer and reset internal counters
 *     void          clear()                        // Reset Timer and clear set point
 *     void          set(int h, int m, int s)       // Set duration interval for Timer to run, hours, minutes, and seconds
 *     void          set(unsigned long millis)      // Set duration in milliseconds
 *     unsigned long elapsedTimeMillis()            // Elapsed time in milliseconds since last Timer start()
 *     unsigned long elapsedTimeSeconds()           // Elapsed time in seconds since last Timer start()
 *     void          setHandler(TimerCallback h)    // Set Timer callback handler
 *     unsigned long setPointMillis()               // Return Timer duration in milliseconds
 *     unsigned long limit()                        // If Timer is started, returns point at which Timer expires in milliseconds, otherwise returns 0
 *     void          pause(unsigned long duration)  // Pause Timer for duration milliseconds; stops Timer until duration expires or start() is called
 *     void          cancelPause()                  // Cancel an active pause and start Timer
 *     bool          paused()                       // Return true if Timer is paused
 *     bool          pauseLimit()                   // If Timer is paused, returns point at which pause expires in milliseconds
 *     void          run()                          // Execute callback handler
 *     void          doDevice()                     // Called in Arduino loop() function to update internal counters, potentially calling callback
 *
 *  Example:
 *  SystemClock c;
 *  Timer t = Timer();
 *        t.set(0,0,15);        // Set an interval of 15 seconds
 *        t.setHandler([this]{  // Set up a callback to print current time (in UTC) and reset the timer
 *            Instant::printDateTime(c.now());
 *            t.start();
 *           });
 *        t.start();
 * 
 *   Note: 
 *      1. Timer.doDevice() must be called from within the application loop.
 *      2. Timer.reset() is called prior to handler invocation so the handler will not be called
 *         again unless Timer.start() is called within the handler.
 *
 */
class Timer {
  public:
  Timer() {}
  Timer( const Timer& t );

  void          start()                        {if(stopped()) {_millis = millis();_pauseMillis=0;_pauseLimit=0;_limit=_millis+_stoppage;}} 
  bool          started()                      {return _millis != 0;}
  bool          stopped()                      {return !started();}
  void          stop()                         {if(started()) {_millis=0;unsigned long current = millis();_stoppage=((current<_limit)?(_limit-current):(0));_limit=0;}}
  void          reset()                        {_millis=0;_stoppage=_setPoint;_limit=0;_pauseLimit=0;_pauseMillis=0;}
  void          clear()                        {reset();_setPoint=0;_stoppage=0;}
  void          set(int h, int m, int s)       {h=((h<0)?(0):(h));m=((m<0)?(0):(m));s=((s<0)?(0):(s));_setPoint = 1000*s + 60000*m + 3600000*h;_stoppage=_setPoint;}
  void          set(unsigned long millis)      {_setPoint = millis;_stoppage = _setPoint;}
  unsigned long elapsedTimeMillis()            {return((_millis>0)?(millis() - _millis):(0));}     // once timer starts, _millis > 0 and _millis < millis()
  unsigned long elapsedTimeSeconds()           {return elapsedTimeMillis()/1000;}
  void          setHandler(TimerCallback h)    {if(h != NULL) _handler=h;else _handler=([]{});}
  unsigned long setPointMillis()               {return _setPoint;}
  unsigned long limit()                        {return _limit;}                                    // limit is 0 if not started
  void          pause(unsigned long duration)  {if(!paused()) {stop();_pauseMillis=millis();_pauseLimit=_pauseMillis+duration;}}
  void          cancelPause()                  {if(paused()) start();}
  bool          paused()                       {return _pauseMillis != 0;}
  unsigned long pauseLimit()                   {return _pauseLimit;}
  void          run()                          {_handler();}
  void          doDevice();

/**
  void          setPoint(int& h, int& m, int& s);
**/
  
  private:
  unsigned long      _millis      = 0;           // Millisecond timestamp of the Timer start
  unsigned long      _setPoint    = 0;           // Millisecond duration set
  unsigned long      _limit       = 0;           // Millisecond endpoint of Timer run
  unsigned long      _stoppage    = 0;           // Remaining milliseconds prior to start(), set at last stop()
  unsigned long      _pauseMillis = 0;           // Millisecond timestamp at pause
  unsigned long      _pauseLimit  = 0;           // Millisecond endpoint of pause
  TimerCallback      _handler     = ([]{});      // Unit of work to be done when Timer expires
};


} // End of namespace lsc

#endif