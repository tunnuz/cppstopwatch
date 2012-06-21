/*
This file is part of Stopwatch, a project by Tommaso Urli.

Stopwatch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Stopwatch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stopwatch.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Stdafx.h"

#ifndef WIN32
	#include <sys/time.h>
#else
	#include <Windows.h>
	#include <iomanip>
#endif

#include "Stopwatch.h"

using std::map;
using std::string;
using std::ostringstream;

Stopwatch::Stopwatch() : active(true), mode(NONE)  {
	records_of = new map<string, PerformanceData>();
}

Stopwatch::~Stopwatch() {
	delete records_of;
}

void Stopwatch::set_mode(StopwatchMode new_mode) {
	mode = new_mode;
}

bool Stopwatch::performance_exists(string perf_name) {
	return (records_of->find(perf_name) != records_of->end());
}

long double Stopwatch::take_time() {

	if ( mode == CPU_TIME ) {
		
		// Use ctime
		return clock();
		
	} else if ( mode == REAL_TIME ) {
		
		// Query operating system
		
#ifdef WIN32
		/*	In case of usage under Windows */
		FILETIME ft;
		LARGE_INTEGER intervals;

		// Get the amount of 100 nanoseconds intervals elapsed since January 1, 1601 (UTC)
		GetSystemTimeAsFileTime(&ft);
		intervals.LowPart = ft.dwLowDateTime;
		intervals.HighPart = ft.dwHighDateTime;

		long double measure = intervals.QuadPart;
		measure -= 116444736000000000.0;			// Convert to UNIX epoch time
		measure /= 10000000.0;						// Convert to seconds

		return measure;
#else
		/* Linux, MacOS, ... */
		struct timeval tv;
		gettimeofday(&tv, NULL);

		long double measure = tv.tv_usec;
		measure /= 1000000.0;						// Convert to seconds
		measure += tv.tv_sec;						// Add seconds part

		return measure;
#endif

	} else {
		// If mode == NONE, clock has not been initialized, then throw exception
		throw StopwatchException("Clock not initialized to a time taking mode!");
	}
}

void Stopwatch::start(string perf_name)  {

	if (!active) return;

	// Just works if not already present
	records_of->insert(make_pair(perf_name, PerformanceData()));

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	// Take ctime
	perf_info.clock_start = take_time();

	// If this is a new start (i.e. not a restart)
	if (!perf_info.paused)
		perf_info.last_time = 0;

	perf_info.paused = false;
}

void Stopwatch::stop(string perf_name) {
	
	if (!active) return;
	
	long double clock_end = take_time();
	
	// Try to recover performance data
	if ( !performance_exists(perf_name) )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;
	
	perf_info.stops++;
	long double  lapse = clock_end - perf_info.clock_start;
	
	if ( mode == CPU_TIME )
		lapse /= (double) CLOCKS_PER_SEC;
	
	// Update last time
	perf_info.last_time = lapse;

	// Update min/max time
	if ( lapse >= perf_info.max_time )	perf_info.max_time = lapse;
	if ( lapse <= perf_info.min_time || perf_info.min_time == 0 )	perf_info.min_time = lapse;
	
	// Update total time
	perf_info.total_time += lapse;
}

void Stopwatch::pause(string perf_name) {
	
	if (!active) return;
	
	long double  clock_end = clock();
	
	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;
	
	long double  lapse = clock_end - perf_info.clock_start;
	
	// Update total time
	perf_info.last_time += lapse;
	perf_info.total_time += lapse;
}

void Stopwatch::reset_all() {
	
	if (!active) return;

	map<string, PerformanceData>::iterator it;
	
	for (it = records_of->begin(); it != records_of->end(); ++it) {
		reset(it->first);
	}
}

void Stopwatch::report_all(std::ostream& output) {

	if (!active) return;

	map<string, PerformanceData>::iterator it;
	
	for (it = records_of->begin(); it != records_of->end(); ++it) {
		report(it->first, output);
	}
}

void Stopwatch::reset(string perf_name) {

	if (!active) return;

	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;
	
	perf_info.clock_start = 0;
	perf_info.total_time = 0;
	perf_info.min_time = 0;
	perf_info.max_time = 0;
	perf_info.last_time = 0;
	perf_info.paused = false;
	perf_info.stops = 0;
}

void Stopwatch::turn_on() {
	std::cout << "Stopwatch active." << std::endl;
	active = true;
}

void Stopwatch::turn_off() {
	std::cout << "Stopwatch inactive." << std::endl;
	active = false;
}

void Stopwatch::report(string perf_name, std::ostream& output) {

	if (!active) return;
	
	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;

	// To support Windows (otherwise string("=", perf_name.length() + 1) would do the job
	string bar = "";
	for (int i = 0; i < perf_name.length(); i++)
		bar.append("=");

	output << std::endl;
	output << "======================" << bar << std::endl;
	output << "Tracking performance: " << perf_name << std::endl;
	output << "======================" << bar << std::endl;
	output << "  *  Avg. time " << (perf_info.total_time / (long double) perf_info.stops) << " sec" << std::endl;
	output << "  *  Min. time " << (perf_info.min_time) << " sec" << std::endl;
	output << "  *  Max. time " << (perf_info.max_time) << " sec" << std::endl;
	output << "  *  Tot. time " << (perf_info.total_time) << " sec" << std::endl;

	ostringstream stops;
	stops << perf_info.stops;

	output << "  *  Stops " << stops.str() << std::endl;
	output << std::endl;
	
}

long double Stopwatch::get_time_so_far(string perf_name) {
    // Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");
    
    long double lapse = (take_time() - (records_of->find(perf_name)->second).clock_start);
    
    if (mode == CPU_TIME)
        lapse /= (double) CLOCKS_PER_SEC;
    
	return lapse;
}

long double Stopwatch::get_total_time(string perf_name) {

	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	return perf_info.total_time;

}

long double Stopwatch::get_average_time(string perf_name) {
	
	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	return (perf_info.total_time / (long double)perf_info.stops);

}

long double Stopwatch::get_min_time(string perf_name) {
	
	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	return perf_info.min_time;

}

long double Stopwatch::get_max_time(string perf_name) {
	
	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	return perf_info.max_time;

}

long double Stopwatch::get_last_time(string perf_name) {

	// Try to recover performance data
	if ( !performance_exists(perf_name)  )
		throw StopwatchException("Performance not initialized.");

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	return perf_info.last_time;

}
