/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2signalrouter.h
	@brief		Declares CNTV2SignalRouter class.
	@copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2SIGNALROUTER_H
#define NTV2SIGNALROUTER_H

#include "ajaexport.h"
#include "ntv2publicinterface.h"
#include <stddef.h>
#include <sstream>
#include <set>
#include <map>

typedef std::set <NTV2OutputXptID>			NTV2OutputCrosspointIDSet,			NTV2OutputXptIDSet;				///< @brief	A collection of distinct ::NTV2OutputXptID values.
typedef NTV2OutputXptIDSet::const_iterator	NTV2OutputCrosspointIDSetConstIter,	NTV2OutputXptIDSetConstIter;	///< @brief	A const iterator for iterating over an ::NTV2OutputXptIDSet.
typedef NTV2OutputXptIDSet::iterator		NTV2OutputCrosspointIDSetIter,		NTV2OutputXptIDSetIter;			///< @brief	A non-const iterator for iterating over an ::NTV2OutputXptIDSet.

AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2OutputXptIDSet & inObj);

typedef NTV2OutputCrosspointIDSet					NTV2CrosspointIDSet;		///< @deprecated	Use ::NTV2OutputXptIDSet instead.

typedef std::set <NTV2InputXptID>					NTV2InputCrosspointIDSet,			NTV2InputXptIDSet;			///< @brief	A collection of distinct ::NTV2InputXptID values.
typedef NTV2InputXptIDSet::const_iterator			NTV2InputCrosspointIDSetConstIter,	NTV2InputXptIDSetConstIter;	///< @brief	A const iterator for iterating over an ::NTV2InputXptIDSet.
typedef NTV2InputXptIDSet::iterator					NTV2InputCrosspointIDSetIter,		NTV2InputXptIDSetIter;		///< @brief	A non-const iterator for iterating over an ::NTV2InputXptIDSet.

AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2InputXptIDSet & inObj);

typedef std::set <NTV2WidgetID>						NTV2WidgetIDSet;			///< @brief	A collection of distinct ::NTV2WidgetID values.
typedef NTV2WidgetIDSet::const_iterator				NTV2WidgetIDSetConstIter;	///< @brief	An iterator for iterating over a read-only ::NTV2WidgetIDSet.

AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2WidgetIDSet & inObj);


typedef std::pair <NTV2InputXptID, NTV2OutputXptID>		NTV2SignalConnection, NTV2XptConnection, NTV2Connection;	///< @brief	This links an ::NTV2InputXptID and an ::NTV2OutputXptID.
typedef std::map <NTV2InputXptID, NTV2OutputXptID>		NTV2XptConnections, NTV2ActualConnections;	///< @brief	A map of zero or more one-to-one actual ::NTV2InputXptID to ::NTV2OutputXptID connections.
typedef NTV2XptConnections::const_iterator				NTV2XptConnectionsConstIter, NTV2ActualConnectionsConstIter;
typedef std::multimap <NTV2InputXptID, NTV2OutputXptID>	NTV2PossibleConnections;	///< @brief	A map of zero or more one-to-many possible ::NTV2InputXptID to ::NTV2OutputXptID connections.
typedef NTV2PossibleConnections::const_iterator			NTV2PossibleConnectionsConstIter;

AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2XptConnections & inObj);

typedef	std::map <std::string, NTV2InputXptID>			String2InputXpt;
typedef String2InputXpt::const_iterator					String2InputXptConstIter;

typedef	std::map <NTV2InputXptID, std::string>			InputXpt2String;
typedef InputXpt2String::const_iterator					InputXpt2StringConstIter;

typedef std::multimap <NTV2InputXptID, NTV2WidgetID>	InputXpt2WidgetIDs;
typedef std::pair <NTV2InputXptID, NTV2WidgetID>		InputXpt2WidgetIDPair;
typedef	InputXpt2WidgetIDs::const_iterator				InputXpt2WidgetIDsConstIter;

typedef	std::map <std::string, NTV2OutputXptID>			String2OutputXpt;
typedef String2OutputXpt::const_iterator				String2OutputXptConstIter;

typedef	std::map <NTV2OutputXptID, std::string>			OutputXpt2String;
typedef OutputXpt2String::const_iterator				OutputXpt2StringConstIter;

typedef std::multimap <NTV2OutputXptID, NTV2WidgetID>	OutputXpt2WidgetIDs;
typedef std::pair <NTV2OutputXptID, NTV2WidgetID>		OutputXpt2WidgetIDPair;
typedef	OutputXpt2WidgetIDs::const_iterator				OutputXpt2WidgetIDsConstIter;

typedef std::pair <NTV2WidgetID, NTV2OutputXptID>		Widget2OutputXptPair;
typedef std::multimap <NTV2WidgetID, NTV2OutputXptID>	Widget2OutputXpts;
typedef Widget2OutputXpts::const_iterator				Widget2OutputXptsConstIter;

typedef std::pair <NTV2WidgetID, NTV2InputXptID>		Widget2InputXptPair;
typedef std::multimap <NTV2WidgetID, NTV2InputXptID>	Widget2InputXpts;
typedef Widget2InputXpts::const_iterator				Widget2InputXptsConstIter;

typedef std::pair <NTV2WidgetID, NTV2Channel>			Widget2ChannelPair;
typedef std::multimap <NTV2WidgetID, NTV2Channel>		Widget2Channels;
typedef Widget2Channels::const_iterator					Widget2ChannelsConstIter;

typedef std::pair <NTV2WidgetID, NTV2WidgetType>		Widget2TypePair;
typedef std::multimap <NTV2WidgetID, NTV2WidgetType>	Widget2Types;
typedef Widget2Types::const_iterator					Widget2TypesConstIter;

typedef std::set<NTV2WidgetType>						NTV2WidgetTypeSet;
typedef NTV2WidgetTypeSet::const_iterator				NTV2WidgetTypeSetConstIter;

/**
	@brief	This class is a collection of widget input-to-output connections that can be applied all-at-once to an NTV2 device.
			Call AddConnection to connect a widget input (specified by ::NTV2InputXptID) to a widget's output (specified by ::NTV2OutputXptID).
			Call the CNTV2Card::ApplySignalRoute function to apply this route to the device.
	@note	Use of this class is optional, as widget signal routing can always be performed using direct calls to CNTV2Card::Connect.
	@note	This class is not thread-safe.
	@note	Public access to the ::NTV2RoutingEntry structs are deprecated. Please use ::NTV2InputXptID instead.
	@see	\ref ntv2signalrouting
**/
class AJAExport CNTV2SignalRouter
{
	//	Instance Methods
	public:
		inline										CNTV2SignalRouter ()			{Reset ();}		///< @brief	My default constructor.
		virtual inline								~CNTV2SignalRouter ()			{}				///< @brief	My default destructor.

		#if !defined (NTV2_DEPRECATE)
			virtual NTV2_DEPRECATED_f(operator						NTV2RoutingTable () const);																///< @deprecated	This function is obsolete.
			virtual NTV2_DEPRECATED_f(operator						NTV2RoutingTable* ());																	///< @deprecated	This function is obsolete.
			virtual inline NTV2_DEPRECATED_f(const NTV2RoutingEntry &	getRoutingEntry (const ULWord inIndex) const)		{return GetRoutingEntry (inIndex);}	///< @deprecated	This function is obsolete.
			virtual inline NTV2_DEPRECATED_f(ULWord					getCurrentIndex (void) const)						{return GetNumberOfConnections ();}	///< @deprecated	Use GetNumberOfRoutes instead.
			virtual inline NTV2_DEPRECATED_f(bool						add (const NTV2RoutingEntry & inEntry))				{return AddConnection (inEntry);}	///< @deprecated	Use AddConnection instead.
			virtual NTV2_DEPRECATED_f(bool							addWithRegisterAndValue (const NTV2RoutingEntry & inEntry, const ULWord inRegisterNum, const ULWord inValue,
																							const ULWord inMask = 0xFFFFFFFF, const ULWord inShift = 0));	///< @deprecated	This function is obsolete.
			virtual inline NTV2_DEPRECATED_f(void						clear (void))										{mConnections.clear ();}			///< @deprecated	Use Reset instead.
			virtual NTV2_DEPRECATED_f(const NTV2RoutingEntry &		GetRoutingEntry (const ULWord inIndex) const);		///< @deprecated	This function is obsolete.
			virtual NTV2_DEPRECATED_f(bool							GetRoutingTable (NTV2RoutingTable & outRoutingTable) const);								///< @deprecated	This function is obsolete.
		#endif	//	!defined (NTV2_DEPRECATE)

		#if !defined (NTV2_DEPRECATE_12_5)
			virtual NTV2_DEPRECATED_f(bool	addWithValue (const NTV2RoutingEntry & inEntry, const ULWord inValue));				///< @deprecated	Use AddConnection with ::NTV2InputXptIDSet instead.
			virtual NTV2_DEPRECATED_f(bool	AddConnection (const NTV2RoutingEntry & inEntry, const NTV2OutputCrosspointID inSignalOutput = NTV2_XptBlack));	///< @deprecated	Use AddConnection(::NTV2InputXptID, ::NTV2OutputXptID) instead.
		#endif	//	!defined (NTV2_DEPRECATE_12_5)

		/**
			@brief	Answers with the ::NTV2InputXptID and ::NTV2OutputXptIDSet for the given ROM register value.
			@param[in]	inROMRegNum			Specifies the ROM register number.
			@param[in]	inROMRegValue		Specifies the ROM register value.
			@param[out]	outInputXpt			Receives the input crosspoint associated with the ROM register.
			@param[out]	outOutputXpts		Receives the valid (implemented) output crosspoint routes.
			@param[in]	inAppendOutputXpts	If true, appends output crosspoints to the output set;
											otherwise clears the output crosspoint set (the default).
			@return	True if successful;  otherwise false.
		**/
		static bool GetRouteROMInfoFromReg (const ULWord inROMRegNum, const ULWord inROMRegValue,
												NTV2InputXptID & outInputXpt, NTV2OutputXptIDSet & outOutputXpts,
												const bool inAppendOutputXpts = false);	//	New in SDK 16.0

		/**
			@brief		Answers with the implemented crosspoint connections as obtained from the given ROM registers.
			@param[in]	inROMRegisters	The ROM register numbers and values.
			@param[out]	outConnections	Receives the legal implemented connections/routes.
			@return	True if successful;  otherwise false.
		**/
		static bool GetPossibleConnections (const NTV2RegReads & inROMRegisters, NTV2PossibleConnections & outConnections);	//	New in SDK 16.0

		/**
			@brief		Prepares an initialized, zeroed NTV2RegReads that's prepared to read all ROM registers from a device.
			@param[out]	outROMRegisters	Receives the prepared NTV2RegReads.
			@return		True if successful;  otherwise false.
		**/
		static bool MakeRouteROMRegisters (NTV2RegReads & outROMRegisters);	//	New in SDK 16.0

		/**
			@brief		Adds a connection between a widget's signal input (sink) and another widget's signal output (source).
			@param[in]	inSignalInput		Specifies the widget signal input (sink) as an ::NTV2InputXptID.
			@param[in]	inSignalOutput		Specifies the widget signal output (source) as an ::NTV2OutputXptID. If not specified, uses ::NTV2_XptBlack.
			@return		True if successfully added;  otherwise false.
			@see		CNTV2Card::Connect, CNTV2SignalRouter::RemoveConnection
		**/
		virtual bool								AddConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput = NTV2_XptBlack);

		/**
			@brief		Removes the connection between the specified input (signal sink) and output (signal source).
			@param[in]	inSignalInput		Specifies the widget signal input (sink) as an ::NTV2InputXptID.
			@param[in]	inSignalOutput		Specifies the widget signal output (source) as an ::NTV2OutputXptID.
			@return		True if successfully removed;  otherwise false.
			@see		CNTV2Card::Disconnect, CNTV2SignalRouter::AddConnection
		**/
		virtual bool								RemoveConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput);

		/**
			@brief		Answers true if I contain a connection between the specified input (signal sink) and output (signal source).
			@param[in]	inSignalInput		Specifies the widget signal input (sink) as an ::NTV2InputXptID.
			@param[in]	inSignalOutput		Specifies the widget signal output (source) as an ::NTV2OutputXptID.
			@return		True if I have such a connection;  otherwise false.
			@see		CNTV2Card::IsConnected, CNTV2Card::IsConnectedTo
		**/
		virtual bool								HasConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput) const;

		/**
			@brief		Answers true if I contain a connection that involves the given input (signal sink).
			@param[in]	inSignalInput		Specifies the widget signal input (sink) of interest as an ::NTV2InputXptID.
			@return		True if I have a connection involving the given input;  otherwise false.
			@see		CNTV2Card::IsConnected, CNTV2Card::GetConnectedInput, CNTV2SignalRouter::HasConnection
		**/
		virtual bool								HasInput (const NTV2InputXptID inSignalInput) const;

		/**
			@return		The output crosspoint that the given input is connected to, or NTV2_XptBlack if not connected.
			@param[in]	inSignalInput		Specifies the widget signal input (sink) of interest.
			@see		CNTV2Card::GetConnectedOutput
		**/
		virtual NTV2OutputXptID						GetConnectedOutput (const NTV2InputXptID inSignalInput) const;

		/**
			@brief		Resets me, erasing any/all existing connections.
			@see		CNTV2Card::ClearRouting, CNTV2SignalRouter::ResetFromRegisters
		**/
		virtual inline void							Reset (void)										{mConnections.clear ();}

		/**
			@brief		Resets me, erasing any/all existing connections, then rebuilding my connections from the given register values.
			@param[in]	inInputXpts		Specifies the input crosspoints of interest (perhaps obtained from CNTV2SignalRouter::GetAllWidgetInputs).
			@param[in]	inRegReads		Specifies the routing registers/values (perhaps obtained from CNTV2Card::ReadRegisters).
			@return		True if successful;  otherwise false.
			@see		CNTV2SignalRouter::Reset, CNTV2SignalRouter::GetRegisterWrites
		**/
		virtual bool								ResetFromRegisters (const NTV2InputXptIDSet & inInputXpts, const NTV2RegisterReads & inRegReads);

		/**
			@brief		Resets me, replacing any/all existing connections with the given connections.
			@param[in]	inConnections	Specifies the new routing connections.
			@return		True if successful;  otherwise false.
			@see		CNTV2SignalRouter::Reset
		**/
		virtual bool								ResetFrom (const NTV2XptConnections & inConnections)	{mConnections = inConnections; return true;}	//	New in SDK 16.0

		/**
			@return	The current number of connections (signal routes).
			@see		CNTV2SignalRouter::IsEmpty
		**/
		virtual inline ULWord						GetNumberOfConnections (void) const					{return ULWord (mConnections.size ());}

		/**
			@return	True if I have no connections (signal routes); otherwise false.
			@see		CNTV2SignalRouter::GetNumberOfConnections
		**/
		virtual inline bool							IsEmpty (void) const								{return mConnections.empty();}

		/**
			@return	A copy of my connections.
		**/
		virtual inline NTV2XptConnections			GetConnections (void) const							{return mConnections;}

		/**
			@brief		Returns a sequence of NTV2RegInfo values that can be written to an NTV2 device using its WriteRegisters function.
			@param[out]	outRegWrites	Receives a sequence of NTV2ReadWriteRegisterSingle values.
			@return		True if successful;  otherwise false.
		**/
		virtual bool								GetRegisterWrites (NTV2RegisterWrites & outRegWrites) const;

		/**
			@brief		Compares me with another routing, and returns three connection mappings as a result of the comparison:
						those that are new, changed, and missing.
			@param[in]	inRHS		The CNTV2SignalRouter that I'm being compared with.
			@param[out]	outNew		Receives the new connections (those that I have, but RHS doesn't).
			@param[out]	outChanged	Receives the changed connections, where only the output crosspoint changed.
									The output crosspoint of these changed connections will be those from "inRHS"
									-- not the new ones.
			@param[out]	outMissing	Receives the deleted connections (those that RHS has, but I don't).
			@return		True if identical (i.e. the returned output connection maps are all empty);  otherwise false.
			@see		CNTV2SignalRouter::operator ==, CNTV2SignalRouter::operator !=
		**/
		virtual bool								Compare (const CNTV2SignalRouter & inRHS,
															NTV2XptConnections & outNew,
															NTV2XptConnections & outChanged,
															NTV2XptConnections & outMissing) const;

		/**
			@return		True if my connections are identical to those of the given right-hand-side signal router; otherwise false.
			@param[in]	inRHS		The CNTV2SignalRouter that I'll be compared with.
			@see		CNTV2SignalRouter::Compare, CNTV2SignalRouter::operator !=
		**/
		virtual inline bool			operator == (const CNTV2SignalRouter & inRHS) const		{NTV2XptConnections tmp; return Compare(inRHS, tmp,tmp,tmp);}

		/**
			@return		True if my connections differ from those of the given right-hand-side signal router;  otherwise false.
			@param[in]	inRHS		The CNTV2SignalRouter that I'll be compared with.
			@see		CNTV2SignalRouter::Compare, CNTV2SignalRouter::operator ==
		**/
		virtual inline bool			operator != (const CNTV2SignalRouter & inRHS) const		{return !(inRHS == *this);}

		/**
			@brief		Prints me in a human-readable format to the given output stream.
			@param		inOutStream			Specifies the output stream that is to receive the human-readable data.
			@param[in]	inForRetailDisplay	Specify true to use human-readable names in the display;  otherwise false to use names found in the SDK.
			@return		A reference to the specified output stream.
			@see		CNTV2SignalRouter::PrintCode
		**/
		virtual std::ostream &						Print (std::ostream & inOutStream, const bool inForRetailDisplay = false) const;

		struct PrintCodeConfig
		{
			bool		mShowComments;		///< @brief	If true, show comments in the generated code
			bool		mShowDeclarations;	///< @brief	If true, show variable declarations in the generated code
			bool		mUseRouter;			///< @brief	If true, use calls to CNTV2DeviceRouter instead of CNTV2Card
			std::string	mPreCommentText;	///< @brief	Comment prefix text
			std::string	mPostCommentText;	///< @brief	Comment postfix text
			std::string	mPreClassText;		///< @brief	Class prefix text
			std::string	mPostClassText;		///< @brief	Class postfix text
			std::string	mPreVariableText;	///< @brief	Variable prefix text
			std::string	mPostVariableText;	///< @brief	Variable postfix text
			std::string	mPreXptText;		///< @brief	Crosspoint variable prefix text
			std::string	mPostXptText;		///< @brief	Crosspoint variable postfix text
			std::string	mPreFunctionText;	///< @brief	Function name prefix text
			std::string	mPostFunctionText;	///< @brief	Function name postfix text
			std::string	mDeviceVarName;		///< @brief	Name to use for CNTV2Card variable
			std::string	mRouterVarName;		///< @brief	Name to use for CNTV2DeviceRouter variable
			std::string	mLineBreakText;		///< @brief	Text to use for line breaks
			std::string	mFieldBreakText;	///< @brief	Text to use for field breaks
			NTV2XptConnections mNew;		///< @brief	Optional, to show new connections
			NTV2XptConnections mChanged;	///< @brief	Optional, to show changed connections
			NTV2XptConnections mMissing;	///< @brief	Optional, to show deleted connections
			/**
				@brief	Default constructor sets the following default settings:
						-	include "//"-style comments and variable declarations;
						-	uses CNTV2Card calls;
						-	uses standard newline line breaks.
			**/
			PrintCodeConfig ();
		};

		/**
			@brief		Prints me as source code to the given output stream.
			@param[out]	outCode		Receives the generated source code.
			@param[in]	inConfig	Specifies how the source code will be generated.
									If unspecified, uses the PrintCodeConfig's default settings.
			@return		True if successful;  otherwise false.
			@see		CNTV2SignalRouter::Print
		**/
		virtual bool								PrintCode (std::string & outCode, const PrintCodeConfig & inConfig = PrintCodeConfig()) const;

		#if !defined (NTV2_DEPRECATE_12_5)
			virtual NTV2_DEPRECATED_f(bool	PrintCode (std::string & outCode, const bool inShowComments, const bool inShowDeclarations, const bool inUseRouter) const);	///< @deprecated	Use the new PrintCode method that accepts a PrintCodeConfig parameter.
		#endif	//	!defined (NTV2_DEPRECATE_12_5)

	//	Instance Data
	private:
        typedef NTV2XptConnections::iterator		NTV2XptConnectionsIter;

		NTV2XptConnections							mConnections;		///< @brief	My collection of NTV2SignalConnections


	public:	//	CLASS METHODS
		/**
			@brief		Returns a string containing the most compact human-readable form for a given input crosspoint.
			@param[in]	inInputXpt		Specifies the ::NTV2InputXptID of interest.
			@return		A string containing the most compact human-readable representation of the input crosspoint.
			@see		CNTV2SignalRouter::NTV2OutputCrosspointIDToString, CNTV2SignalRouter::StringToNTV2InputCrosspointID
		**/
		static std::string			NTV2InputCrosspointIDToString (const NTV2InputXptID inInputXpt);

		/**
			@brief		Returns a string containing the most compact human-readable form for a given output crosspoint.
			@param[in]	inOutputXpt		Specifies the ::NTV2OutputXptID of interest.
			@return		A string containing the most compact human-readable representation of the output crosspoint.
			@see		CNTV2SignalRouter::NTV2InputCrosspointIDToString, CNTV2SignalRouter::StringToNTV2OutputCrosspointID
		**/
		static std::string			NTV2OutputCrosspointIDToString (const NTV2OutputXptID inOutputXpt);

		/**
			@brief		Returns a string containing the most compact human-readable form for a given input crosspoint.
			@param[in]	inStr		Specifies the string to convert into an NTV2InputXptID.
			@return		The corresponding input crosspoint.
			@see		CNTV2SignalRouter::StringToNTV2OutputCrosspointID, CNTV2SignalRouter::NTV2InputCrosspointIDToString
		**/
		static NTV2InputXptID		StringToNTV2InputCrosspointID (const std::string & inStr);

		/**
			@brief		Returns the output crosspoint that corresponds to the given string.
			@param[in]	inStr		Specifies the string to convert into an NTV2OutputXptID.
			@return		The corresponding output crosspoint.
			@see		CNTV2SignalRouter::StringToNTV2InputCrosspointID, CNTV2SignalRouter::NTV2OutputCrosspointIDToString
		**/
		static NTV2OutputXptID		StringToNTV2OutputCrosspointID (const std::string & inStr);

		/**
			@brief		Returns the widget IDs supported by the given device.
			@param[in]	inDeviceID		Specifies the NTV2DeviceID of the NTV2 device of interest.
			@param[out]	outWidgets		Receives the NTV2WidgetIDSet of widgets that are supported by the given device.
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetIDs (const NTV2DeviceID inDeviceID, NTV2WidgetIDSet & outWidgets);

		/**
			@brief		Returns the widgets that "own" the specified output crosspoint.
			@param[in]	inOutputXpt		Specifies the output crosspoint of interest.
			@param[out]	outWidgetIDs	Receives the NTV2WidgetIDSet containing the widgets that "own" the output
										crosspoint (or an empty set upon failure).
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetsForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetIDSet & outWidgetIDs);

		/**
			@brief		Returns the widget that "owns" the specified output crosspoint.
			@param[in]	inOutputXpt		Specifies the output crosspoint of interest.
			@param[out]	outWidgetID		Receives the ::NTV2WidgetID of the widget that "owns" the output crosspoint
										(or ::NTV2_WIDGET_INVALID upon failure).
			@param[in]	inDeviceID		Optionally specifies a device ID to resolve any ambiguity if more than
										one ::NTV2WidgetID is associated with the given ::NTV2OutputXptID.
										Defaults to ::DEVICE_ID_NOTFOUND.
			@note		The default behavior is to answer with the first ::NTV2WidgetID found in the ::NTV2WidgetIDSet.
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetID & outWidgetID, const NTV2DeviceID inDeviceID = DEVICE_ID_NOTFOUND);

		/**
			@brief		Returns the widgets that "own" the specified input crosspoint.
			@param[in]	inInputXpt		Specifies the input crosspoint of interest.
			@param[out]	outWidgetIDs	Receives the ::NTV2WidgetIDSet containing the widgets that "own" the input
										crosspoint (or an empty set upon failure).
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetsForInput (const NTV2InputXptID inInputXpt, NTV2WidgetIDSet & outWidgetIDs);

		/**
			@brief		Returns the widget that "owns" the specified input crosspoint.
			@param[in]	inInputXpt		Specifies the input crosspoint of interest.
			@param[out]	outWidgetID		Receives the ::NTV2WidgetID of the widget that "owns" the input crosspoint
										(or ::NTV2_WIDGET_INVALID upon failure).
			@param[in]	inDeviceID		Optionally specifies a device ID to resolve any ambiguity if more than
										one ::NTV2WidgetID is associated with the given ::NTV2InputXptID.
										Defaults to ::DEVICE_ID_NOTFOUND, which returns the first match.
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetForInput (const NTV2InputXptID inInputXpt, NTV2WidgetID & outWidgetID, const NTV2DeviceID inDeviceID = DEVICE_ID_NOTFOUND);

		/**
			@brief		Returns the input crosspoints known to be "owned" by the given widget.
			@param[in]	inWidgetID		Specifies the ::NTV2WidgetID of the widget of interest.
			@param[out]	outInputs		Receives the set of ::NTV2InputXptIDSet that are "owned" by the widget
										(or empty upon failure).
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetInputs (const NTV2WidgetID inWidgetID, NTV2InputXptIDSet & outInputs);

		/**
			@brief		Returns all known widget input crosspoints for the given device.
			@param[in]	inDeviceID		Specifies the ::NTV2DeviceID of the device of interest.
			@param[out]	outInputs		Receives the ::NTV2InputXptIDSet (or empty upon failure).
			@return		True if successful;  otherwise false.
		**/
		static bool					GetAllWidgetInputs (const NTV2DeviceID inDeviceID, NTV2InputXptIDSet & outInputs);

		/**
			@brief		Returns all routing registers for the given set of input crosspoints.
			@param[in]	inInputs		Specifies the input crosspoints.
			@param[out]	outRegInfos		Receives the ::NTV2RegInfo collection (empty upon failure).
			@return		True if successful;  otherwise false.
		**/
		static bool					GetAllRoutingRegInfos (const NTV2InputXptIDSet & inInputs, NTV2RegisterWrites & outRegInfos);

		/**
			@brief		Returns the output crosspoints known to be "owned" by the given widget.
			@param[in]	inWidgetID		Specifies the ::NTV2WidgetID of the widget of interest.
			@param[out]	outOutputs		Receives the set of ::NTV2OutputXptIDSet that are "owned" by the widget
										(or empty upon failure).
			@return		True if successful;  otherwise false.
		**/
		static bool					GetWidgetOutputs (const NTV2WidgetID inWidgetID, NTV2OutputXptIDSet & outOutputs);

		/**
			@brief		Converts a set of crosspoint registers into a set of crosspoint connections.
			@param[in]	inInputXptIDs	Specifies the input crosspoints.
			@param[in]	inRegValues		Specifies the crosspoint register values.
			@param[out]	outConnections	Receives the connections found in the given register values.
			@return		True if successful;  otherwise false.
		**/
		static bool					GetConnectionsFromRegs (const NTV2InputXptIDSet & inInputXptIDs, const NTV2RegisterReads & inRegValues, NTV2XptConnections & outConnections);	//	New in SDK 16.0

		/**
			@param[in]	inInputXpt	Specifies the input crosspoint ID of interest.
			@return		True if the input only accepts RGB;  otherwise false.
		**/
		static bool					IsRGBOnlyInputXpt (const NTV2InputXptID inInputXpt);	//	New in SDK 16.0

		/**
			@param[in]	inInputXpt	Specifies the input crosspoint ID of interest.
			@return		True if the input only accepts YCbCr;  otherwise false.
		**/
		static bool					IsYUVOnlyInputXpt (const NTV2InputXptID inInputXpt);	//	New in SDK 16.0

		/**
			@param[in]	inInputXpt	Specifies the input crosspoint ID of interest.
			@return		True if the input is a mask/key input;  otherwise false.
		**/
		static bool					IsKeyInputXpt (const NTV2InputXptID inInputXpt);	//	New in SDK 16.0

		/**
			@param[in]	inWidgetID		Specifies the widget ID of interest.
			@return		The corresponding channel.
		**/		
		static NTV2Channel			WidgetIDToChannel (const NTV2WidgetID inWidgetID); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@param[in]	inChannel		Specifies the channel of interest.
			@return		The corresponding widget ID.
		**/
		static NTV2WidgetID			WidgetIDFromTypeAndChannel(const NTV2WidgetType inWidgetType, const NTV2Channel inChannel); // New in SDK 16.1
		
		/**
			@param[in]	inWidgetID		Specifies the widget ID of interest.
			@return		The corresponding widget type.
		**/		
		static NTV2WidgetType		WidgetIDToType (const NTV2WidgetID inWidgetID); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is an SDI widget.
		**/
		static bool					IsSDIWidgetType (const NTV2WidgetType inWidgetType); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is an SDI Input Widget.
		**/
		static bool					IsSDIInputWidgetType (const NTV2WidgetType inWidgetType); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is an SDI Output widget.
		**/
		static bool					IsSDIOutputWidgetType (const NTV2WidgetType inWidgetType); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is a 3G SDI widget.
		**/
		static bool					Is3GSDIWidgetType (const NTV2WidgetType inWidgetType); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is a 12G SDI widget.
		**/
		static bool					Is12GSDIWidgetType (const NTV2WidgetType inWidgetType); // New in SDK 16.1

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is a Dual Link widget.
		**/
		static bool					IsDualLinkWidgetType(const NTV2WidgetType inWidgetType);

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is a Dual Link Input widget.
		**/
		static bool					IsDualLinkInWidgetType(const NTV2WidgetType inWidgetType);

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is a Dual Link Output widget.
		**/
		static bool					IsDualLinkOutWidgetType(const NTV2WidgetType inWidgetType);

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is an HDMI widget.
		**/
		static bool					IsHDMIWidgetType(const NTV2WidgetType inWidgetType);

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is an HDMI Input widget.
		**/
		static bool					IsHDMIInWidgetType(const NTV2WidgetType inWidgetType);

		/**
			@param[in]	inWidgetType	Specifies the widget type of interest.
			@return		True if the widget type is an HDMI Output widget.
		**/
		static bool					IsHDMIOutWidgetType(const NTV2WidgetType inWidgetType);

		/**
			@brief		Compares two sets of crosspoint connections.
			@param[in]	inLHS		Specifies the input crosspoints.
			@param[in]	inRHS		Specifies the crosspoint register values.
			@param[out]	outNew		Receives the new connections found in the RHS that aren't in the LHS.
			@param[out]	outRemoved	Receives the deleted connections not found in the RHS that are in the LHS.
			@return		True if successful;  otherwise false.
		**/
		static bool					CompareConnections (const NTV2XptConnections & inLHS,
														const NTV2XptConnections & inRHS,
														NTV2XptConnections & outNew,
														NTV2XptConnections & outRemoved);	//	New in SDK 16.0

		/**
			@brief		Decodes a given string into a map of crosspoint connections.
			@param[in]	inString			Specifies the string to be parsed. It can contain the pnemonics that
											CNTV2SignalRouter::PrintCode generates, or a simple C++ code snippet that contains
											one or more "device.Connect(...)" calls.
			@param[out]	outConnections		Receives the connections from what is successfully parsed from the string.
											It will be empty if this function fails.
			@return		True if successful;  otherwise false.
		**/
		static bool					CreateFromString (const std::string & inString, NTV2XptConnections & outConnections);	//	New in SDK 16.0

		/**
			@brief		Sets the router from the given string.
			@param[in]	inString		Specifies the string to be parsed.
			@param[out]	outRouter		The CNTV2SignalRouter to be cleared and set from what is parsed from the string.
										It will be empty if this function fails.
			@return		True if successful;  otherwise false.
			@see		CNTV2SignalRouter::PrintCode, CNTV2SignalRouter::ResetFromRegisters
		**/
		static bool					CreateFromString (const std::string & inString, CNTV2SignalRouter & outRouter);

		/**
			@brief		Converts the given map of crosspoint connections into source code.
			@param[out]	outCode			Receives the generated source code.
			@param[in]	inConnections	Receives the connections from what is successfully parsed from the string.
										It will be empty if this function fails.
			@param[in]	inConfig		Specifies how the source code will be generated.
										If unspecified, uses the PrintCodeConfig's default settings.
			@return		True if successful;  otherwise false.
		**/
		static bool					ToCodeString (std::string & outCode, const NTV2XptConnections & inConnections,
													const PrintCodeConfig & inConfig = PrintCodeConfig());	//	New in SDK 16.0


		#if !defined (NTV2_DEPRECATE_12_5)
			static NTV2_DEPRECATED_f(const NTV2RoutingEntry &		GetInputSelectEntry (const NTV2InputCrosspointID inInputXpt));	///< @deprecated	NTV2RoutingEntry is deprecated.
			static NTV2_DEPRECATED_f(NTV2InputCrosspointID		NTV2RoutingEntryToInputCrosspointID (const NTV2RoutingEntry & inEntry));	///< @deprecated	NTV2RoutingEntry is deprecated.
		#endif	//	!defined (NTV2_DEPRECATE_12_5)

    	static bool					IsInitialized(void);	///< @return	True if the Signal Router singleton has been allocated/created; otherwise false.

        /**
            @brief		Explicitly allocates and initializes the Routing Expert singleton.
            @return		True if successful;  otherwise false.
            @note		Normally, there is no need to call this function, as the Routing Expert singleton is
            			automatically allocated and initialized.
            @see		CNTV2SignalRouter::Deinitialize
        **/
    	static bool					Initialize(void);

        /**
            @brief		Explicitly deinitializes and deallocates the Routing Expert singleton.
            @return		True if successful;  otherwise false.
            @note		Normally, there is no need to call this function, as the Routing Expert singleton is
            			automatically deinitialized and deallocated.
            @see		CNTV2SignalRouter::Initialize
        **/
    	static bool					Deinitialize(void);

};	//	CNTV2SignalRouter


/**
	@return		The appropriate ::NTV2InputXptID for the given FrameStore, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inFrameStore	Specifies the FrameStore of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsBInput		Specify true to obtain the "B" input crosspoint (for dual-link). Defaults to false (the "A" input crosspoint).
**/
AJAExport NTV2InputXptID		GetFrameBufferInputXptFromChannel (const NTV2Channel inFrameStore, const bool inIsBInput = false);

/**
	@return		The appropriate ::NTV2InputXptID for the given color space converter (CSC) widget, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inCSC			Specifies the CSC of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsKeyInput	Specify true to obtain the key (alpha) input crosspoint. Defaults to false (the video input crosspoint).
**/
AJAExport NTV2InputXptID		GetCSCInputXptFromChannel (const NTV2Channel inCSC, const bool inIsKeyInput = false);

/**
	@return		The appropriate ::NTV2InputXptID for the given LUT widget, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inLUT			Specifies the LUT of interest, expressed as an ::NTV2Channel (a zero-based index value).
**/
AJAExport NTV2InputXptID		GetLUTInputXptFromChannel (const NTV2Channel inLUT);	//	New in SDK 16.0

/**
	@return		The appropriate ::NTV2InputXptID for the given Dual-Link Input widget, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inChannel		Specifies the Dual-Link Input converter of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inLinkB			Specifies whether to return the A or the B link crosspoint ID. Defaults to the A link crosspoint.
**/
AJAExport NTV2InputXptID		GetDLInInputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB = false);

/**
	@return		The appropriate ::NTV2InputXptID for the given Dual Link Output widget, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inDLOutWidget		Specifies the Dual Link Output of interest, expressed as an ::NTV2Channel (a zero-based index value).
**/
AJAExport NTV2InputXptID		GetDLOutInputXptFromChannel (const NTV2Channel inDLOutWidget);

/**
	@return		The appropriate NTV2OutputCrosspointID for the given color space converter (CSC) widget, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inCSC			Specifies the CSC of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsKey			Specify true to obtain the key (alpha) output crosspoint. Defaults to false (the video output crosspoint).
	@param[in]	inIsRGB			Specify true to obtain the RGB video output crosspoint. Defaults to false (the YUV video output crosspoint).
								Ignored if true passed to inIsKey.
**/
AJAExport NTV2OutputXptID		GetCSCOutputXptFromChannel (const NTV2Channel inCSC, const bool inIsKey = false, const bool inIsRGB = false);

/**
	@return		The appropriate NTV2OutputCrosspointID for the given LUT widget.
	@param[in]	inLUT			Specifies the LUT of interest, expressed as an ::NTV2Channel (a zero-based index value).
**/
AJAExport NTV2OutputXptID		GetLUTOutputXptFromChannel (const NTV2Channel inLUT);	//	New in SDK 16.0

/**
	@return		The appropriate NTV2OutputCrosspointID for the given FrameStore, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inFrameStore	Specifies the FrameStore of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsRGB			Specify true to obtain the RGB output crosspoint. Defaults to false (the YUV output crosspoint).
	@param[in]	inIs425			Specify true to obtain the 425 output crosspoint. Defaults to false (the normal non-425 output crosspoint).
**/
AJAExport NTV2OutputXptID		GetFrameBufferOutputXptFromChannel (const NTV2Channel inFrameStore, const bool inIsRGB = false, const bool inIs425 = false);

/**
	@return		The appropriate ::NTV2OutputCrosspointID for the given ::NTV2InputSource, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inInputSource	Specifies the NTV2InputSource of interest.
	@param[in]	inIsSDI_DS2		Specify true to obtain the DS2 output crosspoint (for SDI input sources).
								Defaults to false (the DS1 output crosspoint).
								Ignored for non-SDI input sources.
	@param[in]	inIsHDMI_RGB	Specify true to obtain the RGB output crosspoint (for HDMI input sources).
								Defaults to false (the YUV output crosspoint).
								Ignored for non-HDMI input sources.
	@param[in]	inHDMI_Quadrant	Specifies the 4K/UHD quadrant of interest (for HDMI input sources), where 0=upperLeft, 1=upperRight, 2=lowerLeft, 3=lowerRight.
								Defaults to 0 (upperLeft). Ignored for non-HDMI input sources.
**/
AJAExport NTV2OutputXptID		GetInputSourceOutputXpt (const NTV2InputSource inInputSource,  const bool inIsSDI_DS2 = false,
																const bool inIsHDMI_RGB = false,  const UWord inHDMI_Quadrant = 0);

/**
	@return		The appropriate SDI input's ::NTV2OutputCrosspointID for the given SDI Input, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inSDIInput		Specifies the SDI Input widget of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsDS2			Specify true to obtain the DS2 output crosspoint. Defaults to false (the DS1 output crosspoint).
**/
AJAExport NTV2OutputXptID		GetSDIInputOutputXptFromChannel (const NTV2Channel inSDIInput,  const bool inIsDS2 = false);

/**
	@return		The ::NTV2OutputCrosspointID for the given Dual Link Output, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inDLOutput		Specifies the Dual-Link Output widget of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsLinkB		Specify true to obtain the DS2 output crosspoint. Defaults to false (the DS1 output crosspoint).
**/
AJAExport NTV2OutputXptID		GetDLOutOutputXptFromChannel(const NTV2Channel inDLOutput, const bool inIsLinkB = false);

/**
	@return		The appropriate ::NTV2OutputCrosspointID for the given Dual Link Input, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inDLInput		Specifies the Dual-Link Input widget of interest, expressed as an ::NTV2Channel (a zero-based index value).
**/
AJAExport NTV2OutputXptID		GetDLInOutputXptFromChannel(const NTV2Channel inDLInput);

/**
	@return		The appropriate ::NTV2InputXptID for the given ::NTV2OutputDestination, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inOutputDest	Specifies the ::NTV2OutputDestination of interest.
	@param[in]	inIsSDI_DS2		Specify true to obtain the DS2 input crosspoint (SDI output destinations only). Defaults to false (the DS1 input).
								Ignored for non-SDI output destinations.
	@param[in]	inHDMI_Quadrant	Specifies the 4K/UHD quadrant of interest (for HDMI output destinations),
								where 0=upperLeft, 1=upperRight, 2=lowerLeft, 3=lowerRight.
								Values above 3 are deemed to be non-4K/UHD (the default).
								Ignored for non-HDMI output destinations.
**/
AJAExport NTV2InputXptID		GetOutputDestInputXpt (const NTV2OutputDestination inOutputDest,  const bool inIsSDI_DS2 = false,  const UWord inHDMI_Quadrant = 99);

/**
	@return		The appropriate ::NTV2InputXptID for the given SDI Output, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inSDIOutput		Specifies the SDI Output widget of interest, expressed as an ::NTV2Channel (a zero-based index value).
	@param[in]	inIsDS2			Specify true to obtain the DS2 input crosspoint. Defaults to false (the DS1 input).
**/
AJAExport NTV2InputXptID		GetSDIOutputInputXpt (const NTV2Channel inSDIOutput,  const bool inIsDS2 = false);

/**
	@return		The appropriate mixer's ::NTV2OutputCrosspointID for the given ::NTV2Channel, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inChannel		Specifies the ::NTV2Channel of interest. Mixer 1 is used for channels 1 & 2, mixer 2 for channels 3 & 4, etc.
	@param[in]	inIsKey			Specify true to obtain the key output crosspoint. Defaults to false (the video output crosspoint).
	@see		::GetMixerFGInputXpt, ::GetMixerBGInputXpt, \ref widget_mixkey
**/
AJAExport NTV2OutputXptID		GetMixerOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsKey = false);

/**
	@return		The appropriate mixer's foreground ::NTV2InputXptID for the given ::NTV2Channel, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inChannel		Specifies the ::NTV2Channel of interest. Mixer 1 is used for channels 1 & 2, mixer 2 for channels 3 & 4, etc.
	@param[in]	inIsKey			Specify true to obtain the key input crosspoint. Defaults to false (the video input).
	@see		::GetMixerBGInputXpt, ::GetMixerOutputXptFromChannel, \ref widget_mixkey
**/
AJAExport NTV2InputXptID		GetMixerFGInputXpt (const NTV2Channel inChannel,  const bool inIsKey = false);

/**
	@return		The appropriate mixer's background ::NTV2InputXptID for the given ::NTV2Channel, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
	@param[in]	inChannel		Specifies the ::NTV2Channel of interest. Mixer 1 is used for channels 1 & 2, mixer 2 for channels 3 & 4, etc.
	@param[in]	inIsKey			Specify true to obtain the key input crosspoint. Defaults to false (the video input).
	@see		::GetMixerFGInputXpt, ::GetMixerOutputXptFromChannel, \ref widget_mixkey
**/
AJAExport NTV2InputXptID		GetMixerBGInputXpt (const NTV2Channel inChannel,  const bool inIsKey = false);

/**
    @return		The appropriate ::NTV2InputXptID for the given TSI Muxer widget, or ::NTV2_INPUT_CROSSPOINT_INVALID upon failure.
    @param[in]	inTSIMuxer		Specifies the 425Mux widget of interest, expressed as an ::NTV2Channel (a zero-based index value).
    @param[in]	inLinkB			Specify true to obtain the "B" input crosspoint. Defaults to false, the "A" input.
**/
AJAExport NTV2InputXptID	     GetTSIMuxInputXptFromChannel(const NTV2Channel inTSIMuxer, const bool inLinkB = false);

/**
    @return		The appropriate ::NTV2OutputXptID for the given TSI Muxer, or ::NTV2_OUTPUT_CROSSPOINT_INVALID upon failure.
    @param[in]	inTSIMuxer		Specifies the 425Mux widget of interest, expressed as an ::NTV2Channel (a zero-based index value).
    @param[in]	inLinkB			Specify true to obtain the "B" output crosspoint. Defaults to false, the "A" output.
    @param[in]	inIsRGB			Specify true to obtain the RGB output crosspoint. Defaults to false, the YUV output.
**/
AJAExport NTV2OutputXptID		GetTSIMuxOutputXptFromChannel (const NTV2Channel inTSIMuxer, const bool inLinkB = false, const bool inIsRGB = false);


//	Stream operators
AJAExport std::ostream & operator << (std::ostream & inOutStream, const CNTV2SignalRouter & inObj);

#if !defined (NTV2_DEPRECATE_12_5)
	//	Stream operators
	AJAExport NTV2_DEPRECATED_f(std::ostream & operator << (std::ostream & inOutStream, const NTV2RoutingEntry & inObj));
	AJAExport NTV2_DEPRECATED_f(std::ostream & operator << (std::ostream & inOutStream, const NTV2RoutingTable & inObj));

	//	Per-widget input crosspoint selection register/mask/shift values
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer1InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer1BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer2BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer3InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer3BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer4InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer4BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer5InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer5BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer6InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer6BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer7InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer7BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer8InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameBuffer8BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC1VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC1KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC2VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC2KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC3VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC3KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC4VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC4KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC5VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC5KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC6VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC6KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC7VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC7KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC8VidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC8KeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT1InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT3InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT4InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT5InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT6InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT7InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetLUT8InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut1StandardSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut2StandardSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut3StandardSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut4StandardSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut1InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut1InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut2InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut2InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut3InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut3InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut4InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut4InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut5InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut5InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut6InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut6InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut7InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut7InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut8InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetSDIOut8InputDS2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn1InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn1DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn2InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn2DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn3InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn3DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn4InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn4DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn5InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn5DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn6InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn6DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn7InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn7DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn8InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkIn8DSInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut1InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut3InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut4InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut5InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut6InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut7InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetDualLinkOut8InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer1BGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer1BGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer1FGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer1FGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer2BGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer2BGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer2FGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer2FGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer3BGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer3BGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer3FGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer3FGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer4BGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer4BGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer4FGKeyInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetMixer4FGVidInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetHDMIOutInputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetHDMIOutQ1InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetHDMIOutQ2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetHDMIOutQ3InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetHDMIOutQ4InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetXpt4KDCQ1InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetXpt4KDCQ2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetXpt4KDCQ3InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetXpt4KDCQ4InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux1AInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux1BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux2AInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux2BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux3AInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux3BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux4AInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	Get425Mux4BInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.

	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetAnalogOutInputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetIICT2InputSelectEntry (void));			///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetAnalogOutCompositeOutSelectEntry(void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetStereoLeftInputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetStereoRightInputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetProAmpInputSelectEntry (void));			///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetIICT1InputSelectEntry (void));			///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetWaterMarker1InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetWaterMarker2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetUpdateRegisterSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetConversionMod2InputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCompressionModInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetConversionModInputSelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetCSC1KeyFromInput2SelectEntry (void));	///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameSync2InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
	AJAExport NTV2_DEPRECATED_f(const NTV2RoutingEntry &	GetFrameSync1InputSelectEntry (void));		///< @deprecated	NTV2RoutingEntry is deprecated.
#endif	//	!defined (NTV2_DEPRECATE_12_5)


#if !defined (NTV2_DEPRECATE)
	typedef CNTV2SignalRouter	CXena2Routing;						///< @deprecated	Use CNTV2SignalRouter instead.

	#define	GetXptLUTInputSelectEntry			GetLUT1InputSelectEntry
	#define GetDuallinkIn1InputSelectEntry		GetDualLinkIn1InputSelectEntry
	#define DuallinkIn1InputSelectEntry			DualLinkIn1InputSelectEntry
	#define	DuallinkOutInputSelectEntry			DualLinkOut1InputSelectEntry
	#define	GetDuallinkOutInputSelectEntry		GetDualLinkOut1InputSelectEntry
	#define	DuallinkOut2InputSelectEntry		DualLinkOut2InputSelectEntry
	#define	GetDuallinkOut2InputSelectEntry		GetDualLinkOut2InputSelectEntry
	#define	DuallinkOut3InputSelectEntry		DualLinkOut3InputSelectEntry
	#define	GetDuallinkOut3InputSelectEntry		GetDualLinkOut3InputSelectEntry
	#define	DuallinkOut4InputSelectEntry		DualLinkOut4InputSelectEntry
	#define	GetDuallinkOut4InputSelectEntry		GetDualLinkOut4InputSelectEntry
	#define	DuallinkOut5InputSelectEntry		DualLinkOut5InputSelectEntry
	#define	GetDuallinkOut5InputSelectEntry		GetDualLinkOut5InputSelectEntry
	#define	DuallinkOut6InputSelectEntry		DualLinkOut6InputSelectEntry
	#define	GetDuallinkOut6InputSelectEntry		GetDualLinkOut6InputSelectEntry
	#define	DuallinkOut7InputSelectEntry		DualLinkOut7InputSelectEntry
	#define	GetDuallinkOut7InputSelectEntry		GetDualLinkOut7InputSelectEntry
	#define	DuallinkOut8InputSelectEntry		DualLinkOut8InputSelectEntry
	#define	GetDuallinkOut8InputSelectEntry		GetDualLinkOut8InputSelectEntry
	#define	MixerBGKeyInputSelectEntry			Mixer1BGKeyInputSelectEntry
	#define	MixerBGVidInputSelectEntry			Mixer1BGVidInputSelectEntry
	#define	MixerFGKeyInputSelectEntry			Mixer1FGKeyInputSelectEntry
	#define	MixerFGVidInputSelectEntry			Mixer1FGVidInputSelectEntry
	#define	GetMixerBGKeyInputSelectEntry		GetMixer1BGKeyInputSelectEntry
	#define	GetMixerBGVidInputSelectEntry		GetMixer1BGVidInputSelectEntry
	#define	GetMixerFGKeyInputSelectEntry		GetMixer1FGKeyInputSelectEntry
	#define	GetMixerFGVidInputSelectEntry		GetMixer1FGVidInputSelectEntry

	#define	XptLUTInputSelectEntry				LUT1InputSelectEntry
	#define	XptLUT2InputSelectEntry				LUT2InputSelectEntry
	#define	XptLUT3InputSelectEntry				LUT3InputSelectEntry
	#define	XptLUT4InputSelectEntry				LUT4InputSelectEntry
	#define	XptLUT5InputSelectEntry				LUT5InputSelectEntry
	#define	XptLUT6InputSelectEntry				LUT6InputSelectEntry
	#define	XptLUT7InputSelectEntry				LUT7InputSelectEntry
	#define	XptLUT8InputSelectEntry				LUT8InputSelectEntry

	#define	GetXptLUT1InputSelectEntry			GetLUT1InputSelectEntry
	#define	GetXptLUT2InputSelectEntry			GetLUT2InputSelectEntry
	#define	GetXptLUT3InputSelectEntry			GetLUT3InputSelectEntry
	#define	GetXptLUT4InputSelectEntry			GetLUT4InputSelectEntry
	#define	GetXptLUT5InputSelectEntry			GetLUT5InputSelectEntry
	#define	GetXptLUT6InputSelectEntry			GetLUT6InputSelectEntry
	#define	GetXptLUT7InputSelectEntry			GetLUT7InputSelectEntry
	#define	GetXptLUT8InputSelectEntry			GetLUT8InputSelectEntry

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer1InputSelectEntry);			///< @deprecated	Use GetFrameBuffer1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer2InputSelectEntry);			///< @deprecated	Use GetFrameBuffer2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer3InputSelectEntry);			///< @deprecated	Use GetFrameBuffer3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer4InputSelectEntry);			///< @deprecated	Use GetFrameBuffer4InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer5InputSelectEntry);			///< @deprecated	Use GetFrameBuffer5InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer6InputSelectEntry);			///< @deprecated	Use GetFrameBuffer6InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer7InputSelectEntry);			///< @deprecated	Use GetFrameBuffer7InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameBuffer8InputSelectEntry);			///< @deprecated	Use GetFrameBuffer8InputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC1VidInputSelectEntry);				///< @deprecated	Use GetCSC1VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC1KeyInputSelectEntry);				///< @deprecated	Use GetCSC1KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC2VidInputSelectEntry);				///< @deprecated	Use GetCSC2VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC2KeyInputSelectEntry);				///< @deprecated	Use GetCSC2KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC3VidInputSelectEntry);				///< @deprecated	Use GetCSC3VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC3KeyInputSelectEntry);				///< @deprecated	Use GetCSC3KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC4VidInputSelectEntry);				///< @deprecated	Use GetCSC4VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC4KeyInputSelectEntry);				///< @deprecated	Use GetCSC4KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC5VidInputSelectEntry);				///< @deprecated	Use GetCSC5VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC5KeyInputSelectEntry);				///< @deprecated	Use GetCSC5KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC6VidInputSelectEntry);				///< @deprecated	Use GetCSC6VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC6KeyInputSelectEntry);				///< @deprecated	Use GetCSC6KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC7VidInputSelectEntry);				///< @deprecated	Use GetCSC7VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC7KeyInputSelectEntry);				///< @deprecated	Use GetCSC7KeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC8VidInputSelectEntry);				///< @deprecated	Use GetCSC8VidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC8KeyInputSelectEntry);				///< @deprecated	Use GetCSC8KeyInputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUTInputSelectEntry);				///< @deprecated	Use GetLUT1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT2InputSelectEntry);				///< @deprecated	Use GetLUT2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT3InputSelectEntry);				///< @deprecated	Use GetLUT3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT4InputSelectEntry);				///< @deprecated	Use GetLUT4InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT5InputSelectEntry);				///< @deprecated	Use GetLUT5InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT6InputSelectEntry);				///< @deprecated	Use GetLUT6InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT7InputSelectEntry);				///< @deprecated	Use GetLUT7InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry XptLUT8InputSelectEntry);				///< @deprecated	Use GetLUT8InputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut1StandardSelectEntry);			///< @deprecated	Use GetSDIOut1StandardSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut2StandardSelectEntry);			///< @deprecated	Use GetSDIOut2StandardSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut3StandardSelectEntry);			///< @deprecated	Use GetSDIOut3StandardSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut4StandardSelectEntry);			///< @deprecated	Use GetSDIOut4StandardSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut1InputSelectEntry);				///< @deprecated	Use GetSDIOut1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut1InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut1InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut2InputSelectEntry);				///< @deprecated	Use GetSDIOut2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut2InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut2InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut3InputSelectEntry);				///< @deprecated	Use GetSDIOut3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut3InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut3InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut4InputSelectEntry);				///< @deprecated	Use GetSDIOut4InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut4InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut4InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut5InputSelectEntry);				///< @deprecated	Use GetSDIOut5InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut5InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut5InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut6InputSelectEntry);				///< @deprecated	Use GetSDIOut6InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut6InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut6InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut7InputSelectEntry);				///< @deprecated	Use GetSDIOut7InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut7InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut7InputDS2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut8InputSelectEntry);				///< @deprecated	Use GetSDIOut8InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry SDIOut8InputDS2SelectEntry);			///< @deprecated	Use GetSDIOut8InputDS2SelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn1InputSelectEntry);			///< @deprecated	Use GetDualLinkIn1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn1DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn1DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn2InputSelectEntry);			///< @deprecated	Use GetDualLinkIn2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn2DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn2DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn3InputSelectEntry);			///< @deprecated	Use GetDualLinkIn3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn3DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn3DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn4InputSelectEntry);			///< @deprecated	Use GetDualLinkIn4InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn4DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn4DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn5InputSelectEntry);			///< @deprecated	Use GetDualLinkIn5InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn5DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn5DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn6InputSelectEntry);			///< @deprecated	Use GetDualLinkIn6InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn6DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn6DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn7InputSelectEntry);			///< @deprecated	Use GetDualLinkIn7InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn7DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn7DSInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn8InputSelectEntry);			///< @deprecated	Use GetDualLinkIn8InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkIn8DSInputSelectEntry);			///< @deprecated	Use GetDualLinkIn8DSInputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut1InputSelectEntry);			///< @deprecated	Use GetDualLinkOut1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut2InputSelectEntry);			///< @deprecated	Use GetDualLinkOut2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut3InputSelectEntry);			///< @deprecated	Use GetDualLinkOut3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut4InputSelectEntry);			///< @deprecated	Use GetDualLinkOut4InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut5InputSelectEntry);			///< @deprecated	Use GetDualLinkOut5InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut6InputSelectEntry);			///< @deprecated	Use GetDualLinkOut6InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut7InputSelectEntry);			///< @deprecated	Use GetDualLinkOut7InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry DualLinkOut8InputSelectEntry);			///< @deprecated	Use GetDualLinkOut8InputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer1BGKeyInputSelectEntry);			///< @deprecated	Use GetMixer1BGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer1BGVidInputSelectEntry);			///< @deprecated	Use GetMixer1BGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer1FGKeyInputSelectEntry);			///< @deprecated	Use GetMixer1FGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer1FGVidInputSelectEntry);			///< @deprecated	Use GetMixer1FGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer2BGKeyInputSelectEntry);			///< @deprecated	Use GetMixer2BGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer2BGVidInputSelectEntry);			///< @deprecated	Use GetMixer2BGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer2FGKeyInputSelectEntry);			///< @deprecated	Use GetMixer2FGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer2FGVidInputSelectEntry);			///< @deprecated	Use GetMixer2FGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer3BGKeyInputSelectEntry);			///< @deprecated	Use GetMixer3BGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer3BGVidInputSelectEntry);			///< @deprecated	Use GetMixer3BGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer3FGKeyInputSelectEntry);			///< @deprecated	Use GetMixer3FGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer3FGVidInputSelectEntry);			///< @deprecated	Use GetMixer3FGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer4BGKeyInputSelectEntry);			///< @deprecated	Use GetMixer4BGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer4BGVidInputSelectEntry);			///< @deprecated	Use GetMixer4BGVidInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer4FGKeyInputSelectEntry);			///< @deprecated	Use GetMixer4FGKeyInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Mixer4FGVidInputSelectEntry);			///< @deprecated	Use GetMixer4FGVidInputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Xpt4KDCQ1InputSelectEntry);				///< @deprecated	Use GetXpt4KDCQ1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Xpt4KDCQ2InputSelectEntry);				///< @deprecated	Use GetXpt4KDCQ2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Xpt4KDCQ3InputSelectEntry);				///< @deprecated	Use GetXpt4KDCQ3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry Xpt4KDCQ4InputSelectEntry);				///< @deprecated	Use GetXpt4KDCQ4InputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry HDMIOutQ1InputSelectEntry);				///< @deprecated	Use GetHDMIOutQ1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry HDMIOutQ2InputSelectEntry);				///< @deprecated	Use GetHDMIOutQ2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry HDMIOutQ3InputSelectEntry);				///< @deprecated	Use GetHDMIOutQ3InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry HDMIOutQ4InputSelectEntry);				///< @deprecated	Use GetHDMIOutQ4InputSelectEntry instead.

	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CompressionModInputSelectEntry);		///< @deprecated	Use GetCompressionModInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry ConversionModInputSelectEntry);			///< @deprecated	Use GetConversionModInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry CSC1KeyFromInput2SelectEntry);			///< @deprecated	Use GetCSC1KeyFromInput2SelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameSync2InputSelectEntry);			///< @deprecated	Use GetFrameSync2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry FrameSync1InputSelectEntry);			///< @deprecated	Use GetFrameSync1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry AnalogOutInputSelectEntry);				///< @deprecated	Use GetAnalogOutInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry ProAmpInputSelectEntry);				///< @deprecated	Use GetProAmpInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry IICT1InputSelectEntry);					///< @deprecated	Use GetIICT1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry WaterMarker1InputSelectEntry);			///< @deprecated	Use GetWaterMarker1InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry WaterMarker2InputSelectEntry);			///< @deprecated	Use GetWaterMarker2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry UpdateRegisterSelectEntry);				///< @deprecated	Use GetUpdateRegisterSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry ConversionMod2InputSelectEntry);		///< @deprecated	Use GetConversionMod2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry HDMIOutInputSelectEntry);				///< @deprecated	Use GetHDMIOutInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry IICT2InputSelectEntry);					///< @deprecated	Use GetIICT2InputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry AnalogOutCompositeOutSelectEntry);		///< @deprecated	Use GetAnalogOutCompositeOutSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry StereoLeftInputSelectEntry);			///< @deprecated	Use GetStereoLeftInputSelectEntry instead.
	extern NTV2_DEPRECATED_v(const NTV2RoutingEntry StereoRightInputSelectEntry);			///< @deprecated	Use GetStereoRightInputSelectEntry instead.
#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2SIGNALROUTER_H
