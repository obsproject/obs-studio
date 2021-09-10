/* SPDX-License-Identifier: MIT */
/**
	@file		tcp_socket.cpp
	@brief		Implements the AJATCPSocket class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/network/tcp_socket.h"
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
#define DEBUG_TCP_OPERATION   0
#define MAX_PENDING           10


/////////////////////////////
// Definitions
/////////////////////////////
///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJATCPSocket::AJATCPSocket(void)
{
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJATCPSocket::~AJATCPSocket(void)
{
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAStatus
AJATCPSocket::Open(const string& ipAddress, uint16_t port)
{
	if ((true == IsInstantiated()) && (-1 == mSocket))
	{
		if (-1 != (mSocket = (int) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
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
#if DEBUG_TCP_OPERATION
				cerr << __FUNCTION__
					<< ": Socket created and bound"
					<< endl;
#endif
				return (AJA_STATUS_SUCCESS);
			}

#if DEBUG_TCP_OPERATION
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
#if DEBUG_TCP_OPERATION
			cerr << __FUNCTION__
				<< ": Unable to create a socket (errno:"
				<< strerror(errno)
				<< ")"
				<< endl;
#endif
		}
	}
	return (AJA_STATUS_FAIL);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAStatus
AJATCPSocket::Connect(const string& ipAddress, uint16_t port)
{
	struct sockaddr_in serverAddress;

	if ((-1 != mSocket) && (0 != ipAddress.length()))
	{
		memset(&serverAddress, 0, sizeof(struct sockaddr_in));
		serverAddress.sin_family      = AF_INET;
		serverAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());
		serverAddress.sin_port        = htons(port);

		if (0 == connect(
					mSocket,
					(struct sockaddr*) &serverAddress,
					sizeof(struct sockaddr_in)))
		{
			return (AJA_STATUS_SUCCESS);
		}
	}
	return (AJA_STATUS_FAIL);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAStatus
AJATCPSocket::Listen(void)
{
	if (-1 != mSocket)
	{
		if (0 == listen(mSocket, MAX_PENDING))
		{
			return (AJA_STATUS_SUCCESS);
		}
	}
	return (AJA_STATUS_FAIL);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
int
AJATCPSocket::Accept(void)
{
	if (-1 != mSocket)
	{
		struct sockaddr_in client;
		socklen_t          length = sizeof(struct sockaddr_in);
		int                sock;

		memset(&client, 0, sizeof(struct sockaddr_in));

        if ((sock = (int)accept(mSocket, (struct sockaddr*) &client, &length)) < 0)
		{
			// If we have an EINVAL error, then either the address length
			// is invalid, or we are not listening to connections -- which
			// likely means that we shutting down.
			if (errno != EINVAL)
			{
				cerr << __FUNCTION__
					<< ": Did not accept a connect-request from "
					<< inet_ntoa(client.sin_addr)
					<< " (errno:"
					<< errno
					<< ")"
					<< endl;
			}
			else
			{
				cout << __FUNCTION__
					<< ": Not listening for connections"
					<< endl;
			}
		}
		else
		{
			cout << __FUNCTION__
				<< ": Accepted the connect-request from "
				<< inet_ntoa(client.sin_addr)
				<< ", socket "
				<< sock
				<< endl;

			return (sock);
		}
	}
	return (-1);
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
uint32_t
AJATCPSocket::Read(uint8_t* pData, uint32_t dataLength)
{
	int bytesReceived = 0;

	if (-1 != mSocket)
	{
		if (0 >= (bytesReceived =
					recv(
						mSocket,
						(char*) pData,
						dataLength,
						0)))
		{
			if (0 != bytesReceived)
			{
				AJA_REPORT(
					0,
					AJA_DebugSeverity_Error,
					"AJATCPSocket::Read failed (errno:%d)",
					errno);
#if DEBUG_TCP_OPERATION
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
AJATCPSocket::Write(const uint8_t* pData, uint32_t dataLength)
{
	int bytesSent = 0;

	if (-1 != mSocket)
	{
		if (-1 == (bytesSent = send(
									mSocket,
									(char*) pData,
									(int) dataLength,
									0)))
		{
#if DEBUG_TCP_OPERATION
			cerr << __FUNCTION__
				<< ": sendto errno:"
				<< errno
				<< endl;
#endif
		}
	}
	return uint32_t(bytesSent);
}

// These are here to silence the warning about hiding overloaded virtual functions
uint32_t
AJATCPSocket::Read(uint8_t* pData, uint32_t dataLength, struct sockaddr_in& client)
{
    AJA_UNUSED(client);
    return Read(pData, dataLength);
}

uint32_t
AJATCPSocket::Write(const uint8_t* pData, uint32_t dataLength, struct sockaddr_in& targetAddress)
{
    AJA_UNUSED(targetAddress);
    return Write(pData, dataLength);
}

//////////////////////// End of tcp_socket.cpp //////////////////////
