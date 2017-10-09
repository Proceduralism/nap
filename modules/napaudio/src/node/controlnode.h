#pragma once

// RTTI includes
#include <rtti/rtti.h>

// Audio includes
#include <utility/linearramper.h>
#include <utility/exponentialramper.h>
#include <utility/translator.h>
#include <core/audionode.h>
#include <core/audionodemanager.h>

namespace nap {
    
    namespace audio {
        
        /**
         * Used to generate a control signal by ramping between different values.
         * Ramps can be either linear or exponential.
         * Optionally a lookup table can be used to shape the output signal.
         */
        class NAPAPI ControlNode : public Node {
            RTTI_ENABLE(Node)
            
        public:
            enum class RampMode { LINEAR, EXPONENTIAL };
            
        public:
            ControlNode(NodeManager& manager);
            
            /**
             * The output signal pin
             */
            OutputPin output = { this };
            
            /**
             * Set the output value immediately.
             */
            void setValue(ControllerValue value);
            
            /**
             * Return the output value, optionally shaped by the lookup translator.
             */
            ControllerValue getValue() const;
            
            /**
             * Return the output value bypassing the loopkup translator.
             */
            ControllerValue getRawValue() const { return mValue; }

            /**
             * Start ramping to @destination over a period of @time, using mode to indicate the type of ramp.
             */
            void ramp(ControllerValue destination, TimeValue time, RampMode mode = RampMode::LINEAR);
            
            /**
             * @return: wether the object is currently ramping to a new value.
             */
            bool isRamping() const;
            
            /**
             * Stops the current ramp (if any) and stays on the current value.
             */
            void stop();
            
            /**
             * Assign a translator to this node to shape the output value.
             */
            void setTranslator(Translator<ControllerValue>& translator) { mTranslator = &translator; }
            
            /**
             * @return: wether this node uses a translator lookup table to shape it's output values.
             */
            bool hasTranslator() const { return mTranslator != nullptr; }
            
            /**
             * Signal that is emitted when the destination of a ramp has been reached.
             */
            nap::Signal<ControlNode&> rampFinishedSignal;
            
        private:
            void process() override;
            
            nap::Slot<ControllerValue> mDestinationReachedSlot = { this, &ControlNode::destinationReached };
            void destinationReached(ControllerValue value) { rampFinishedSignal(*this); }
            
            ControllerValue mValue = 0;
            LinearRamper<ControllerValue> mLinearRamper = { mValue };
            ExponentialRamper<ControllerValue> mExponentialRamper = { mValue };
            Translator<ControllerValue>* mTranslator = nullptr;
        };
        
    }
    
}
