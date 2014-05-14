/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: timer.h,v 1.7 2002/08/20 06:31:17 taku-ku Exp $;

 Copyright (C) 2001-2002  Taku Kudo <taku-ku@is.aist-nara.ac.jp>
 All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later verjsion.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
*/


#ifndef _TIMER_H
#define _TIMER_H

// $Id: timer.h,v 1.7 2002/08/20 06:31:17 taku-ku Exp $;

namespace TinySVM {

class Timer
{
private:
  char buf[32];
  long start_clock;
  long end_clock;

#ifndef _WIN32
  struct tms start_tms;
  struct tms end_tms;
#endif
   
public:

#ifdef _WIN32
  void start() { start_clock = GetTickCount()/10; };
  void end()   { end_clock   = GetTickCount()/10; };
#else

  void start() 
  { 
    times(&start_tms); 
    start_clock = (start_tms.tms_utime + start_tms.tms_stime); 
  };
  
  void end()   
  { 
    times(&end_tms);
    end_clock = (end_tms.tms_utime + end_tms.tms_stime);
  };
#endif

  char *getDiff()
  {
    this->end();
    int t = (end_clock - start_clock)/100;
    int sec, min, hour;
    sec  = t % 60;
    min  = (t / 60) % 60;
    hour = t / 3600;
    sprintf(buf,"%02d:%02d:%02d", hour, min, sec);
    return buf;
  }

  Timer() 
  {
    this->start();
  }
};

};
#endif

