/* SPDX-License-Identifier: MIT */
/**
	@file		performance.cpp
	@brief		Monitors the operational performance, timing, and stats of an arbitrary module.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/common/performance.h"
#include "ajabase/system/debug.h"

#include <iomanip>
#include <sstream>
#include <math.h>

using std::string;
using std::map;

/////////////////////////////
// Definitions
/////////////////////////////

// Fix for old Linux systems
#ifndef UINT64_MAX
#define UINT64_MAX      18446744073709551615
#endif

AJAPerformance::AJAPerformance(const std::string& name,
                               AJATimerPrecision precision,
                               uint64_t skipEntries)
    : mTimer(precision)
{
    mName       = name;
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
    mMean       = 0.0;
    mM2         = 0.0;
    mNumEntriesToSkipAtStart = skipEntries;
}

AJAPerformance::AJAPerformance(const std::string& name,
                               const AJAPerformanceExtraMap &values,
                               AJATimerPrecision precision,
                               uint64_t skipEntries)
    : mTimer(precision)
{
    mName       = name;
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
    mExtras     = values;
    mMean       = 0.0;
    mM2         = 0.0;
    mNumEntriesToSkipAtStart = skipEntries;
}

AJAPerformance::AJAPerformance(AJATimerPrecision precision,
                               uint64_t skipEntries)
    : mTimer(precision)
{
    mName       = "";
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
    mMean       = 0.0;
    mM2         = 0.0;
    mNumEntriesToSkipAtStart = skipEntries;
}

AJAPerformance::~AJAPerformance(void)
{
    // If not already stopped then stop and output report
    if (mTimer.IsRunning())
    {
        Stop();
        Report();
    }
}

void AJAPerformance::SetExtras(const AJAPerformanceExtraMap& values)
{
    mExtras = values;
}

std::string AJAPerformance::Name(void)
{
    return mName;
}

uint64_t AJAPerformance::Entries(void)
{
    return mEntries;
}

uint64_t AJAPerformance::TotalTime(void)
{
    return mTotalTime;
}

uint64_t AJAPerformance::MinTime(void)
{
    return mMinTime;
}

uint64_t AJAPerformance::MaxTime(void)
{
    return mMaxTime;
}

double AJAPerformance::Mean(void)
{
    return mMean;
}

double AJAPerformance::Variance(void)
{
    uint64_t entries = Entries();
    if (entries > 1)
    {
        return mM2/(entries-1);
    }
    else
        return 0.0;
}

double AJAPerformance::StandardDeviation(void)
{
    return sqrt(Variance());
}

const AJAPerformanceExtraMap AJAPerformance::Extras(void)
{
    return mExtras;
}

AJATimerPrecision AJAPerformance::Precision(void)
{
    return mTimer.Precision();
}

void AJAPerformance::Start(void)
{
	mTimer.Start();
}

void AJAPerformance::Stop(void)
{
	uint32_t elapsedTime;

	mTimer.Stop();
	elapsedTime = mTimer.ElapsedTime();

    // Honor the number of entries asked to skip at start
    if (mNumEntriesToSkipAtStart > 0)
    {
        mNumEntriesToSkipAtStart--;
        return;
    }

    mTotalTime += elapsedTime;
    mEntries++;

    // calculate the running mean and sum of squares of differences from the current mean (mM2)
    // mM2 is needed to calculate the variance and the standard deviation
    // see: https://stackoverflow.com/a/17053010
    //      http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
    double delta1 = elapsedTime - mMean;
    mMean += delta1/mEntries;
    double delta2 = elapsedTime - mMean;
    mM2 += delta1 * delta2;

    if (elapsedTime > mMaxTime)
	{
        mMaxTime = elapsedTime;

		// First-time assignment in the event that a
		// test run has times that always monotonically
		// increase.
        if (elapsedTime < mMinTime)
		{
            mMinTime = elapsedTime;
		}
	}
    else if (elapsedTime < mMinTime)
	{
        mMinTime = elapsedTime;
	}
}

void AJAPerformance::Report(const std::string& name, const char* pFileName, int32_t lineNumber)
{
    int entries = (int)Entries();
    if (entries > 0)
    {
        int min      = (int)MinTime();
        int max      = (int)MaxTime();
        double mean  = Mean();
        double stdev = StandardDeviation();
        string times = (entries == 1) ? "time,  " : "times, ";
        string reportName = name.empty() ? Name() : name;

        std::ostringstream oss;
        oss << "  ["     << std::left  << std::setw(23) << std::setfill(' ') << reportName << "] " <<
               "called " << std::right << std::setw(4)  << entries << " "  << times <<
               "min: "   << std::right << std::setw(4)  << min     << ", " <<
               "mean: "  << std::right << std::setw(5)  << std::fixed << std::setprecision(2) << mean  << ", " <<
               "stdev: " << std::right << std::setw(5)  << std::fixed << std::setprecision(2) << stdev << ", " <<
               "max: "   << std::right << std::setw(4)  << max;

        AJADebug::Report(AJA_DebugUnit_StatsGeneric,
                         AJA_DebugSeverity_Debug,
                         pFileName == NULL ? __FILE__ : pFileName,
                         lineNumber < 0 ? __LINE__ : lineNumber,
                         oss.str());
    }
}

bool AJAPerformanceTracking_start(AJAPerformanceTracking& stats,
                                 std::string key, AJATimerPrecision precision, uint64_t skipEntries)
{
    if(stats.find(key) == stats.end())
    {
        // not already in map
        AJAPerformance newStatsGroup(key, precision, skipEntries);
        stats[key] = newStatsGroup;
    }

    AJAPerformanceTracking::iterator foundAt = stats.find(key);
    if(foundAt != stats.end())
    {
        foundAt->second.Start();
        return true;
    }
    else
    {
        return false;
    }
}

bool AJAPerformanceTracking_start(AJAPerformanceTracking& stats,
                                 std::string key, const AJAPerformanceExtraMap& extras, AJATimerPrecision precision,
                                 uint64_t skipEntries)
{
    if(stats.find(key) == stats.end())
    {
        // not already in map
        AJAPerformance newStatsGroup(key, extras, precision, skipEntries);
        stats[key] = newStatsGroup;
    }

    AJAPerformanceTracking::iterator foundAt = stats.find(key);
    if(foundAt != stats.end())
    {
        foundAt->second.Start();
        return true;
    }
    else
    {
        return false;
    }
}

bool AJAPerformanceTracking_stop(AJAPerformanceTracking& stats, std::string key)
{
    AJAPerformanceTracking::iterator foundAt = stats.find(key);
    if(foundAt != stats.end())
    {
        foundAt->second.Stop();
        return true;
    }
    else
    {
        return false;
    }
}

bool AJAPerformanceTracking_report(AJAPerformanceTracking& stats, std::string title, const char *pFileName, int32_t lineNumber)
{
    const int32_t unit = (int32_t)AJA_DebugUnit_StatsGeneric;
    const int32_t severity = (int32_t)AJA_DebugSeverity_Debug;

    if (stats.size() > 0)
    {
        if (title.empty())
        {
            title = "stats_report";
        }

        string units = AJATimer::PrecisionName(stats.begin()->second.Precision(), true);

        std::ostringstream title_oss;
        std::ostringstream units_oss;
        std::ostringstream footer_oss;
        title_oss << title << ", tracking " << stats.size() << " {";
        units_oss << "  * time units are in " << units << " *";
        footer_oss << "}";

        AJADebug::Report(unit, severity, pFileName, lineNumber, title_oss.str());
        AJADebug::Report(unit, severity, pFileName, lineNumber, units_oss.str());

        AJAPerformanceTracking::iterator foundAt = stats.begin();
        while (foundAt != stats.end())
        {
            std::string key = foundAt->first;
            AJAPerformance perf = foundAt->second;

            perf.Report(key, pFileName, lineNumber);

            ++foundAt;
        }

        AJADebug::Report(unit, severity, pFileName, lineNumber, footer_oss.str());
        return true;
    }
    else
    {
        return false;
    }
}
