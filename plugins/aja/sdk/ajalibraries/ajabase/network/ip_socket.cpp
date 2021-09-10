/* SPDX-License-Identifier: MIT */
/**
	@file		ip_socket.cpp
	@brief		Implements the AJAIPSocket class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/network/ip_socket.h"
#include "ajabase/system/debug.h"
#include <errno.h>
#include <iostream>
#if defined(AJA_LINUX)
#include <unistd.h>
#endif
#if defined(AJA_WINDOWS)
#include <WS2tcpip.h>
#endif

using std::cout;
using std::endl;
using std::string;

/////////////////////////////
// Defines
/////////////////////////////
#if defined(AJA_WINDOWS)
#	define CLOSE_SOCKET(x)       closesocket(x)
#else
#	define CLOSE_SOCKET(x)       close(x)
#endif

#define DEBUG_IP_INSTANTIATION   0
#define DEBUG_SOCKET_OPERATION   0
#define DEBUG_SOCKET_OPTION      0
#define DEBUG_GET_IP_ADDRESSES   0


/////////////////////////////
// Statics
/////////////////////////////
AJA_EXPORT AJALock  AJAIPSocket::mMutex("");
AJA_EXPORT uint32_t AJAIPSocket::mInstantiationCount = 0;


/////////////////////////////
// Definitions
/////////////////////////////
///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAIPSocket::AJAIPSocket(void)
{
	mSocketLength = sizeof(struct sockaddr_in);
	mSocket       = -1;
	Initialize();
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAIPSocket::~AJAIPSocket(void)
{
	Close();
	Deinitialize();
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool
AJAIPSocket::Initialize(void)
{
	mMutex.Lock();

#if defined(AJA_WINDOWS)
	WORD    versionRequested;
	WSADATA wsaData;
	int     err;

	versionRequested = MAKEWORD(2, 2);

	if (0 != (err = WSAStartup(versionRequested, &wsaData)))
	{
		AJA_REPORT(AJA_DebugUnit_Enumeration, AJA_DebugSeverity_Info,
			"AJAIPSocket::Initialize() Winsock 2.2 DLL not found");
	}
	// From the Microsoft documentation:
	// Note that if the DLL supports versions greater
	// than 2.2 in addition to 2.2, it will still return
	// 2.2 in wVersion since that is the version we requested
	else if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		// Tell the user that we could not find a usable WinSock DLL
		AJA_REPORT(AJA_DebugUnit_Enumeration, AJA_DebugSeverity_Info,
			"AJAIPSocket::Initialize() "
			"Could not find usable Winsock 2.2 DLL");
		WSACleanup();
	}
	else
	{
		mInstantiationCount++;
	}

#else // Linux and Mac

	mInstantiationCount++;

#endif
#if DEBUG_IP_INSTANTIATION
	cout << __FUNCTION__
		<<": The network stack instantiation count:"
		<< mInstantiationCount
		<< endl;
#endif

	mMutex.Unlock();
	return (0 != mInstantiationCount) ? true : false;
}



///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool
AJAIPSocket::Deinitialize(void)
{
	mMutex.Lock();

	if (0 != mInstantiationCount)
	{
		mInstantiationCount--;

		if (0 == mInstantiationCount)
		{
#if defined(AJA_WINDOWS)
			WSACleanup();
#endif
		}

#if DEBUG_IP_INSTANTIATION
		cout << __FUNCTION__
			<<": The network stack instantiation count:"
			<< mInstantiationCount
			<< endl;
#endif

	}
	mMutex.Unlock();
	return true;
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAStatus AJAIPSocket::Open(const string& ipAddress, uint16_t port)
{
    AJA_UNUSED(ipAddress);
    AJA_UNUSED(port);
    return AJA_STATUS_FAIL;
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool AJAIPSocket::IsOpen(void)
{
	return (-1 != mSocket);
}


///////////////////////////////////////////////////////////
// The arguments are one of:
//   SHUT_RD
//   SHUT_WR
//   SHUT_RDWR
///////////////////////////////////////////////////////////
bool
AJAIPSocket::Shutdown(int how)
{
	if (-1 != mSocket)
	{
		if (0 == shutdown(mSocket, how))
		{
			return true;
		}
#if DEBUG_SOCKET_OPERATION
		cout << __FUNCTION__
			<< ": shutdown errno:"
			<< errno
			<< endl;
#endif
	}
	return false;
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool
AJAIPSocket::Close(void)
{
	if (-1 != mSocket)
	{
		CLOSE_SOCKET(mSocket);
		mSocket = -1;
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool
AJAIPSocket::SetSocketOption(int option, const void* pValue)
{
	bool status = false;

	if ((-1 != mSocket) && (NULL != pValue))
	{
		switch (option)
		{
			case SO_BROADCAST:
			case SO_REUSEADDR:
			case SO_KEEPALIVE:
			case SO_SNDBUF:
			case SO_RCVBUF:
			{
				if (-1 != setsockopt(
							mSocket,
							SOL_SOCKET,
							option,
#if defined(AJA_WINDOWS)
							(char*) pValue,
#else
							(int*) pValue,
#endif
							sizeof(int)))
				{
					status = true;
				}
				break;
			}

			case SO_LINGER:
			{
				if (-1 != setsockopt(
							mSocket,
							SOL_SOCKET,
							option,
#if defined(AJA_WINDOWS)
							(char*) pValue,
#else
							(int*) pValue,
#endif
							sizeof(struct linger)))
				{
					status = true;
				}
				break;
			}

			case IP_ADD_MEMBERSHIP:
			case IP_DROP_MEMBERSHIP:
			{
				if (-1 != setsockopt(
							mSocket,
							IPPROTO_IP,
							option,
#if defined(AJA_WINDOWS)
							(char*) pValue,
#else
							(int*) pValue,
#endif
							sizeof(struct ip_mreq)))
				{
					status = true;
				}
				break;
			}

			default:
				AJA_REPORT(
					0,
					AJA_DebugSeverity_Error,
					"AJAIPSocket::SetSocketOptions unsupported option {%d)",
					option);
				break;
		}

		if (true != status)
		{
#if DEBUG_SOCKET_OPTION
			cout << __FUNCTION__
				<< ": setsockopt("
				<< option
				<< ") errno "
				<< errno
				<< endl;
#endif
			AJA_REPORT(
				0,
				AJA_DebugSeverity_Error,
				"AJAIPSocket::SetSocketOptions setsockopt %d failed",
				option);
		}
	}
	return (status);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool
AJAIPSocket::GetHostIPAddresses(
				IPAddressContainerType& ipv4AddressContainer,
				IPAddressContainerType& ipv6AddressContainer)
{
#if defined(AJA_WINDOWS)
	// ToDo
#else
	struct ifaddrs* pIFAddrContainer = NULL;
	struct ifaddrs* pIFAddr          = NULL;
	void*           pAddrPtr         = NULL;
	char            addressBuffer[INET6_ADDRSTRLEN];

	getifaddrs(&pIFAddrContainer);

	if (NULL != pIFAddrContainer)
	{
		for (pIFAddr = pIFAddrContainer; NULL != pIFAddr; pIFAddr = pIFAddr->ifa_next)
		{
			// Is this an IP4 address
			if (AF_INET == pIFAddr->ifa_addr->sa_family)
			{
				pAddrPtr = &((struct sockaddr_in*) pIFAddr->ifa_addr)->sin_addr;
				inet_ntop(AF_INET, pAddrPtr, addressBuffer, INET_ADDRSTRLEN);

				if (string(addressBuffer) != string("127.0.0.1"))
				{
#	if DEBUG_GET_IP_ADDRESSES
					cout << "Adding IPv4 \""
						<< pIFAddr->ifa_name
						<< "\", "
						<< addressBuffer
						<< endl;
#	endif
					ipv4AddressContainer[addressBuffer] = pIFAddr->ifa_name;
				}
			}
			// This could be an IP6 address, then
			else if (AF_INET6 == pIFAddr->ifa_addr->sa_family)
			{
				pAddrPtr = &((struct sockaddr_in*) pIFAddr->ifa_addr)->sin_addr;
				inet_ntop(AF_INET6, pAddrPtr, addressBuffer, INET6_ADDRSTRLEN);

				if ((string(addressBuffer) != string("::")) &&
					(string(addressBuffer) != string("0:0:0:0:0:0:0:0")))
				{
#	if DEBUG_GET_IP_ADDRESSES
					cout << "Adding IPv6 \""
						<< pIFAddr->ifa_name
						<< "\", "
						<< addressBuffer
						<< endl;
#	endif
					ipv6AddressContainer[addressBuffer] = pIFAddr->ifa_name;
				}
			}
		}
		freeifaddrs(pIFAddrContainer);
	}
#endif
	return (0 != (ipv4AddressContainer.size() + ipv6AddressContainer.size()));
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJAIPSocket::Poll(
				uint8_t*            pData,
				uint32_t            dataLength,
				struct sockaddr_in& client,
				int                 timeout)
{
    AJA_UNUSED(pData);
    AJA_UNUSED(dataLength);
    AJA_UNUSED(client);
    AJA_UNUSED(timeout);

	return (0);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJAIPSocket::Read(uint8_t* pData, uint32_t dataLength, struct sockaddr_in& client)
{
    AJA_UNUSED(pData);
    AJA_UNUSED(dataLength);
    AJA_UNUSED(client);
	return (0);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJAIPSocket::Write(
				const uint8_t*      pData,
				uint32_t            dataLength,
				struct sockaddr_in& targetAddress)
{
    AJA_UNUSED(pData);
    AJA_UNUSED(dataLength);
    AJA_UNUSED(targetAddress);
	return (0);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
bool
AJAIPSocket::IsInstantiated(void)
{
	return (0 != mInstantiationCount);
}
