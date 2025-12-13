#pragma once

#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {

        class CameraNode : public BaseNodeBackend
        {
              public:
            explicit CameraNode(const std::string &id);
            ~CameraNode() override = default;

            void initialize() override;
            void execute(ExecutionContext &ctx) override;

            // Camera specific properties
            void setDeviceId(const std::string &deviceId);
            std::string getDeviceId() const;

              private:
            std::string m_deviceId;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
