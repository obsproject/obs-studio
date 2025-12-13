#pragma once

#include <string>
#include <vector>
#include <map>
#include <any>
#include <functional>
#include <memory>

namespace NeuralStudio {
    namespace SceneGraph {

        // Forward declarations
        class IExecutableNode;
        struct ExecutionContext;
        struct ExecutionResult;

        //=============================================================================
        // Data Type System
        //=============================================================================

        enum class DataCategory {
            Primitive,  // Scalar, Vector, Color, Boolean, String
            Media,      // Texture, Audio, Video, Mesh
            Spatial,    // Transform, Camera, Light
            Effect,     // Shader, Material, Filter, LUT
            AI,         // Tensor, Model, Embedding
            Composite   // Array, Map, Variant, Stream
        };

        enum class ComputeRequirement {
            Low,      // Simple operations, can run on CPU
            Medium,   // Moderate processing, benefits from GPU
            High,     // Intensive processing, requires GPU
            VeryHigh  // Requires high-end GPU (RTX, ML accelerators)
        };

        struct DataType {
            DataCategory category;
            std::string typeName;  // "Texture2D", "AudioBuffer", "Mesh", etc.
            size_t elementSize = 0;

            DataType() = default;
            DataType(DataCategory cat, const std::string &name, size_t size = 0) :
                category(cat), typeName(name), elementSize(size)
            {}

            // Type compatibility
            bool isCompatibleWith(const DataType &other) const;
            bool canConvertTo(const DataType &other) const;

            bool operator==(const DataType &other) const
            {
                return category == other.category && typeName == other.typeName;
            }

            // Common type constructors
            static DataType Texture2D()
            {
                return DataType(DataCategory::Media, "Texture2D");
            }
            static DataType Audio()
            {
                return DataType(DataCategory::Media, "AudioBuffer");
            }
            static DataType Video()
            {
                return DataType(DataCategory::Media, "VideoFrame");
            }
            static DataType Mesh()
            {
                return DataType(DataCategory::Media, "Mesh");
            }
            static DataType Transform()
            {
                return DataType(DataCategory::Spatial, "Transform");
            }
            static DataType Scalar()
            {
                return DataType(DataCategory::Primitive, "Scalar", sizeof(float));
            }
            static DataType Vector3()
            {
                return DataType(DataCategory::Primitive, "Vector3", sizeof(float) * 3);
            }
            static DataType Color()
            {
                return DataType(DataCategory::Primitive, "Color", sizeof(float) * 4);
            }
            static DataType Boolean()
            {
                return DataType(DataCategory::Primitive, "Boolean", sizeof(bool));
            }
            static DataType String()
            {
                return DataType(DataCategory::Primitive, "String");
            }
            static DataType Any()
            {
                return DataType(DataCategory::Composite, "Any");
            }
        };

        //=============================================================================
        // Pin System
        //=============================================================================

        struct PinDescriptor {
            std::string pinId;
            std::string pinName;  // Display name
            DataType dataType;
            bool isInput;
            bool isOptional = false;
            std::any defaultValue;  // Default value if not connected

            PinDescriptor() = default;
            PinDescriptor(const std::string &id, const std::string &name, const DataType &type, bool input) :
                pinId(id), pinName(name), dataType(type), isInput(input)
            {}
        };

        //=============================================================================
        // Node Metadata
        //=============================================================================

        struct SemanticVersion {
            int major = 1;
            int minor = 0;
            int patch = 0;

            std::string toString() const
            {
                return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
            }
        };

        struct NodeMetadata {
            std::string displayName;
            std::string category;  // "Video", "Audio", "3D", "Effect", "AI", "Filter"
            std::string description;
            std::vector<std::string> tags;

            // Visual (Qt integration)
            uint32_t nodeColorRGB = 0xFF4488FF;  // ARGB
            std::string icon;                    // Resource path

            // Capabilities
            bool supportsGPU = false;
            bool supportsCPU = true;
            bool supportsAsync = false;
            bool supportsCaching = true;
            bool supportsRealtime = true;
            bool supportsOffline = true;

            // Resources
            uint64_t estimatedMemoryMB = 0;
            ComputeRequirement computeRequirement = ComputeRequirement::Low;

            // Version
            SemanticVersion version;
            std::string author;
        };

        //=============================================================================
        // Execution Context & Result
        //=============================================================================

        // Forward declarations
        namespace libvr {
            class SceneManager;
        }

        struct ExecutionContext {
            void *renderer = nullptr;                     // IRenderEngine*
            libvr::SceneManager *sceneManager = nullptr;  // SceneManager*
            void *presentationTarget = nullptr;
            double deltaTime = 0.0;
            uint64_t frameNumber = 0;

            // Shared data pool (for inter-node communication)
            std::map<std::string, std::any> sharedData;
        };

        struct ExecutionResult {
            enum class Status {
                Success,
                PartialSuccess,  // Some outputs failed, others succeeded
                Failure,
                NeedsRetry,  // Temporary failure (resource unavailable)
                UnsupportedOperation
            };

            Status status = Status::Success;
            std::string errorMessage;
            std::vector<std::string> warnings;

            // Error recovery
            std::shared_ptr<IExecutableNode> fallbackNode = nullptr;
            std::map<std::string, std::any> fallbackData;

            static ExecutionResult success()
            {
                return ExecutionResult {Status::Success};
            }

            static ExecutionResult failure(const std::string &error)
            {
                ExecutionResult result;
                result.status = Status::Failure;
                result.errorMessage = error;
                return result;
            }
        };

        //=============================================================================
        // Node Interface
        //=============================================================================

        struct NodeConfig {
            std::map<std::string, std::any> parameters;

            template<typename T> T get(const std::string &key, const T &defaultValue = T()) const
            {
                auto it = parameters.find(key);
                if (it != parameters.end()) {
                    try {
                        return std::any_cast<T>(it->second);
                    } catch (...) {
                        return defaultValue;
                    }
                }
                return defaultValue;
            }
        };

        class IExecutableNode
        {
              public:
            virtual ~IExecutableNode() = default;

            // Identity
            virtual std::string getNodeId() const = 0;
            virtual std::string getNodeType() const = 0;
            virtual const NodeMetadata &getMetadata() const = 0;

            // Pin system (type-safe connections)
            virtual const std::vector<PinDescriptor> &getInputPins() const = 0;
            virtual const std::vector<PinDescriptor> &getOutputPins() const = 0;

            // Execution
            virtual ExecutionResult process(ExecutionContext &ctx) = 0;

            // Lifecycle
            virtual bool initialize(const NodeConfig &config)
            {
                return true;
            }
            virtual void cleanup() {}

            // Capabilities (can be overridden)
            virtual bool supportsAsync() const
            {
                return getMetadata().supportsAsync;
            }
            virtual bool supportsCaching() const
            {
                return getMetadata().supportsCaching;
            }
            virtual bool supportsGPU() const
            {
                return getMetadata().supportsGPU;
            }

            // Pin data access (implemented by base class)
            virtual void setPinData(const std::string &pinId, const std::any &data) = 0;
            virtual std::any getPinData(const std::string &pinId) const = 0;
            virtual bool hasPinData(const std::string &pinId) const = 0;
        };

        //=============================================================================
        // Node Factory (Plugin System)
        //=============================================================================

        class NodeFactory
        {
              public:
            using NodeCreator = std::function<std::shared_ptr<IExecutableNode>(const std::string &nodeId)>;

            // Register a node type
            static void registerNodeType(const std::string &typeName, NodeCreator creator);

            // Create a node instance
            static std::shared_ptr<IExecutableNode> create(const std::string &typeName, const std::string &nodeId);

            // Query available types
            static std::vector<std::string> getRegisteredTypes();
            static bool isTypeRegistered(const std::string &typeName);

              private:
            static std::map<std::string, NodeCreator> &getRegistry();
        };

// Helper macro for registration
#define REGISTER_NODE_TYPE(NodeClass, TypeName) \
    namespace { \
        struct NodeClass##Registrar { \
            NodeClass##Registrar() { \
                NodeFactory::registerNodeType(TypeName, \
                    [](const std::string& id) -> std::shared_ptr<IExecutableNode> { \
                        return std::make_shared<NodeClass>(id); \
                    }); \
            } \
        }; \
        static NodeClass##Registrar s_##NodeClass##Registrar; \
    }

    }  // namespace SceneGraph
}  // namespace NeuralStudio
