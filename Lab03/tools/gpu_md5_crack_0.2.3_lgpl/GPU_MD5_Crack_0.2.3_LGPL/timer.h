
/* 
    Copyright (C) 2009  Benjamin Vernoux, titanmkd@gmail.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */
	
#ifndef TIMING_H
#define TIMING_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "global_types.h"

	class Timer {
	public:
		Timer(void);
		~Timer(void);
		int64 StartTiming(void);
		int64 StopTiming(void);
		double ElapsedTiming(int64 start, int64 stop);
	private:
#if (defined(WIN32) || defined(__WIN32__) || defined(__WIN32))
		int64 m_Freq;   // Frequency for QueryPerformanceFrequency
#endif
	};

#ifdef  __cplusplus
}
#endif

#endif /* TIMING_H */
