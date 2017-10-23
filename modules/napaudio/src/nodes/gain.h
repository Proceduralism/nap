#pragma once

#include <audionode.h>

namespace nap {
    
    namespace audio {
    
        /**
         * Node to scale an audio signal
         */
        class NAPAPI Gain : public Node
        {
        public:
            Gain(NodeManager& manager) : Node(manager) { }
            
            /**
             * The input to be scaled
             */
            InputPin audioInput;
            
            /**
             * Outputs the scaled signal
             */
            OutputPin audioOutput = { this };
            
            /**
             * Sets the gain or scaling factor
             */
            void setGain(ControllerValue gain) { mGain = gain; }
            
            /**
             * @return: the gain scaling factor
             */
            ControllerValue getGain() const { return mGain; }
            
        private:
            /**
             * Calculate the output, perform the scaling
             */
            void process() override;
            
            ControllerValue mGain  = 1;
        };
        
    }
}