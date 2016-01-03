// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/FunctionNode.h"
#include "internal/AudioBus.h"

using namespace std;
using namespace lab;

namespace lab {
    
    FunctionNode::FunctionNode(float sampleRate, int channels) : AudioScheduledSourceNode(sampleRate), numChannels(channels)
    {
        //@ tofix - channels needs to be set on this node. quick fix is to setChannelCount at the app layer for the node
        // see Groove.h example file.
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, channels)));
        initialize();
    }
    
    FunctionNode::~FunctionNode()
    {
        uninitialize();
    }
    
    void FunctionNode::process(ContextRenderLock& r, size_t framesToProcess)
    {
        AudioBus* outputBus = output(0)->bus(r);
        
        if (!isInitialized() || !outputBus->numberOfChannels() || !_function) 
        {
            outputBus->zero();
            return;
        }
        
        size_t quantumFrameOffset;
        size_t nonSilentFramesToProcess;
        
        updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);
        
        if (!nonSilentFramesToProcess) 
        {
            outputBus->zero();
            return;
        }

        for (size_t i = 0; i < channelCount(); ++i) 
        {
            float * destP = outputBus->channel(i)->mutableData();
            
            // Start rendering at the correct offset.
            destP += quantumFrameOffset;
            int n = nonSilentFramesToProcess;
            
            _function(r, this, i, destP, n);
        }

        _now += double(framesToProcess) / sampleRate();

        outputBus->clearSilentFlag();
    }
    
    void FunctionNode::reset(ContextRenderLock  & r)
    {
        // No-op
    }
    
    bool FunctionNode::propagatesSilence(double now) const
    {
        return !isPlayingOrScheduled() || hasFinished();
    }
    
} // namespace lab

