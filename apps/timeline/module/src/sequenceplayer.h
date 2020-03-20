#pragma once

// internal includes
#include "sequence.h"

// external includes
#include <nap/resource.h>
#include <parameter.h>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	/**
	*/
	class NAPAPI SequencePlayer : public Resource
	{
		friend class SequenceEditor;

		RTTI_ENABLE(Resource)
	public:
		bool init(utility::ErrorState& errorState) override;

		bool save(const std::string& name, utility::ErrorState& errorState);

		bool load(const std::string& name, utility::ErrorState& errorState);

		double getDuration() const;
	public:
		std::string mDefaultShow;
		bool mCreateDefaultShowOnFailure = true;

		std::vector<ResourcePtr<ParameterFloat>> mParameters;
	protected:
		// Sequence Editor interface
		Sequence& getSequence();
	private:
		std::vector<std::unique_ptr<rtti::Object>> mReadObjects;

		Sequence* mSequence = nullptr;

		double	mDuration = 0.0;
		bool	mDurationNeedsUpdating = true;

		void updateDuration();
	};
}
