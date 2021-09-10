/* SPDX-License-Identifier: MIT */
/**
	@file		udp_socket.cpp
	@brief		Implements the AJAUDPSocket class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/network/udp_socket.h"
#include "ajabase/system/debug.h"
#include <errno.h>
#include <string.h>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using std::string;


/////////////////////////////
// Defines
/////////////////////////////
#define DEBUG_UDP_OPERATION   0


/////////////////////////////
// Definitions
/////////////////////////////
///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAUDPSocket::AJAUDPSocket(void)
{
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAUDPSocket::~AJAUDPSocket(void)
{
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAStatus
AJAUDPSocket::Open(const string& ipAddress, uint16_t port)
{
	if ((true == IsInstantiated()) && (-1 == mSocket))
	{
		if (-1 != (mSocket = (int) socket(AF_INET, SOCK_DGRAM, 0)))
		{
			if (0 == ipAddress.length())
			{
				mSocketAddress.sin_addr.s_addr = INADDR_ANY;
			}
			else
			{
				mSocketAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());
			}

			mSocketAddress.sin_family = AF_INET;
			mSocketAddress.sin_port   = htons(port);

			if (0 == bind(
						mSocket,
						(struct sockaddr*) &mSocketAddress,
						sizeof(struct sockaddr_in)))
			{
#if DEBUG_UDP_OPERATION
				cerr << __FUNCTION__
					<< ": Socket created and bound"
					<< endl;
#endif
                return AJA_STATUS_SUCCESS;
			}

#if DEBUG_UDP_OPERATION
			cerr << __FUNCTION__
				<< ": Unable to bind the socket (errno:"
				<< strerror(errno)
				<< ")"
				<< endl;
#endif
			Close();
		}
		else
		{
#if DEBUG_UDP_OPERATION
			cerr << __FUNCTION__
				<< ": Unable to create a socket (errno:"
				<< strerror(errno)
				<< ")"
				<< endl;
#endif
		}
	}
    return AJA_STATUS_FAIL;
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJAUDPSocket::Poll(
				uint8_t*            pData,
				uint32_t            dataLength,
				struct sockaddr_in& client,
				int                 timeout)
{
	int retVal = 0;

	if (-1 != mSocket)
	{
#if defined(AJA_LINUX) || defined(AJA_MAC)
		struct pollfd fds[1];

		fds[0].fd     = mSocket;
		fds[0].events = POLLIN;

		if (0 < (retVal = poll(fds, 1, timeout)))
		{
			if (fds[0].revents & POLLIN)
			{
				return (Read(pData, dataLength, client));
			}
			else
			{
				AJA_REPORT(
					0,
					AJA_DebugSeverity_Warning,
					"AJAUDPSocket::Poll (errno:%d)",
					errno);
#if DEBUG_UDP_OPERATION
				cerr << __FUNCTION__
					<< ": poll event (errno:"
					<< errno
					<< ")"
					<< endl;
#endif
				return (0);
			}
		}
		else if (0 == retVal)
		{
			// Timed-out
#if DEBUG_UDP_OPERATION
			cerr << __FUNCTION__ << ": time-out" << endl;
#endif
		}
		else
		{
			AJA_REPORT(
				0,
				AJA_DebugSeverity_Error,
				"AJAUDPSocket::Poll failed (errno:%d)",
				errno);
#if DEBUG_UDP_OPERATION
			cerr << __FUNCTION__
				<< ": poll errno:"
				<< errno
				<< " (retVal:"
				<< retVal
				<< ")"
				<< endl;
#endif
		}
#endif
	}
	return uint32_t(retVal);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJAUDPSocket::Read(uint8_t* pData, uint32_t dataLength, struct sockaddr_in& client)
{
	socklen_t socketLength = sizeof(struct sockaddr_in);
	int       bytesReceived = 0;

	if (-1 != mSocket)
	{
		if (0 >= (bytesReceived =
					recvfrom(
						mSocket,
						(char*) pData,
						dataLength,
						0,
						(struct sockaddr*) &client,
						&socketLength)))
		{
			if (0 != bytesReceived)
			{
				AJA_REPORT(
					0,
					AJA_DebugSeverity_Error,
					"AJAUDPSocket::Read failed (errno:%d)",
					errno);
#if DEBUG_UDP_OPERATION
				cerr << __FUNCTION__
					<< ": recvfrom errno:"
					<< errno
					<< endl;
#endif
			}
		}
	}
	return uint32_t(bytesReceived);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJAUDPSocket::Write(
					const uint8_t*      pData,
					uint32_t            dataLength,
					struct sockaddr_in& targetAddress)
{
	int bytesSent = 0;

	if (-1 != mSocket)
	{
		if (-1 == (bytesSent = sendto(
									mSocket,
									(char*) pData,
									(int) dataLength,
									0,
									(struct sockaddr*) &targetAddress,
									sizeof(struct sockaddr_in))))
		{
#if DEBUG_UDP_OPERATION
			cerr << __FUNCTION__
				<< ": sendto errno:"
				<< errno
				<< endl;
#endif
		}
	}
	return uint32_t(bytesSent);
}
