#pragma once

#include "VstWindow.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using grpc::Status;

class AEffect;
class grpc_vst_communicatorImpl;

class VstModule
{
public:
	VstModule(const std::wstring& modulePath, const int32_t listenPort);
	~VstModule();

public:
	bool start();
	void join();
	void shutdown_server();

public:
	AEffect* m_effect{ nullptr };
	std::atomic<bool> m_stopSignal{false};
	std::function<void(int msgType)> m_hwndSendFunction;
	
private:
	int32_t m_listenPort{ 0 };
	HMODULE m_dllHandle{ NULL };

	std::wstring m_modulePath;
	std::unique_ptr<Server> m_server;	
	std::unique_ptr<ServerBuilder> m_builder;
	std::unique_ptr<grpc_vst_communicatorImpl> m_service;
};
