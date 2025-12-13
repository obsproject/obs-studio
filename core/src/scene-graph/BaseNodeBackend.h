#pragma once

#include "IExecutableNode.h"

namespace NeuralStudio {
    namespace SceneGraph {

        /**
 * @brief Base class for processing nodes
 * 
 * Provides common functionality for pin management and data storage.
 * Specific node types should inherit from this class.
 */
        class BaseNodeBackend : public IExecutableNode
        {
              public:
            BaseNodeBackend(const std::string &nodeId, const std::string &nodeType);
            virtual ~BaseNodeBackend() = default;

            // IExecutableNode interface
            std::string getNodeId() const override
            {
                return m_nodeId;
            }
            std::string getNodeType() const override
            {
                return m_nodeType;
            }
            const NodeMetadata &getMetadata() const override
            {
                return m_metadata;
            }

            const std::vector<PinDescriptor> &getInputPins() const override
            {
                return m_inputs;
            }
            const std::vector<PinDescriptor> &getOutputPins() const override
            {
                return m_outputs;
            }

            void setPinData(const std::string &pinId, const std::any &data) override;
            std::any getPinData(const std::string &pinId) const override;
            bool hasPinData(const std::string &pinId) const override;

              protected:
            // Helper methods for derived classes
            void setMetadata(const NodeMetadata &metadata)
            {
                m_metadata = metadata;
            }

            void addInput(const std::string &pinId, const std::string &name, const DataType &type);
            void addOutput(const std::string &pinId, const std::string &name, const DataType &type);

            // Convenient data accessors
            template<typename T> T *getInputData(const std::string &pinId)
            {
                if (hasPinData(pinId)) {
                    try {
                        return std::any_cast<T>(&m_pinData[pinId]);
                    } catch (...) {
                        return nullptr;
                    }
                }
                return nullptr;
            }

            template<typename T> T getInputData(const std::string &pinId, const T &defaultValue)
            {
                if (hasPinData(pinId)) {
                    try {
                        return std::any_cast<T>(m_pinData[pinId]);
                    } catch (...) {
                        return defaultValue;
                    }
                }
                return defaultValue;
            }

            template<typename T> void setOutputData(const std::string &pinId, const T &data)
            {
                m_pinData[pinId] = data;
            }

              private:
            std::string m_nodeId;
            std::string m_nodeType;
            NodeMetadata m_metadata;

            std::vector<PinDescriptor> m_inputs;
            std::vector<PinDescriptor> m_outputs;

            std::map<std::string, std::any> m_pinData;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
