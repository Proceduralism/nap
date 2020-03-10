#pragma once

// Local Includes
#include "cvadapter.h"

// External Includes
#include <nap/resource.h>
#include <glm/glm.hpp>
#include <atomic>

namespace nap
{
	/**
	 * Captures frames from a video streamed over the network.
	 * The captured video frame is stored on the GPU when hardware acceleration is available (OpenCL).
	 * This is not a video player, only a non-blocking interface into an OpenCV video stream.
	 *
	 * Add this device to a nap::CVCaptureDevice to capture frames from the video stream, in a background thread.
	 * Note that this object should only be added once to a nap::CVCaptureDevice!
	 */
	class NAPAPI CVWebStream : public CVAdapter
	{
		RTTI_ENABLE(CVAdapter)
	public:
		/**
		 * Initialize this object after de-serialization
		 * @param errorState contains the error message when initialization fails
		 * @return if initialization succeeded.
		 */
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * Returns capture frame width in pixels.
		 * @return capture frame width in pixels.
		 */
		int getWidth() const;

		/**
		 * Returns capture frame height in pixels.
		 * @return capture frame height in pixels.
		 */
		int getHeight() const;

		std::string		mLink;									///< Property: 'Link' link to the web stream.
		bool			mConvertRGB = true;						///< Property: 'ConvertRGB' if the frame is converted into RGB
		bool			mFlipHorizontal = false;				///< Property: 'FlipHorizontal' flips the frame on the x-axis
		bool			mFlipVertical = false;					///< Property: 'FlipVertical' flips the frame on the y-axis
		bool			mResize = false;						///< Property: 'Resize' if the frame is resized to the specified 'Size' after capture
		glm::ivec2		mSize = { 1280, 720 };					///< Property: 'Size' frame size, only used when 'Resize' is turned on.

	protected:
		
		/**
		 * @return total number of matrices associated with a frame captured from this device.
		 */
		virtual int getMatrixCount() override					{ return 1; }

		/**
		 * Called by the capture device. Opens the video file or image sequence.
		 * @param captureDevice device to open
		 * @param api api back-end to use
		 * @param error contains the error if the opening operation fails
		 */
		virtual bool onOpen(cv::VideoCapture& captureDevice, int api, nap::utility::ErrorState& error) override;

		/**
		 * This method decodes and returns the just grabbed frame.
		 * @param captureDevice the device to capture the frame from.
		 * @param outFrame contains the new decoded frame
		 * @return if decoding succeeded.
		 */
		virtual CVFrame onRetrieve(cv::VideoCapture& captureDevice, utility::ErrorState& error) override;

	private:
		CVFrame					mCaptureFrame					{ 1 };
		CVFrame					mOutputFrame					{ 1 };
	};
}
