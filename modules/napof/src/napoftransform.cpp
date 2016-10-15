#include <napoftransform.h>
#include <nap/entity.h>
#include <nap/logger.h>

namespace nap
{
	/**
	@brief Constructor, initializes global matrix to be identiy
	**/
	OFTransform::OFTransform()
	{
		mGlobalTransform.makeIdentityMatrix();
		mTranslate.setClamped(false);
		mRotate.setClamped(false);
		mScale.setClamped(false);
		mPivot.setClamped(false);
		mUniformScale.setClamped(false);
	}


	/**
	@brief Composes a matrix out of the individual transform attributes
	**/
	ofMatrix4x4 OFTransform::getLocalTransform() const
	{
		ofMatrix4x4 matrix;
		matrix.makeIdentityMatrix();
		matrix.glTranslate(mTranslate.getValue());
		matrix.glScale(mScale.getValue() * mUniformScale.getValue());
		matrix.glRotate(mRotate.getValue().x, 1.0f, 0.0f, 0.0f);
		matrix.glRotate(mRotate.getValue().y, 0.0f, 1.0f, 0.0f);
		matrix.glRotate(mRotate.getValue().z, 0.0f, 0.0f, 1.0f);
		return matrix;
	}


	// Iterate over all the children and find child transforms
	void OFTransform::fetchChildTransforms()
	{
		// Clear all child transforms
		mChildTransforms.clear();

		// Get ownder
		Entity* parent = getParent();
		if (parent == nullptr)
		{
			nap::Logger::warn("transform has no parent: %s", this->getName().c_str());
			return;
		}

		// Iterate over all entities and add to child transform
		for (auto& child : parent->getEntities())
		{
			OFTransform* child_xform = child->getComponent<OFTransform>();
			if (child_xform == nullptr)
			{
				Logger::debug("child has no transform: %s", child->getName().c_str());
				continue;
			}
			mChildTransforms.emplace_back(child_xform);

			// Now fetch all other child transforms
			child_xform->fetchChildTransforms();
		}
	}


	// Updates it's own global matrix and forwards calls to it's children
	void OFTransform::update(const ofMatrix4x4& inParentMatrix)
	{
		mGlobalTransform = getLocalTransform() * inParentMatrix;
		for (auto& child : mChildTransforms)
		{
			child->update(mGlobalTransform);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	OFRotateComponent::OFRotateComponent()
	{
		mPreviousTime = ofGetElapsedTimef();
		mEnableUpdates.valueChangedSignal.connect(mUpdateCalled);
		mReset.signal.connect(mResetCalled);
		mSpeed.setClamped(false);
	}


	/**
	@brief Update

	Updates local time axis based on the time passed
	**/
	void OFRotateComponent::onUpdate()
	{
		// Get time difference and store
		float current_time = ofGetElapsedTimef();
		float diff_time = current_time - mPreviousTime;
		mPreviousTime = current_time;

		// Store time
		mTimeX += (diff_time * mSpeed.getValue() * mX.getValue());
		mTimeY += (diff_time * mSpeed.getValue() * mY.getValue());
		mTimeZ += (diff_time * mSpeed.getValue() * mZ.getValue());

		// Get component
		OFTransform* xform = mDependency.get();
		if (xform == nullptr)
		{
			Logger::warn("Rotation component can't find transform component");
			return;
		}

		// Set rotation
		xform->mRotate.setValue(ofVec3f(mTimeX, mTimeY, mTimeZ));
	}


	/**
	@brief Resets the entire operator and disables updates
	**/
	void OFRotateComponent::onReset(const SignalAttribute& signal)
	{		
		mTimeX = 0.0f;
		mTimeY = 0.0f;
		mTimeZ = 0.0f;

		nap::OFTransform* xform = mDependency.get();
		if (xform == nullptr)
		{
			Logger::warn("Rotation component can't find transform component");
			return;
		}
		xform->mRotate.setValue(ofVec3f(0.0f, 0.0f, 0.0f));
	}


	/**
	@brief Stores the last time stamp so rotation continues where it left off
	**/
	void OFRotateComponent::onUpdateChanged(const bool& value)
	{
		mPreviousTime = ofGetElapsedTimef();
	}

	//////////////////////////////////////////////////////////////////////////
	// Scale Component

	/**
	@brief ScaleComponet constructor
	**/
	OFScaleComponent::OFScaleComponent()
	{
		mPreviousTime = ofGetElapsedTimef();

		// Connect
		mReset.signal.connect(mResetCalled);
		mEnableUpdates.valueChangedSignal.connect(mUpdateCalled);
	}


	/**
	@brief Scales all axis of the transform component
	**/
	void OFScaleComponent::onUpdate()
	{

		// Get time difference and store
		float current_time = ofGetElapsedTimef();
		float diff_time = current_time - mPreviousTime;
		mPreviousTime = current_time;

		// Store time
		mTime += (diff_time * mSpeed.getValue());

		// Get transform
		OFTransform* xform = mDependency.get();
		if (xform == nullptr)
		{
			nap::Logger::warn(*this, "unable to find transform component");
			return;
		}

		// Get reference scale
		if (!mRefSet)
		{
			mRefSet = true;
			mScaleRef = xform->mScale.getValue();
		}

		// Get normalized sine wave
		float v = mInstant.getValue() ? gGetSquareWave(mTime, 1.0f) : gGetSineWave(mTime, 1.0f);
		
		// Get final additive value
		float range = mInfluence.getValue() * mScale.getValue();
		float dv = 0.0f;

		// Calculate new scale
		switch (mBlendMode.getValue())
		{
		case 0:								//< Additive
			dv = range * v;
			break;
		case 1:								//< Centered
			dv = ofLerp(0.0f - (range / 2.0f), range / 2.0f, v);
			break;
		case 2:								//< Subtractive
			dv = 0.0f - (range * v);
			break;
		default:
			break;
		}

		// Calculate new scale
		ofVec3f new_scale;
		new_scale.x = mScaleRef.x + dv;
		new_scale.y = mScaleRef.y + dv;
		new_scale.z = mScaleRef.z + dv;

		// Set scale
		xform->mScale.setValue(new_scale);
	}


	/**
	@brief Resets the time
	**/
	void OFScaleComponent::onReset(const SignalAttribute& signal)
	{
		// Reset time
		mTime = 0.0f;

		// Get xform
		OFTransform* xform = mDependency.get();
		if (xform == nullptr)
		{
			nap::Logger::warn(*this, "Unable to find transform component");
			return;
		}

		// Reset reference scale
		xform->mScale.setValue(mScaleRef);
	}


	/**
	@brief Updates the time
	**/
	void OFScaleComponent::onUpdateChanged(const bool& value)
	{
		mPreviousTime = ofGetElapsedTimef();
	}

}

RTTI_DEFINE(nap::OFTransform)
RTTI_DEFINE(nap::OFRotateComponent)
RTTI_DEFINE(nap::OFScaleComponent)
