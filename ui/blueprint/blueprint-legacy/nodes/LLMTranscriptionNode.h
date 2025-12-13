#pragma once

#include "../NodeItem.h"

class LLMTranscriptionNode : public NodeItem
{
      public:
    LLMTranscriptionNode() : NodeItem("LLM Transcription")
    {
        addInputPort("Audio In", PortDataType::Audio);
        addOutputPort("Text Out", PortDataType::String);  // New 'String' type needed? Using Script/Object for now

        // Properties: Model (Whisper/Gemini), Language
    }
};
