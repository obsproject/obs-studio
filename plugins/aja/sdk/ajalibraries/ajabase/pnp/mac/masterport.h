/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/mac/masterport.h
	@brief		Declares the MasterPort class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#if !defined (__MASTERPORT_H__)
	#define __MASTERPORT_H__

	//	Includes
    #include "ajabase/common/ajarefptr.h"
	#include <IOKit/IOKitLib.h>
	#include <assert.h>


	/**
		@brief	I'm a wrapper class for IONotificationPortRef that provides automatic clean-up.
	**/
	class MasterPort;
	typedef AJARefPtr <MasterPort>					MasterPortPtr;

	class MasterPort
	{
		//	Class Methods
		public:
			static IONotificationPortRef			Get (void);
			static MasterPortPtr					GetInstance (void);
		private:
			static bool								Create (MasterPortPtr & outObj);

		//	Instance Methods
		public:
			virtual									~MasterPort ();
			virtual inline IONotificationPortRef	GetPortRef (void) const										{return mpMasterPort;}
			virtual inline							operator IONotificationPortRef () const						{return GetPortRef ();}
			virtual inline bool						IsOkay (void) const											{return GetPortRef () ? true : false;}

		private:
			explicit								MasterPort ();
			inline									MasterPort (const MasterPort & inObj)						{if (&inObj != this) assert (false);}
			inline MasterPort &						operator = (const MasterPort & inRHS)						{if (&inRHS != this) assert (false); return *this;}

		//	Instance Data
		private:
			IONotificationPortRef					mpMasterPort;	///	My master port reference

	};	//	MasterPort

#endif	//	__MASTERPORT_H__
