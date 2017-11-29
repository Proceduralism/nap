#include "levelmetercomponent.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>

// RTTI
RTTI_BEGIN_CLASS(nap::audio::LevelMeterComponent)
    RTTI_PROPERTY("Input", &nap::audio::LevelMeterComponent::mInput, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::LevelMeterComponentInstance)
    RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
    RTTI_FUNCTION("getLevel", &nap::audio::LevelMeterComponentInstance::getLevel)
RTTI_END_CLASS

namespace nap
{
    
    namespace audio
    {
        
        bool LevelMeterComponentInstance::init(utility::ErrorState& errorState)
        {
            LevelMeterComponent* resource = rtti_cast<LevelMeterComponent>(getComponent());
            mInput = resource->mInput->getInstance();
            assert(mInput);
            
            for (auto channel = 0; channel < mInput->getChannelCount(); ++channel)
            {
                meters.emplace_back(std::make_unique<LevelMeterNode>(getNodeManager()));
                meters.back()->input.connect(mInput->getOutputForChannel(channel));
            }
            
            return true;
        }
        
        
        NodeManager& LevelMeterComponentInstance::getNodeManager()
        {
            return getEntityInstance()->getCore()->getService<AudioService>(rtti::ETypeCheck::IS_DERIVED_FROM)->getNodeManager();
        }
        
        
        SampleValue LevelMeterComponentInstance::getLevel(int channel)
        {
            assert(channel < meters.size());
            return meters[channel]->getLevel();
        }
        
        
    }
    
}
