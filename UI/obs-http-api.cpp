#include "obs-http-api.h"

#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <chrono>
#include <ratio>
#include <string>
#include <sstream>
#include <mutex>

#ifdef _WIN32
#include <filesystem>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#endif

#include <fstream>
#include "ui-config.h"

#ifdef _WIN32
////////////////////////////////////////////////////////////////////////////////////
#define MAX_HTTP_BUFFER_SIZE (MAX_STREAM_URL_LENGTH + MAX_STREAM_KEY_LENGTH + 1024)
HANDLE obs_http_thread;
volatile bool obs_http_is_exit = false;
volatile bool obs_http_exited = false;
char obs_local_ip_addr[INET_ADDRSTRLEN] = {0};
char obs_http_response_buf[MAX_HTTP_BUFFER_SIZE] = {0};
char obs_http_request_buf[MAX_HTTP_BUFFER_SIZE] = {0};


static void json_parser(char *string, const char *json)
{
	bool qin_flag = false;
	size_t len = strlen(json);
	int idx = 0;
	for (size_t i = 0; i < len; i++) {
		if (json[i] != ' ' && json[i] != '\r' && json[i] != '\n') {
			if (json[i] == '"') {
				if (qin_flag == 0)
					qin_flag = 1;
				else
					qin_flag = 0;
			} else {
				string[idx] = json[i];
				idx++;
			}

			if (json[i] == ' ' && qin_flag) {
				string[idx] = json[i];
				idx++;
			}
		}
	}
}

static OBSHttpApiValue* json_get_values(const char* json_object)
{
	char szJson[MAX_HTTP_BUFFER_SIZE] = {0};
	size_t json_object_size = sizeof(OBSHttpApiValue);
	OBSHttpApiValue *obsHttpApiValue =
		(OBSHttpApiValue *)malloc(json_object_size);
	if (obsHttpApiValue != NULL) 
		memset(obsHttpApiValue, 0, json_object_size);

	if (json_object != NULL) {
		json_parser(szJson, json_object);
		char items[10][256] = {0};
		size_t idx = 0;
		char *tmp_item = strtok(szJson, ",");
		while (tmp_item != NULL) {
			strncpy(items[idx], tmp_item, strlen(tmp_item));
			tmp_item = strtok(NULL, ",");
			idx++;
		}

		for (size_t i = 0; i < idx; i++) {
			char *tmp_title = strtok(items[i], ":");
			while (tmp_title != NULL) {
				if (!strcmp(tmp_title, "streamUrl")) {
					tmp_title = strtok(NULL, "");
					strncpy(obsHttpApiValue->streamUrl, tmp_title,
						strlen(tmp_title));
				} else if (!strcmp(tmp_title, "streamKey")) {
					tmp_title = strtok(NULL, "");
					strncpy(obsHttpApiValue->streamKey, tmp_title,
						strlen(tmp_title));
				} else {
					tmp_title = strtok(NULL, "");
				}
				idx++;
			}
		}
	}
	return obsHttpApiValue;
}

static DWORD CALLBACK obs_http_internal_server_thread(LPVOID param)
{
	WSADATA wsa_data;
	SOCKET sockSvr;
	SOCKET sockSS;
	int len;
	struct sockaddr_in addrSockSvr;
	struct sockaddr_in addrSockClt;
	char szBuffer[1024] = {0};
	char szBufIP[1024] = {0};
	long ret = 0;
	BOOL valid = FALSE;

	if (::WSAStartup(MAKEWORD(2, 0), &wsa_data) != 0) {
		return 1;
	}

	if (::gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR) {
		::WSACleanup();
		return 1;
	}
	int status;
	struct addrinfo hints, *res, *p;
	struct addrinfo *servinfo;
	::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	status = ::getaddrinfo(szBuffer, NULL, &hints, &servinfo);

	//char ipstr[INET_ADDRSTRLEN] = {0};

	if ((status = getaddrinfo(szBuffer, NULL, &hints, &res)) != 0) {
		return 1;
	}

	for (p = res; p != NULL; p = p->ai_next) {
		void *addr;
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv4->sin_addr);
		inet_ntop(p->ai_family, addr, obs_local_ip_addr,
			  sizeof obs_local_ip_addr);
		if (strcmp(obs_local_ip_addr, "127.0.0.1")) {
			break;
		}
	}

	freeaddrinfo(res);
	//config_set_string(App()->GlobalConfig(), "spoon", "localIP", ipstr);

	sockSvr = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sockSvr == INVALID_SOCKET) {
		// TODO: socket error alert
		::WSACleanup();
		return 1;
	}

	addrSockSvr.sin_family = AF_INET;
	addrSockSvr.sin_port = htons(9028);
	addrSockSvr.sin_addr.S_un.S_addr = INADDR_ANY;

	::setsockopt(sockSvr, SOL_SOCKET, SO_REUSEADDR, (const char *)&valid,
		     sizeof(valid));
	if (::bind(sockSvr, (struct sockaddr *)&addrSockSvr,
		   sizeof(addrSockSvr)) != 0) {
		::WSACleanup();
		return 1;
	}

	if (::listen(sockSvr, 5) != 0) {
		::WSACleanup();
		return 1;
	}

	obs_http_is_exit = false; 
	while (!obs_http_is_exit) {
		len = sizeof(addrSockClt);
		sockSS = ::accept(sockSvr, (struct sockaddr *)&addrSockClt,
				  &len);

		if (sockSS == INVALID_SOCKET) {
			//printf("Accept Error No : %d", WSAGetLastError());
			return 1;
		}

		memset(obs_http_request_buf, 0, sizeof(obs_http_request_buf));
		::recv(sockSS, obs_http_request_buf, sizeof(obs_http_request_buf), 0);

		char *strRequest = strtok(obs_http_request_buf, "{");
		strRequest = strtok(NULL, "}");

		OBSHttpApiValue *streamValue = json_get_values(strRequest);

		FILE *file = fopen("OBS_HTTP_API.DAT", "w");
		fwrite(streamValue, sizeof(OBSHttpApiValue), 1, file);
		fclose(file);

		file = fopen("OBS_HTTP_API.DAT", "r");
		if (file) {
			OBSHttpApiValue streamCheckValue; 
			fread(&streamCheckValue, sizeof(OBSHttpApiValue), 1,
			      file);

			fclose(file);

			char bodyMessage[MAX_HTTP_BUFFER_SIZE] = {0};
			_snprintf_s(
				bodyMessage, MAX_HTTP_BUFFER_SIZE,
				MAX_HTTP_BUFFER_SIZE,
				"{ \"result\" : \"0\", \"streamUrl\" : \"%s\", \"streamKey\" : \"%s\" }\r\n",
				streamCheckValue.streamUrl, streamCheckValue.streamKey);

			int bodyMsgLen = (int)strlen(bodyMessage);
			memset(obs_http_response_buf, 0, MAX_HTTP_BUFFER_SIZE);
			_snprintf_s(obs_http_response_buf, MAX_HTTP_BUFFER_SIZE,
				    MAX_HTTP_BUFFER_SIZE,
				    "HTTP/1.0 200 OK\r\n"
				    "Content-Length: %d\r\n"
				    "Content-Type: text/json\r\n"
				    "\r\n%s",
				    bodyMsgLen, bodyMessage);

		} else {
			char bodyMessage[] = "{ \"result\" : \"1\" }\r\n";
			int bodyMsgLen = (int)strlen(bodyMessage);
			memset(obs_http_response_buf, 0, MAX_HTTP_BUFFER_SIZE);
			_snprintf_s(obs_http_response_buf, MAX_HTTP_BUFFER_SIZE,
				    MAX_HTTP_BUFFER_SIZE,
				    "HTTP/1.0 200 OK\r\n"
				    "Content-Length: %d\r\n"
				    "Content-Type: text/json\r\n"
				    "\r\n%s",
				    bodyMsgLen, bodyMessage);
		}
		::send(sockSS, obs_http_response_buf, (int)strlen(obs_http_response_buf), 0);

		::closesocket(sockSS);
	}

	::WSACleanup();

	obs_http_exited = true;
	return 0;
}

static inline BOOL spoon_http_run_service()
{
	obs_http_thread = CreateThread(NULL, 0, obs_http_internal_server_thread,
				    NULL, 0, NULL);
	return obs_http_thread != NULL;
}
////////////////////////////////////////////////////////////////////////////////////
#endif // _WIN32


bool OBSHttpApi::HttpApiStart()
{
	return spoon_http_run_service();
}

void OBSHttpApi::HttpApiExit()
{
	obs_http_is_exit = true;
	while (obs_http_exited) {
		Sleep(200);
	}
	return;
}

char* OBSHttpApi::GetLocalIpAddr()
{
	return obs_local_ip_addr;
}
