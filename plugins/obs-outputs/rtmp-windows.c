#ifdef _WIN32
#include "rtmp-stream.h"
#include <WinSock2.h>

void FatalSocketShutdown(struct rtmp_stream *stream)
{
	closesocket(stream->rtmp.m_sb.sb_socket);
	stream->rtmp.m_sb.sb_socket = -1;
	stream->write_buf_len = 0;
}

void *socket_thread_windows(void *data)
{
	bool canWrite = false;
	struct rtmp_stream *stream = data;

	int delayTime;
	size_t latencyPacketSize;
	uint64_t lastSendTime = 0;

	WSANETWORKEVENTS networkEvents;
	HANDLE hSendBacklogEvent;
	OVERLAPPED sendBacklogOverlapped;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	WSAEventSelect(stream->rtmp.m_sb.sb_socket, stream->socket_available_event, FD_READ|FD_WRITE|FD_CLOSE);

	hSendBacklogEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (stream->low_latency_mode)
	{
		delayTime = 1400.0f / (stream->write_buf_size / 1000.0f);
		latencyPacketSize = 1460;
	}
	else
	{
		latencyPacketSize = stream->write_buf_size;
		delayTime = 0;
	}

	if (!stream->disable_send_window_optimization)
	{
		memset (&sendBacklogOverlapped, 0, sizeof(sendBacklogOverlapped));
		sendBacklogOverlapped.hEvent = hSendBacklogEvent;
		idealsendbacklognotify (stream->rtmp.m_sb.sb_socket, &sendBacklogOverlapped, NULL);
	}
	else
		blog (LOG_INFO, "socket_thread_windows: Send window optimization disabled by user.");

	HANDLE hObjects[3];

	hObjects[0] = stream->socket_available_event;
	hObjects[1] = stream->buffer_has_data_event;
	hObjects[2] = hSendBacklogEvent;

	for (;;)
	{
		if (os_event_try(stream->stop_event) != EAGAIN)
		{
			pthread_mutex_lock(&stream->write_buf_mutex);
			if (stream->write_buf_len == 0)
			{
				//OSDebugOut (TEXT("Exiting on empty buffer.\n"));
				pthread_mutex_unlock(&stream->write_buf_mutex);
				break;
			}

			//OSDebugOut (TEXT("Want to exit, but %d bytes remain.\n"), curDataBufferLen);
			pthread_mutex_unlock(&stream->write_buf_mutex);
		}

		int status = WaitForMultipleObjects (3, hObjects, FALSE, INFINITE);
		if (status == WAIT_ABANDONED || status == WAIT_FAILED)
		{
			blog (LOG_ERROR, "socket_thread_windows: Aborting due to WaitForMultipleObjects failure");
			FatalSocketShutdown (stream);
			return NULL;
		}

		if (status == WAIT_OBJECT_0)
		{
			//Socket event
			if (WSAEnumNetworkEvents (stream->rtmp.m_sb.sb_socket, NULL, &networkEvents))
			{
				blog (LOG_ERROR, "socket_thread_windows: Aborting due to WSAEnumNetworkEvents failure, %d", WSAGetLastError());
				FatalSocketShutdown (stream);
				return NULL;
			}

			if (networkEvents.lNetworkEvents & FD_WRITE)
				canWrite = true;

			if (networkEvents.lNetworkEvents & FD_CLOSE)
			{
				if (lastSendTime)
				{
					uint32_t diff = (os_gettime_ns() / 1000000) - lastSendTime;
					blog (LOG_ERROR, "socket_thread_windows: Received FD_CLOSE, %u ms since last send (buffer: %d / %d)", diff, stream->write_buf_len, stream->write_buf_size);
				}

				if (os_event_try(stream->stop_event) != EAGAIN)
					blog (LOG_ERROR, "socket_thread_windows: Aborting due to FD_CLOSE during shutdown, %d bytes lost, error %d", stream->write_buf_len, networkEvents.iErrorCode[FD_CLOSE_BIT]);
				else
					blog (LOG_ERROR, "socket_thread_windows: Aborting due to FD_CLOSE, error %d", networkEvents.iErrorCode[FD_CLOSE_BIT]);
				FatalSocketShutdown (stream);
				return NULL;
			}

			if (networkEvents.lNetworkEvents & FD_READ)
			{
				BYTE discard[16384];
				int ret, errorCode;
				BOOL fatalError = FALSE;

				for (;;)
				{
					ret = recv(stream->rtmp.m_sb.sb_socket, (char *)discard, sizeof(discard), 0);
					if (ret == -1)
					{
						errorCode = WSAGetLastError();

						if (errorCode == WSAEWOULDBLOCK)
							break;

						fatalError = TRUE;
					}
					else if (ret == 0)
					{
						errorCode = 0;
						fatalError = TRUE;
					}

					if (fatalError)
					{
						blog(LOG_ERROR, "socket_thread_windows: Socket error, recv() returned %d, GetLastError() %d", ret, errorCode);
						FatalSocketShutdown (stream);
						return NULL;
					}
				}
			}
		}
		else if (status == WAIT_OBJECT_0 + 2)
		{
			//Ideal send backlog event
			ULONG idealSendBacklog;

			if (!idealsendbacklogquery(stream->rtmp.m_sb.sb_socket, &idealSendBacklog))
			{
				int curTCPBufSize, curTCPBufSizeSize = sizeof(curTCPBufSize);
				if (!getsockopt(stream->rtmp.m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
				{
					if (curTCPBufSize < (int)idealSendBacklog)
					{
						int bufferSize = (int)idealSendBacklog;
						setsockopt(stream->rtmp.m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof(bufferSize));
						blog (LOG_INFO, "socket_thread_windows: Increasing send buffer to ISB %d (buffer: %d / %d)", idealSendBacklog, stream->write_buf_len, stream->write_buf_size);
					}
				}
				else
					blog (LOG_ERROR, "socket_thread_windows: Got hSendBacklogEvent but getsockopt() returned %d", WSAGetLastError());
			}
			else
				blog (LOG_ERROR, "socket_thread_windows: Got hSendBacklogEvent but WSAIoctl() returned %d", WSAGetLastError());

			ResetEvent(hSendBacklogEvent);
			idealsendbacklognotify (stream->rtmp.m_sb.sb_socket, &sendBacklogOverlapped, NULL);
			continue;
		}

		if (canWrite)
		{
			bool exitLoop = false;
			do
			{
				pthread_mutex_lock(&stream->write_buf_mutex);

				if (!stream->write_buf_len)
				{
					//this is now an expected occasional condition due to use of auto-reset events, we could end up emptying the buffer
					//as it's filled in a previous loop cycle, especially if using low latency mode.
					pthread_mutex_unlock(&stream->write_buf_mutex);
					//Log(TEXT("RTMPPublisher::SocketLoop: Trying to send, but no data available?!"));
					break;
				}

				int ret;
				if (stream->low_latency_mode)
				{
					size_t sendLength = min (latencyPacketSize, stream->write_buf_len);
					ret = send(stream->rtmp.m_sb.sb_socket, (const char *)stream->write_buf, (int)sendLength, 0);
				}
				else
				{
					ret = send(stream->rtmp.m_sb.sb_socket, (const char *)stream->write_buf, (int)stream->write_buf_len, 0);
				}

				if (ret > 0)
				{
					if (stream->write_buf_len - ret)
						memmove(stream->write_buf, stream->write_buf + ret, stream->write_buf_len - ret);
					stream->write_buf_len -= ret;

					lastSendTime = os_gettime_ns() / 1000000;

					os_event_signal(stream->buffer_space_available_event);
				}
				else
				{
					int errorCode;
					BOOL fatalError = FALSE;

					if (ret == -1)
					{
						errorCode = WSAGetLastError();

						if (errorCode == WSAEWOULDBLOCK)
						{
							canWrite = false;
							pthread_mutex_unlock(&stream->write_buf_mutex);
							break;
						}

						fatalError = TRUE;
					}
					else if (ret == 0)
					{
						errorCode = 0;
						fatalError = TRUE;
					}

					if (fatalError)
					{
						//connection closed, or connection was aborted / socket closed / etc, that's a fatal error for us.
						blog (LOG_ERROR, "RTMPPublisher::SocketLoop: Socket error, send() returned %d, GetLastError() %d", ret, errorCode);
						pthread_mutex_unlock(&stream->write_buf_mutex);
						FatalSocketShutdown (stream);
						return NULL;
					}
				}

				//finish writing for now
				if (stream->write_buf_len <= 1000)
					exitLoop = true;

				pthread_mutex_unlock(&stream->write_buf_mutex);

				if (delayTime)
					os_sleep_ms (delayTime);
			} while (!exitLoop);
		}
	}

	blog (LOG_INFO, "socket_thread_windows: Normal exit");
	return NULL;
}
#endif