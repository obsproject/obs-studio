#include "VstModule.h"

#include "..\vst_header\aeffectx.h"
#include "..\headers\StlBuffer.h"

#include "obs_vst_api.grpc.pb.h"

#include <filesystem>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class grpc_vst_communicatorImpl final : public grpc_vst_communicator::Service
{
	Status com_grpc_dispatcher(ServerContext*, const grpc_dispatcher_Request* request, grpc_dispatcher_Reply* reply) override
	{
		if (m_effect == nullptr)
			return Status::OK;

		int64_t retValue = 0;
		std::string outputBuffer;

		switch (request->param1())
		{
		case effGetEffectName:
		case effGetVendorString:
		{
			// Needs a filled and ready buffer to write to
			outputBuffer.resize(request->ptr_size());
			retValue = m_effect->dispatcher(m_effect, request->param1(), request->param2(), request->param3(), outputBuffer.data(), request->param4());
			break;
		}
		case effGetChunk:
		{
			// Needs an empty pointer
			void* buf = nullptr;
			intptr_t chunkSize = m_effect->dispatcher(m_effect, request->param1(), request->param2(), request->param3(), &buf, request->param4());

			outputBuffer.resize(chunkSize);
			memcpy(outputBuffer.data(), buf, chunkSize);

			retValue = chunkSize;
			break;
		}
		case effSetChunk:
		{
			// Accepts the incoming data
			retValue = m_effect->dispatcher(m_effect, request->param1(), request->param2(), request->param3(), (void*)request->ptr_data().data(), request->param4());
			break;
		}
		default:
		{
			retValue = m_effect->dispatcher(m_effect, request->param1(), request->param2(), request->param3(), (void*)request->ptr_value(), request->param4());
			break;
		}
		}

		reply->set_returnval(retValue);
		reply->set_ptr_data(outputBuffer);

		if (request->param1() == effClose)
		{
			m_effect = nullptr;
			m_owner->m_stopSignal = true;
			return Status::OK;
		}

		// afx data
		reply->set_magic(m_effect->magic);
		reply->set_numprograms(m_effect->numPrograms);
		reply->set_numparams(m_effect->numParams);
		reply->set_numinputs(m_effect->numInputs);
		reply->set_numoutputs(m_effect->numOutputs);
		reply->set_flags(m_effect->flags);
		reply->set_initialdelay(m_effect->initialDelay);
		reply->set_uniqueid(m_effect->uniqueID);
		reply->set_version(m_effect->version);

		return Status::OK;
	}

	Status com_grpc_processReplacing(ServerContext*, const grpc_processReplacing_Request* request, grpc_processReplacing_Reply* reply) override
	{
		if (m_effect == nullptr)
			return Status::OK;
		
		size_t read_idx_a = 0;
		size_t read_idx_b = 0;

		float** adata = new float*[request->arraysize()];
		float** bdata = new float*[request->arraysize()];

		for (size_t c = 0; c < request->arraysize(); c++) 
		{
			adata[c] = new float[request->frames()];
			bdata[c] = new float[request->frames()];

			StlBuffer::pop_buffer(request->adata(), read_idx_a, (char*)adata[c], request->frames() * sizeof(float));
			StlBuffer::pop_buffer(request->bdata(), read_idx_b, (char*)bdata[c], request->frames() * sizeof(float));
		}

		m_effect->processReplacing(m_effect, adata, bdata, request->frames());	  
		
		std::string buffer_adata;
		std::string buffer_bdata;

		for (int c = 0; c < request->arraysize(); c++) 
		{
			buffer_adata.append((char*)adata[c], request->frames() * sizeof(float));
			buffer_bdata.append((char*)bdata[c], request->frames() * sizeof(float));
			
			delete[] adata[c];
			delete[] bdata[c];
		}
		
		delete[] adata;
		delete[] bdata;

		reply->set_adata(buffer_adata);
		reply->set_bdata(buffer_bdata);

		// afx data
		reply->set_magic(m_effect->magic);
		reply->set_numprograms(m_effect->numPrograms);
		reply->set_numparams(m_effect->numParams);
		reply->set_numinputs(m_effect->numInputs);
		reply->set_numoutputs(m_effect->numOutputs);
		reply->set_flags(m_effect->flags);
		reply->set_initialdelay(m_effect->initialDelay);
		reply->set_uniqueid(m_effect->uniqueID);
		reply->set_version(m_effect->version);
		
		return Status::OK;
	}

	Status com_grpc_setParameter(ServerContext*, const grpc_setParameter_Request* request, grpc_setParameter_Reply* reply) override
	{
		if (m_effect == nullptr)
			return Status::OK;

		m_effect->setParameter(m_effect, request->param1(), request->param2());

		// afx data
		reply->set_magic(m_effect->magic);
		reply->set_numprograms(m_effect->numPrograms);
		reply->set_numparams(m_effect->numParams);
		reply->set_numinputs(m_effect->numInputs);
		reply->set_numoutputs(m_effect->numOutputs);
		reply->set_flags(m_effect->flags);
		reply->set_initialdelay(m_effect->initialDelay);
		reply->set_uniqueid(m_effect->uniqueID);
		reply->set_version(m_effect->version);
		
		return Status::OK;
	}

	Status com_grpc_getParameter(ServerContext*, const grpc_getParameter_Request* request, grpc_getParameter_Reply* reply) override
	{
		if (m_effect == nullptr)
			return Status::OK;

		float result = m_effect->getParameter(m_effect, request->param1());
		reply->set_returnval(result);

		// afx data
		reply->set_magic(m_effect->magic);
		reply->set_numprograms(m_effect->numPrograms);
		reply->set_numparams(m_effect->numParams);
		reply->set_numinputs(m_effect->numInputs);
		reply->set_numoutputs(m_effect->numOutputs);
		reply->set_flags(m_effect->flags);
		reply->set_initialdelay(m_effect->initialDelay);
		reply->set_uniqueid(m_effect->uniqueID);
		reply->set_version(m_effect->version);
		
		return Status::OK;
	}

	Status com_grpc_sendHwndMsg(ServerContext*, const grpc_sendHwndMsg_Request* request, grpc_sendHwndMsg_Reply*) override
	{
		if (m_effect == nullptr)
			return Status::OK;

		m_owner->m_hwndSendFunction(request->msgtype());
		return Status::OK;
	}

	Status com_grpc_updateAEffect(ServerContext*, const grpc_updateAEffect_Request*, grpc_updateAEffect_Reply* reply) override
	{
		if (m_effect == nullptr)
			return Status::OK;

		// afx data
		reply->set_magic(m_effect->magic);
		reply->set_numprograms(m_effect->numPrograms);
		reply->set_numparams(m_effect->numParams);
		reply->set_numinputs(m_effect->numInputs);
		reply->set_numoutputs(m_effect->numOutputs);
		reply->set_flags(m_effect->flags);
		reply->set_initialdelay(m_effect->initialDelay);
		reply->set_uniqueid(m_effect->uniqueID);
		reply->set_version(m_effect->version);
		
		return Status::OK;
	}

	Status com_grpc_stopServer(ServerContext*, const grpc_stopServer_Request*, grpc_stopServer_Reply* reply) override
	{
		m_owner->m_stopSignal = true;
		reply->set_nullreply(0);
		return Status::OK;
	}

public:
	AEffect* m_effect{ nullptr };
	VstModule* m_owner{ nullptr };
};

VstModule::VstModule(const std::wstring& modulePath, const int32_t listenPort) :
	m_modulePath(modulePath),
	m_listenPort(listenPort)
{

}

VstModule::~VstModule()
{
	if (m_dllHandle != NULL)
		::FreeLibrary(m_dllHandle);
}

bool VstModule::start()
{
	// Vst
	//

	typedef AEffect *(*vstPluginMain)(audioMasterCallback audioMaster);

	::SetDllDirectoryW(std::filesystem::path(m_modulePath).remove_filename().c_str());	
	m_dllHandle = ::LoadLibraryW(m_modulePath.c_str());
	::SetDllDirectoryW(nullptr);

	if (m_dllHandle == NULL)
		return false;

	vstPluginMain mainEntryPoint = (vstPluginMain)GetProcAddress(m_dllHandle, "VSTPluginMain");

	if (mainEntryPoint == nullptr)
		mainEntryPoint = (vstPluginMain)GetProcAddress(m_dllHandle, "VstPluginMain()");

	if (mainEntryPoint == nullptr)
		mainEntryPoint = (vstPluginMain)GetProcAddress(m_dllHandle, "main");

	if (mainEntryPoint == nullptr) 
		return false;

	// Instantiate the plug-in
	m_effect = mainEntryPoint([](AEffect *effect, int32_t opcode, int32_t /*index*/, intptr_t /*value*/, void* /*ptr*/, float /*opt*/)
	{
		// hostCallback
		if (effect && effect->user != nullptr)
		{
			intptr_t result = 0;

			switch (opcode) 
			{
			case audioMasterSizeWindow:
				return static_cast<intptr_t>(0);
			}

			return result;
		}

		switch (opcode) 
		{
		case audioMasterVersion:
			return static_cast<intptr_t>(2400);
		default:
			return static_cast<intptr_t>(0);
		}
	});

	if (m_effect == nullptr)
		return false;

	m_effect->user = reinterpret_cast<void*>(1);

	// Grpc
	//

	grpc::EnableDefaultHealthCheckService(true);
	grpc::reflection::InitProtoReflectionServerBuilderPlugin();

	m_builder = std::make_unique<ServerBuilder>();
	m_builder->AddListeningPort(std::string("localhost:") + std::to_string(m_listenPort), grpc::InsecureServerCredentials());
	
	m_service = std::make_unique<grpc_vst_communicatorImpl>();
	m_service->m_effect = m_effect;
	m_service->m_owner = this;
	m_builder->RegisterService(m_service.get());

	m_server = m_builder->BuildAndStart();
	return true;
}

void VstModule::join()
{
	if (m_server == nullptr)
		return;

	m_server->Wait();
	m_stopSignal = true;

	// Cleanup
	FreeLibrary(m_dllHandle);
	m_dllHandle = NULL;
}

void VstModule::shutdown_server()
{
	if (m_server == nullptr)
		return;

	m_server->Shutdown();
}
