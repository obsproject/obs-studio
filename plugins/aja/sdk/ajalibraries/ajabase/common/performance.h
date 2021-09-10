/* SPDX-License-Identifier: MIT */
/**
	@file		performance.h
	@brief		Declaration of the AJAPerformance class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PERFORMANCE_H
#define AJA_PERFORMANCE_H

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/common/timer.h"
#include <string>
#include <map>

typedef std::map<std::string, uint64_t> AJAPerformanceExtraMap;

/////////////////////////////
// Declarations
/////////////////////////////
class AJAExport AJAPerformance
{
	public:
        /**
         *	Constructor
         *
         *  @param[in]  name The name for the performance object.
         *  @param[in]  precision The Precision units to use.
         *  @param[in]  skipEntries The number of entries to skip over before tracking performance,
         *                          the idea here is to skip over any "warm-up" period.
         */
        AJAPerformance(const std::string& name,
                       AJATimerPrecision precision = AJATimerPrecisionMilliseconds,
                       uint64_t skipEntries = 0);

        /**
         *	Constructor
         *
         *  @param[in]  name The name for the performance object.
         *  @param[in]  values Extra values that can be stored along with performance info.
         *  @param[in]  precision The Precision units to use.
         *  @param[in]  skipEntries The number of entries to skip over before tracking performance,
         *                          the idea here is to skip over any "warm-up" period.
         */
        AJAPerformance(const std::string& name, const AJAPerformanceExtraMap& values,
                       AJATimerPrecision precision = AJATimerPrecisionMilliseconds,
                       uint64_t skipEntries = 0);

        /**
         *	Constructor
         *
         *  @param[in]  precision The Precision units to use.
         *  @param[in]  skipEntries The number of entries to skip over before tracking performance,
         *                          the idea here is to skip over any "warm-up" period.
         */
        AJAPerformance(AJATimerPrecision precision = AJATimerPrecisionMilliseconds,
                       uint64_t skipEntries = 0);

		~AJAPerformance(void);

        /**
         *	Set extra values that can be stored along with performance info
         *
         *  @param[in]  values The extra values to assign to this object
         */
        void SetExtras(const AJAPerformanceExtraMap& values);

        /**
         *	Start the timer of the performance object
         */
		void Start(void);

        /**
         *	Stop the timer of the performance object and updates the performance stats:
         *  number of entries, min time, max time, total time
         */
		void Stop(void);

        /**
         *	Print out a performance report to AJADebug
         *  @param[in]  name Name to use in printout, if empty will use the name passed in constructor
         *	@param[in]	pFileName	The source filename reporting the performace.
         *	@param[in]	lineNumber	The line number in the source file reporting the performance.
         */
        void Report(const std::string& name = "", const char *pFileName = NULL, int32_t lineNumber = -1);

        /**
         *  Returns the name for the performance object that was set in the constructor
         */
        std::string Name(void);

        /**
         *  Returns the Precision units set in the constructor
         */
        AJATimerPrecision Precision(void);

        /**
         *  Returns the number of times that the start/stop pair has been called
         */
        uint64_t Entries(void);

        /**
         *  Returns the total elapsed time between all start/stop pairs (in Precision units)
         */
        uint64_t TotalTime(void);

        /**
         *  Returns the minimum time between all start/stop pairs (in Precision units)
         */
        uint64_t MinTime(void);

        /**
         *  Returns the maximum time between all start/stop pairs (in Precision units)
         */
        uint64_t MaxTime(void);

        /**
         *  Returns the mean (average) time of all start/stop pairs (in Precision units)
         */
        double Mean(void);

        /**
         *  Returns the variance of all start/stop pairs (in Precision units)
         */
        double Variance(void);

        /**
         *  Returns the standard deviation of all start/stop pairs (in Precision units)
         */
        double StandardDeviation(void);

        /**
         *  Returns a map of any extra values stored in the performance object
         */
        const AJAPerformanceExtraMap Extras(void);

    private:
        AJATimer                    mTimer;
        std::string                 mName;
        uint64_t                    mTotalTime;
        uint64_t                    mEntries;
        uint64_t                    mMinTime;
        uint64_t                    mMaxTime;
        double                      mMean;
        double                      mM2;
        uint64_t                    mNumEntriesToSkipAtStart;

        AJAPerformanceExtraMap      mExtras;
};

// Helper functions to track/report many performance timers and store in a map
typedef std::map<std::string, AJAPerformance> AJAPerformanceTracking;

extern bool AJAPerformanceTracking_start(AJAPerformanceTracking& stats,
                                        std::string key,
                                        AJATimerPrecision precision = AJATimerPrecisionMilliseconds,
                                        uint64_t skipEntries = 0);

extern bool AJAPerformanceTracking_start(AJAPerformanceTracking& stats,
                                        std::string key, const AJAPerformanceExtraMap& extras,
                                        AJATimerPrecision precision = AJATimerPrecisionMilliseconds,
                                        uint64_t skipEntries = 0);

extern bool AJAPerformanceTracking_stop(AJAPerformanceTracking& stats, std::string key);

extern bool AJAPerformanceTracking_report(AJAPerformanceTracking& stats, std::string title = "",
                                         const char *pFileName = NULL, int32_t lineNumber = -1);


#endif // AJA_PERFORMANCE_H
//////////////////////// End of performance.h ///////////////////////

