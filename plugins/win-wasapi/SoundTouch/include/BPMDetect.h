////////////////////////////////////////////////////////////////////////////////
///
/// Beats-per-minute (BPM) detection routine.
///
/// The beat detection algorithm works as follows:
/// - Use function 'inputSamples' to input a chunks of samples to the class for
///   analysis. It's a good idea to enter a large sound file or stream in smallish
///   chunks of around few kilosamples in order not to extinguish too much RAM memory.
/// - Input sound data is decimated to approx 500 Hz to reduce calculation burden,
///   which is basically ok as low (bass) frequencies mostly determine the beat rate.
///   Simple averaging is used for anti-alias filtering because the resulting signal
///   quality isn't of that high importance.
/// - Decimated sound data is enveloped, i.e. the amplitude shape is detected by
///   taking absolute value that's smoothed by sliding average. Signal levels that
///   are below a couple of times the general RMS amplitude level are cut away to
///   leave only notable peaks there.
/// - Repeating sound patterns (e.g. beats) are detected by calculating short-term 
///   autocorrelation function of the enveloped signal.
/// - After whole sound data file has been analyzed as above, the bpm level is 
///   detected by function 'getBpm' that finds the highest peak of the autocorrelation 
///   function, calculates it's precise location and converts this reading to bpm's.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _BPMDetect_H_
#define _BPMDetect_H_

#include <vector>
#include "STTypes.h"
#include "FIFOSampleBuffer.h"

namespace soundtouch
{

    /// Minimum allowed BPM rate. Used to restrict accepted result above a reasonable limit.
    #define MIN_BPM 45

    /// Maximum allowed BPM rate range. Used for calculating algorithm parametrs
    #define MAX_BPM_RANGE 200

    /// Maximum allowed BPM rate range. Used to restrict accepted result below a reasonable limit.
    #define MAX_BPM_VALID 190

////////////////////////////////////////////////////////////////////////////////

    typedef struct
    {
        float pos;
        float strength;
    } BEAT;


    class IIR2_filter
    {
        double coeffs[5];
        double prev[5];

    public:
        IIR2_filter(const double *lpf_coeffs);
        float update(float x);
    };


    /// Class for calculating BPM rate for audio data.
    class BPMDetect
    {
    protected:
        /// Auto-correlation accumulator bins.
        float *xcorr;

        /// Sample average counter.
        int decimateCount;

        /// Sample average accumulator for FIFO-like decimation.
        soundtouch::LONG_SAMPLETYPE decimateSum;

        /// Decimate sound by this coefficient to reach approx. 500 Hz.
        int decimateBy;

        /// Auto-correlation window length
        int windowLen;

        /// Number of channels (1 = mono, 2 = stereo)
        int channels;

        /// sample rate
        int sampleRate;

        /// Beginning of auto-correlation window: Autocorrelation isn't being updated for
        /// the first these many correlation bins.
        int windowStart;

        /// window functions for data preconditioning
        float *hamw;
        float *hamw2;

        // beat detection variables
        int pos;
        int peakPos;
        int beatcorr_ringbuffpos;
        int init_scaler;
        float peakVal;
        float *beatcorr_ringbuff;

        /// FIFO-buffer for decimated processing samples.
        soundtouch::FIFOSampleBuffer *buffer;

        /// Collection of detected beat positions
        //BeatCollection beats;
        std::vector<BEAT> beats;

        // 2nd order low-pass-filter
        IIR2_filter beat_lpf;

        /// Updates auto-correlation function for given number of decimated samples that 
        /// are read from the internal 'buffer' pipe (samples aren't removed from the pipe 
        /// though).
        void updateXCorr(int process_samples      /// How many samples are processed.
        );

        /// Decimates samples to approx. 500 Hz.
        ///
        /// \return Number of output samples.
        int decimate(soundtouch::SAMPLETYPE *dest,      ///< Destination buffer
            const soundtouch::SAMPLETYPE *src, ///< Source sample buffer
            int numsamples                     ///< Number of source samples.
        );

        /// Calculates amplitude envelope for the buffer of samples.
        /// Result is output to 'samples'.
        void calcEnvelope(soundtouch::SAMPLETYPE *samples,  ///< Pointer to input/output data buffer
            int numsamples                    ///< Number of samples in buffer
        );

        /// remove constant bias from xcorr data
        void removeBias();

        // Detect individual beat positions
        void updateBeatPos(int process_samples);


    public:
        /// Constructor.
        BPMDetect(int numChannels,  ///< Number of channels in sample data.
            int sampleRate    ///< Sample rate in Hz.
        );

        /// Destructor.
        virtual ~BPMDetect();

        /// Inputs a block of samples for analyzing: Envelopes the samples and then
        /// updates the autocorrelation estimation. When whole song data has been input
        /// in smaller blocks using this function, read the resulting bpm with 'getBpm' 
        /// function. 
        /// 
        /// Notice that data in 'samples' array can be disrupted in processing.
        void inputSamples(const soundtouch::SAMPLETYPE *samples,    ///< Pointer to input/working data buffer
            int numSamples                            ///< Number of samples in buffer
        );

        /// Analyzes the results and returns the BPM rate. Use this function to read result
        /// after whole song data has been input to the class by consecutive calls of
        /// 'inputSamples' function.
        ///
        /// \return Beats-per-minute rate, or zero if detection failed.
        float getBpm();

        /// Get beat position arrays. Note: The array includes also really low beat detection values 
        /// in absence of clear strong beats. Consumer may wish to filter low values away.
        /// - "pos" receive array of beat positions
        /// - "values" receive array of beat detection strengths
        /// - max_num indicates max.size of "pos" and "values" array.  
        ///
        /// You can query a suitable array sized by calling this with NULL in "pos" & "values".
        ///
        /// \return number of beats in the arrays.
        int getBeats(float *pos, float *strength, int max_num);
    };
}
#endif // _BPMDetect_H_
