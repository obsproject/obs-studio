#pragma once
#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {
        // Generic LLM node supporting multiple backends (Gemini, GPT, Claude, etc.)
        class LLMNode : public BaseNodeBackend
        {
              public:
            LLMNode(const std::string &id);
            ~LLMNode() override;
            void execute(const ExecutionContext &context) override;
            void setPrompt(const std::string &prompt);
            void setModel(const std::string &model);
            std::string getPrompt() const;
            std::string getModel() const;

              private:
            std::string m_prompt;
            std::string m_model;
            bool m_dirty = false;
        };
    }  // namespace SceneGraph
}  // namespace NeuralStudio
