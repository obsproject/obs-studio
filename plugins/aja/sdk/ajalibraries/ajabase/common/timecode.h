/* SPDX-License-Identifier: MIT */
/**
	@file		timecode.h
	@brief		Declares the AJATimeCode class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIMECODE_H
#define AJA_TIMECODE_H
#include "ajabase/common/timebase.h"

/** \class AJATimeCode timecode.h
 *  \brief Utility class for timecodes.
 *
 *   This is a storage and conversion class for timecodes.
 */
class AJA_EXPORT AJATimeCode
{
public:
	AJATimeCode();
	AJATimeCode(uint32_t frame);
    AJATimeCode(const std::string &str, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTc=false);
    AJATimeCode(const std::string &str, const AJATimeBase& timeBase);
	AJATimeCode(const AJATimeCode& other);

	virtual	~AJATimeCode();
	
    /**
     *	Query string showing timecode for current frame count given the passed parameters.
     *
     *	@param[out]	str                         string in which to place timecode.
     *	@param[in]	timeBase  					frame rate from which to calculate string.
     *	@param[in]	bDropFrame					drop frame value for string.
     */
    void				QueryString(std::string &str, const AJATimeBase& timeBase, bool bDropFrame);
    void				QueryString(char *pString, const AJATimeBase& timeBase, bool bDropFrame);     ///< @deprecated	Use QueryString(std::string) instead.

	/**
	 *	Query SMPTE string showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[out]	pString						buffer in which to place string.
	 *	@param[in]	timeBase  					frame rate from which to calculate string.
	 *	@param[in]	bDropFrame					drop frame value for string.
	 */
	void				QuerySMPTEString(char *pString, const AJATimeBase& timeBase, bool bDropFrame);
	
	/**
	 *	Query SMPTE string byte count.
	 *
	 *	@return		number of bytes in SMPTE timecode string.
	 */
	static int          QuerySMPTEStringSize(void);
	
	/**
	 * Query frame number.
	 *
	 *	@return		frame number.
	 */
	uint32_t			QueryFrame(void) const;
	
	
	/**
	 *	Query HFR divide-by-two flag.
	 *
	 *	@return	bStdTc	 Return true when using standard TC notation for HFR (e.g 01:00:00:59 -> 01:00:00:29*), set to true by default
	 */
	bool                QueryStdTimecodeForHfr() { return m_stdTimecodeForHfr; }
	
	/**
	 *	Query hmsf values showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[out]	h						    place in which to put hours value.
	 *	@param[out]	m						    place in which to put minutes value.
	 *	@param[out]	s						    place in which to put seconds value.
	 *	@param[out]	f						    place in which to put frames value.
	 *	@param[in]	timeBase  					frame rate from which to calculate string.
	 *	@param[in]	bDropFrame					drop frame value for string.
	 */
	void				QueryHmsf(uint32_t &h, uint32_t &m, uint32_t &s, uint32_t &f, const AJATimeBase& timeBase, bool bDropFrame) const;
	
	/**
	 *	Set current frame number.
	 *
	 *	@param[in]	frame    					new frame number.
	 */
	void				Set(uint32_t frame);
	
	/**
	 *	Set current frame number.
	 *
     *	@param[in]	str                         xx:xx:xx:xx style string representing new frame number.
 	 *	@param[in]	timeBase  					frame rate associated with pString.
	 */
    void				Set(const std::string &str, const AJATimeBase& timeBase);
	
	/**
	 *	Set current frame number.  A variant which may have junk in the string.
	 *
     *	@param[in]	str                         xx:xx:xx:xx style string representing new frame number.
 	 *	@param[in]	timeBase  					frame rate associated with pString.
	 *  @param[in]  bDrop                       true if drop frame
	 */
    void				SetWithCleanup(const std::string &str, const AJATimeBase& timeBase, bool bDrop);
	
	/**
	 *	Set current frame number.
	 *
     *	@param[in]	str                         xx:xx:xx:xx style string representing new frame number.
 	 *	@param[in]	timeBase  					frame rate associated with pString.
 	 *	@param[in]	bDropFrame  				true if forcing dropframe, false otherwise.
	 */
    void				Set(const std::string &str, const AJATimeBase& timeBase, bool bDropFrame);
	
	/**
	 *	Set current frame number.
	 *
	 *	@param[in]	h    					    hours value.
	 *	@param[in]	m    					    minutes value.
	 *	@param[in]	s    					    seconds value.
	 *	@param[in]	f    					    frames value.
 	 *	@param[in]	timeBase  					frame rate associated with hmsf.
 	 *	@param[in]	bDropFrame  				true if forcing dropframe, false otherwise.
	 */
	void				SetHmsf(uint32_t h, uint32_t m, uint32_t s, uint32_t f, const AJATimeBase& timeBase, bool bDropFrame);
	
	/**
	 *	Set timecode via a SMPTE string.
	 *
	 *	@param[in]	pBufr    					pointer to string.
	 *	@param[in]	timeBase                    time base associated with string.
	 */
	void                SetSMPTEString(const char *pBufr, const AJATimeBase& timeBase);
	
	/**
	 *	Set timecode via RP188 bytes.
	 *
	 *	@param[in]	inDBB		Specifies the DBB bits of the RP188 struct.
	 *	@param[in]	inLo		Specifies the lo-order 32-bit word of the RP188 struct.
 	 *	@param[in]	inHi		Specifies the hi-order 32-bit word of the RP188 struct.
 	 *	@param[in]	inTimeBase	Specifies the time base to use.
	 */
	void                SetRP188 (const uint32_t inDBB, const uint32_t inLo, const uint32_t inHi, const AJATimeBase & inTimeBase);
	
	/**
	 *	Get RP188 register values using the given timebase, and drop frame.
	 *
	 *	@param[in]	pDbb		If non-NULL, points to the variable to receive the DBB component.
	 *	@param[in]	pLow		If non-NULL, points to the variable to receive the low byte component.
 	 *	@param[in]	pHigh		If non-NULL, points to the variable to receive the high byte component.
 	 *	@param[in]	timeBase	Specifies the AJATimeBase to use.
 	 *	@param[in]	bDrop		Specify true if forcing drop-frame;  otherwise false.
 	 *	@bug		Unimplemented.
 	 *	@todo		Needs to be implemented.
	 */
	void                QueryRP188(uint32_t *pDbb, uint32_t *pLow, uint32_t *pHigh, const AJATimeBase& timeBase, bool bDrop);
	
	/**
	 *	Get RP188 register values using the given timebase, and drop frame.
	 *
	 *	@param[out]	outDBB		Receives the DBB component.
	 *	@param[out]	outLo		Receives the low byte component.
 	 *	@param[out]	outHi		Receives the high byte component.
 	 *	@param[in]	timeBase	Specifies the AJATimeBase to use.
 	 *	@param[in]	bDrop		Specify true if forcing drop-frame;  otherwise false.
 	 *	@bug		Unimplemented.
 	 *	@todo		Needs to be implemented.
	 */
	void                QueryRP188(uint32_t & outDBB, uint32_t & outLo, uint32_t & outHi, const AJATimeBase & timeBase, const bool bDrop);
	
	/**
	 *	Set HFR divide-by-two flag.
	 *
	 *	@param[in]	bStdTc    Set true when using standard TC notation for HFR (e.g 01:00:00:59 -> 01:00:00:29*), set to true by default
	 */
	void                SetStdTimecodeForHfr(bool bStdTc) {m_stdTimecodeForHfr = bStdTc;}

	
	/**
	 *	Query string showing timecode for current frame count given the passed parameters.
	 *
     *	@param[in]	str                     string with timecode
	 */
    static bool			QueryIsDropFrame(const std::string &str);
	

    static int 			QueryStringSize(void);  ///< @deprecated	Not needed when using std::string.
	
	/**
	 *	Query if rp188 data is drop frame or not
	 *
	 *	@param[in]	inDBB	Specifies the DBB bits of the RP188 struct.
	 *	@param[in]	inLo	Specifies the lo-order 32-bit word of the RP188 struct.
 	 *	@param[in]	inHi	Specifies the hi-order 32-bit word of the RP188 struct.
	 */
	static bool			QueryIsRP188DropFrame (const uint32_t inDBB, const uint32_t inLo, const uint32_t inHi);
	
	AJATimeCode&		operator=(const AJATimeCode  &val);
	AJATimeCode&		operator+=(const AJATimeCode &val);
	AJATimeCode&		operator-=(const AJATimeCode &val);
	AJATimeCode&		operator+=(const int32_t val);
	AJATimeCode&		operator-=(const int32_t val);
	const AJATimeCode	operator+(const AJATimeCode &val) const;
	const AJATimeCode	operator+(const int32_t val) const;
	const AJATimeCode	operator-(const AJATimeCode &val) const;
	const AJATimeCode	operator-(const int32_t val) const;
	bool				operator==(const AJATimeCode &val) const;
	bool				operator<(const AJATimeCode &val) const;
	bool				operator<(const int32_t val) const;
	bool				operator>(const AJATimeCode &val) const;
	bool				operator>(const int32_t val) const;
	bool				operator!=(const AJATimeCode &val) const;
	
	uint32_t			m_frame;
	bool				m_stdTimecodeForHfr;
protected:
private:
};

#endif	// AJA_TIMECODE_H
