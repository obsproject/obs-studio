#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace libvr {

    class IPCServer
    {
          public:
        IPCServer();
        ~IPCServer();

        bool Start(const std::string &socketPath);
        void Stop();

        // Callback for incoming commands (JSON string in, JSON string out response)
        void SetCommandCallback(std::function<std::string(const std::string &)> cb);

          private:
        void ServerLoop();
        void HandleClient(int clientFd);

        std::string m_socketPath;
        std::atomic<bool> m_running;
        std::thread m_serverThread;
        int m_serverFd = -1;

        std::function<std::string(const std::string &)> m_commandCallback;
    };

}  // namespace libvr
