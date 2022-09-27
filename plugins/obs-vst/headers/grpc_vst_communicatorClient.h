#pragma once

#include <obs_vst_api.grpc.pb.h>
#include <grpcpp/grpcpp.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class AEffect;

class grpc_vst_communicatorClient
{
public:
	grpc_vst_communicatorClient(std::shared_ptr<Channel> channel);

	intptr_t dispatcher(AEffect* a, int b, int c, intptr_t d, void* ptr, float f, size_t ptr_size);

	float getParameter(AEffect* a, int b);
	
	void setParameter(AEffect* a, int b, float c);
	void processReplacing(AEffect* a, float** adata, float** bdata, int frames, int arraySize);
	void sendHwndMsg(AEffect* a, int msgType);
	void updateAEffect(AEffect* a);
	void stopServer(AEffect* a);

	std::atomic<bool> m_connected{ false };

private:
	std::unique_ptr<grpc_vst_communicator::Stub> stub_;
};
