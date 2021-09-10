/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydatafactory.h
	@brief		Declaration of the AJAAncillaryDataFactory class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATAFACTORY_H
	#define AJA_ANCILLARYDATAFACTORY_H

	#include "ancillarydata.h"


	/**
		@brief	Use my AJAAncillaryDataFactory::GuessAncillaryDataType method to determine what kind of
				ancillary data is being held by a (generic) ::AJAAncillaryData object.
				Use my AJAAncillaryDataFactory::Create method to instantiate a new ::AJAAncillaryData
				object specific to a given type.
	**/
	class AJAExport AJAAncillaryDataFactory
	{
		public:
			/**
				@brief		Creates a new particular subtype of ::AJAAncillaryData object.
				@param[in]	inAncType	Specifies the subtype of ::AJAAncillaryData object (subclass) to instantiate.
				@param[in]	inAncData	Supplies an existing ::AJAAncillaryData object to clone from.
				@return		A pointer to the new instance;  or NULL upon failure.
			**/
			static AJAAncillaryData *		Create (const AJAAncillaryDataType inAncType, const AJAAncillaryData & inAncData);

			/**
				@brief		Creates a new ::AJAAncillaryData object having a particular subtype.
				@param[in]	inAncType	Type of ::AJAAncillaryData object (subclass) to instantiate.
				@param[in]	pInAncData	Optionally supplies an existing ::AJAAncillaryData object to clone from.
				@return		A pointer to the new instance;  or NULL upon failure.
			**/
			static AJAAncillaryData *		Create (const AJAAncillaryDataType inAncType, const AJAAncillaryData * pInAncData = NULL);


			/**
				@brief		Given a generic ::AJAAncillaryData object, attempts to guess what kind of specific
							::AJAAncillaryData object it might be from its raw packet data.
				@param[in]	inAncData	An ::AJAAncillaryData object that contains "raw" packet data.
				@return		The guessed ::AJAAncillaryDataType (or ::AJAAncillaryDataType_Unknown if no idea...).
			**/
			static AJAAncillaryDataType		GuessAncillaryDataType (const AJAAncillaryData & inAncData);

			/**
				@brief		Given a generic ::AJAAncillaryData object, attempts to guess what kind of specific
							::AJAAncillaryData object from its raw packet data.
				@param[in]	pInAncData	A valid, non-NULL pointer to an ::AJAAncillaryData object that contains
										"raw" packet data.
				@return		The guessed ::AJAAncillaryDataType (or ::AJAAncillaryDataType_Unknown if no idea...).
			**/
			static AJAAncillaryDataType		GuessAncillaryDataType (const AJAAncillaryData * pInAncData);

	};	//	AJAAncillaryDataFactory

#endif	// AJA_ANCILLARYDATAFACTORY_H
