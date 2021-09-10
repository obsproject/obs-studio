/* SPDX-License-Identifier: MIT */
/**
	@file		tcp_socket.h
	@brief		Declares the AJATCPSocket class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TCP_SOCKET_H
#define AJA_TCP_SOCKET_H

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/network/ip_socket.h"


/////////////////////////////
// Declarations
/////////////////////////////
class AJA_EXPORT AJATCPSocket : public AJAIPSocket
{
	public:
		AJATCPSocket(void);
		virtual ~AJATCPSocket(void);

		AJAStatus Open(const std::string& ipAddress, uint16_t port);

		AJAStatus Connect(const std::string& ipAddress, uint16_t port);
		AJAStatus Listen(void);
		int       Accept(void);

        uint32_t Read(uint8_t* pData, uint32_t dataLength);
        uint32_t Write(const uint8_t* pData, uint32_t dataLength);

	private:
        uint32_t Read(uint8_t* pData, uint32_t dataLength, struct sockaddr_in& client);
        uint32_t Write(const uint8_t* pData, uint32_t dataLength, struct sockaddr_in& targetAddress);
};


#endif	//	AJA_TCP_SOCKET_H
