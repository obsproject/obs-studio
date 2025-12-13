#pragma once
#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {
        class ScriptNode : public BaseNodeBackend
        {
              public:
            ScriptNode(const std::string &id);
            ~ScriptNode() override;
            void execute(const ExecutionContext &context) override;
            void setScriptPath(const std::string &path);
            std::string getScriptPath() const;
            void setScriptLanguage(const std::string &lang);  // "lua", "python"
            std::string getScriptLanguage() const;

              private:
            std::string m_scriptPath;
            std::string m_scriptLanguage = "lua";
            bool m_dirty = false;
            void *m_scriptEngine = nullptr;
        };
    }  // namespace SceneGraph
}  // namespace NeuralStudio
