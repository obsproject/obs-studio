#include "headers/grpc_vst_communicatorClient.h"
#include "headers/StlBuffer.h"

#include <aeffectx.h>

grpc_vst_communicatorClient::grpc_vst_communicatorClient(std::shared_ptr<Channel> channel) :
	stub_(grpc_vst_communicator::NewStub(channel))
{
	m_connected = channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(3));
}

intptr_t grpc_vst_communicatorClient::dispatcher(AEffect* a, int b, int c, intptr_t d, void* ptr, float f, size_t ptr_size)
{
	grpc_dispatcher_Request request;
	request.set_param1(b);
	request.set_param2(c);
	request.set_param3(d);
	request.set_param4(f);
	request.set_ptr_value(int64_t(ptr));
	request.set_ptr_size(int32_t(ptr_size));

	if (ptr_size > 0)
	{
		std::string str;
		str.resize(ptr_size);
		memcpy(str.data(), ptr, ptr_size);
		request.set_ptr_data(str);
	}

	grpc_dispatcher_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_dispatcher(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
	
	if (ptr != nullptr)
	{
		void** realDeal = (void**)ptr;

		if (*realDeal == nullptr)
		{
			*realDeal = malloc(reply.ptr_data().size());
			memcpy(*realDeal, reply.ptr_data().data(), reply.ptr_data().size());
		}
		else if (ptr_size != 0)
		{
			if (ptr_size > reply.ptr_data().size())
				ptr_size = reply.ptr_data().size();

			memcpy(realDeal, reply.ptr_data().data(), ptr_size);
		}
		else
		{
			// Unsafe, we don't know how big ptr is
			m_connected = false;
			return 0;
		}
	}
	
	a->magic = reply.magic(); 
	a->numPrograms = reply.numprograms(); 
	a->numParams = reply.numparams(); 
	a->numInputs = reply.numinputs(); 
	a->numOutputs = reply.numoutputs(); 
	a->flags = reply.flags();
	a->initialDelay = reply.initialdelay();
	a->uniqueID = reply.uniqueid(); 
	a->version = reply.version();

	return reply.returnval();
}

void grpc_vst_communicatorClient::setParameter(AEffect* a, int b, float c) 
{
	grpc_setParameter_Request request;
	request.set_param1(b);
	request.set_param2(c);

	grpc_setParameter_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_setParameter(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
			
	a->magic = reply.magic(); 
	a->numPrograms = reply.numprograms(); 
	a->numParams = reply.numparams(); 
	a->numInputs = reply.numinputs(); 
	a->numOutputs = reply.numoutputs(); 
	a->flags = reply.flags();
	a->initialDelay = reply.initialdelay();
	a->uniqueID = reply.uniqueid(); 
	a->version = reply.version();
}

float grpc_vst_communicatorClient::getParameter(AEffect* a, int b)
{
	grpc_getParameter_Request request;
	request.set_param1(b);

	grpc_getParameter_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_getParameter(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
			
	a->magic = reply.magic(); 
	a->numPrograms = reply.numprograms(); 
	a->numParams = reply.numparams(); 
	a->numInputs = reply.numinputs(); 
	a->numOutputs = reply.numoutputs(); 
	a->flags = reply.flags();
	a->initialDelay = reply.initialdelay();
	a->uniqueID = reply.uniqueid(); 
	a->version = reply.version();
	return reply.returnval();
}

void grpc_vst_communicatorClient::processReplacing(AEffect* a, float** adata, float** bdata, int frames, int arraySize)
{
	std::string adataBuffer;
	std::string bdataBuffer;

	for (int c = 0; c < arraySize; c++)
	{
		adataBuffer.append((char*)adata[c], frames * sizeof(float));
		bdataBuffer.append((char*)bdata[c], frames * sizeof(float));
	}

	grpc_processReplacing_Request request;
	request.set_arraysize(arraySize);
	request.set_frames(frames);
	request.set_adata(adataBuffer);
	request.set_bdata(bdataBuffer);

	grpc_processReplacing_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_processReplacing(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
	
	size_t read_idx_a = 0;
	size_t read_idx_b = 0;

	for (int c = 0; c < arraySize; c++)
	{
		StlBuffer::pop_buffer(reply.adata(), read_idx_a, (char*)adata[c], frames * sizeof(float));
		StlBuffer::pop_buffer(reply.bdata(), read_idx_b, (char*)bdata[c], frames * sizeof(float));
	}

	a->magic = reply.magic(); 
	a->numPrograms = reply.numprograms(); 
	a->numParams = reply.numparams(); 
	a->numInputs = reply.numinputs(); 
	a->numOutputs = reply.numoutputs(); 
	a->flags = reply.flags();
	a->initialDelay = reply.initialdelay();
	a->uniqueID = reply.uniqueid(); 
	a->version = reply.version();
}

void grpc_vst_communicatorClient::sendHwndMsg(AEffect* /*a*/, int msgType)
{
	grpc_sendHwndMsg_Request request;
	request.set_msgtype(msgType);

	grpc_sendHwndMsg_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_sendHwndMsg(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
}

void grpc_vst_communicatorClient::updateAEffect(AEffect* a)
{
	grpc_updateAEffect_Request request;
	request.set_nullreply(1);

	grpc_updateAEffect_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_updateAEffect(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
			
	a->magic = reply.magic(); 
	a->numPrograms = reply.numprograms(); 
	a->numParams = reply.numparams(); 
	a->numInputs = reply.numinputs(); 
	a->numOutputs = reply.numoutputs(); 
	a->flags = reply.flags();
	a->initialDelay = reply.initialdelay();
	a->uniqueID = reply.uniqueid(); 
	a->version = reply.version();
}

void grpc_vst_communicatorClient::stopServer(AEffect* /*a*/)
{
	grpc_stopServer_Request request;
	request.set_nullreply(0);

	grpc_stopServer_Reply reply;
	ClientContext context;
	Status status = stub_->com_grpc_stopServer(&context, request, &reply);

	if (!status.ok())
		m_connected = false;
}
