/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bft.h
	@brief		Handy macros for Basic Functionality Tests.
	@copyright	(C) 2014-2021 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef _NTV2BFT_H_
	#define	_NTV2BFT_H_

	#include <iostream>
	#include <assert.h>

    #if defined (_DEBUG)  &&  defined (MSWindows)
        #include <Windows.h>
    #endif

	#define	STDOUT	std::cout
	#define	STDERR	std::cerr
	#define	ENDL	std::endl

	#if defined (_DEBUG)
		class NTV2BFT_Debugger
		{
			public:
				static inline void Stop (void)
				{
					#if defined (MSWindows)
						DebugBreak ();
					#else
						//	FORCE TRIGGER DEBUGGER
						STDERR << "Stop" << ENDL;
						char *	pByte	(NULL);
						char	byte	(*pByte);
						byte++;
					#endif	//	not Windows
				}
		};
		#define	DEBUG_BREAK()			do {NTV2BFT_Debugger::Stop ();} while (false)
	#else
		#define	DEBUG_BREAK()			do {} while (false)
	#endif	//	_DEBUG


	/**************************************************************************************************************************
		Basic Functionality Testing Macros

		These macros are useful for logging the results of basic functionality tests (BFTs).
		They assume that the function in which they're used returns an boolean value, which will be true if successful,
		and false if unsuccessful.

		SHOULD_BE_TRUE (condition)		--	Logs a failure if the condition evaluates to false; otherwise, logs a success.
											Use this macro when you expect the condition expression to return true.
		SHOULD_BE_FALSE (condition)		--	Logs a failure if the condition evaluates to true; otherwise, logs a success.
											Use this macro when you expect the condition expression to return false.
		SHOULD_BE_EQUAL (x, y)			--	Logs a success if x == y; otherwise logs a failure.
											Use this macro when you expect x to equal y.
		SHOULD_BE_UNEQUAL (x, y)		--	Logs a success if x != y; otherwise logs a failure.
											Use this macro when you expect x to not equal y.

		The following two symbols determine how these macros behave.

			SHOW_PASSED					If true, successes are logged to STDOUT. (Failures are always logged to STDERR.)
										If false, successes are not logged.

			STOP_AFTER_FAILURE			If true, failures will abort the rest of the test.
										If false, failures won't stop execution, but will instead "press on regardless".

			DEBUG_BREAK_AFTER_FAILURE	If true, failures in _DEBUG builds will invoke the DEBUG_BREAK macro (see above).
										If false, failures won't call DEBUG_BREAK in _DEBUG builds.
	**************************************************************************************************************************/
	#if !defined (SHOW_PASSED)
		#define SHOW_PASSED					(false)
	#endif
	#if !defined (STOP_AFTER_FAILURE)
		#define	STOP_AFTER_FAILURE			(true)
	#endif
	#if !defined (DEBUG_BREAK_AFTER_FAILURE)
		#define	DEBUG_BREAK_AFTER_FAILURE	(false)
	#endif


	#define	SHOULD_BE_TRUE(_x_)			do																															\
										{																															\
											if (!(_x_))																												\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           Expected 'True' result from '" << #_x_ << "'" << ENDL										\
														<< "           Instead got 'False'" << ENDL;																\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT << "## NOTE:  '" << #_x_ << "' in '" << __FUNCTION__ << "' returned 'True'" << ENDL;							\
										} while (false)


	#define	SHOULD_BE_FALSE(_x_)		do																															\
										{																															\
											if (_x_)																												\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           Expected 'False' result from '" << #_x_ << "'" << ENDL										\
														<< "           Instead got 'True'" << ENDL;																	\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT << "## NOTE:  '" << #_x_ << "' in '" << __FUNCTION__ << "' returned 'False'" << ENDL;						\
										} while (false)


	#define	SHOULD_BE_NULL(_x_)			do																															\
										{																															\
											if ((_x_) != NULL)																										\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           '" << (_x_) << "' is non-NULL" << ENDL														\
														<< "           Expected '" << #_x_ << "' to be NULL" << ENDL;												\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT << "## NOTE:  '" << #_x_ << "' is NULL in '" << __FUNCTION__ << "'" << ENDL;									\
										} while (false)


	#define	SHOULD_BE_NON_NULL(_x_)		do																															\
										{																															\
											if ((_x_) == NULL)																										\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           '" << (_x_) << "' is NULL" << ENDL															\
														<< "           Expected '" << #_x_ << "' to be non-NULL" << ENDL;											\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT << "## NOTE:  '" << #_x_ << "' is non-NULL in '" << __FUNCTION__ << "'" << ENDL;								\
										} while (false)


	#define	SHOULD_BE_EQUAL(_x_, _y_)	do																															\
										{																															\
											if ((_x_) != (_y_))																										\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           '" << (_x_) << "' is not equal to '" << (_y_) << "'" << ENDL									\
														<< "           Expected '" << #_x_ << "' to equal '" << #_y_ << "'" << ENDL;								\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT << "## NOTE:  '" << #_x_ << "' is equal to '" << #_y_ << "' in '" << __FUNCTION__ << "'" << ENDL;			\
										} while (false)


	#define	SHOULD_BE_UNEQUAL(_x_, _y_)	do																															\
										{																															\
											if ((_x_) == (_y_))																										\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           '" << (_x_) << "' is equal to '" << (_y_) << "'" << ENDL										\
														<< "           Expected '" << #_x_ << "' to not equal '" << #_y_ << "'" << ENDL;							\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT << "## NOTE:  '" << #_x_ << "' is not equal to '" << #_y_ << "' in '" << __FUNCTION__ << "'" << ENDL;		\
										} while (false)

#endif	//	_NTV2BFT_H_
