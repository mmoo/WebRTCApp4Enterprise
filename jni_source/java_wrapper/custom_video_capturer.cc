/*
 * custom_video_capturer.cc
 *
 *  Created on: Apr 19, 2017
 *      Author: mekya
 */

#include "custom_video_capturer.h"
#include "webrtc/api/video/i420_buffer.h"
#include <iostream>

using namespace antmedia;

CustomVideoCapturer::CustomVideoCapturer():vframe(nullptr,webrtc::VideoRotation::kVideoRotation_0, 0)
{
	std::vector<cricket::VideoFormat> supported;
	cricket::VideoFormat format;
	format.width = 1280;
	format.height = 720;
	format.fourcc = cricket::FOURCC_I420;
	format.interval = cricket::VideoFormat::FpsToInterval(25);
	supported.push_back(format);

	cricket::VideoFormat format2;
	format2.width = 640;
	format2.height = 480;
	format2.fourcc = cricket::FOURCC_I420;
	format2.interval = cricket::VideoFormat::FpsToInterval(25);
	supported.push_back(format2);

	cricket::VideoFormat format3;
	format3.width = 480;
	format3.height = 360;
	format3.fourcc = cricket::FOURCC_I420;
	format3.interval = cricket::VideoFormat::FpsToInterval(25);
	supported.push_back(format3);

	cricket::VideoFormat format4;
	format4.width = 360;
	format4.height = 240;
	format4.fourcc = cricket::FOURCC_I420;
	format4.interval = cricket::VideoFormat::FpsToInterval(25);
	supported.push_back(format4);


	SetSupportedFormats(supported);
}

CustomVideoCapturer::~CustomVideoCapturer()
{
}


void CustomVideoCapturer::OnSinkWantsChanged(const rtc::VideoSinkWants& wants) {
  //RTC_DCHECK(thread_checker_.CalledOnValidThread());
  LOG(WARNING) << "-------------------------------------";
  std::cout << "---------------OnSinkWantsChanged----------------------" << std::endl;
  LOG(WARNING) << "-------------------------------------";

}

cricket::CaptureState CustomVideoCapturer::Start(const cricket::VideoFormat& capture_format)
{

	if (capture_state() == cricket::CS_RUNNING) {
		std::cout << "Start called when it's already started.";
		return capture_state();
	}

	LOG(WARNING) << "capture format: " << capture_format.width <<" height: " << capture_format.height << " frame rate: "
			<< capture_format.framerate() << " interval: " << capture_format.interval;
	m_startThread = rtc::Thread::Current();

	//pthread_create(&g_pthread, NULL, grabCapture, (void*)this);

	SetCaptureFormat(&capture_format);
	return cricket::CS_RUNNING;
}

void CustomVideoCapturer::Stop()
{
	LOG(WARNING) << "Custom Video Capturer Stop";
	std::cout << "Stop";
	if (capture_state() == cricket::CS_STOPPED) {
		std::cout << "Stop called when it's already stopped.";
		return;
	}

	m_startThread = nullptr;

	SetCaptureFormat(NULL);
	SetCaptureState(cricket::CS_STOPPED);
}

/*
void* CustomVideoCapturer::grabCapture(void* arg) {
	CustomVideoCapturer *vc = (CustomVideoCapturer*)arg;

	if(nullptr == vc){
		std::cout << "VideoCapturer pointer is null" << std::endl;
		return 0;
	}
	vc->grabCaptureInternal();

	return 0;
}
 */

void CustomVideoCapturer::stop() {

	LOG(WARNING) << "Custom Video Capturer stop";

	if (m_startThread->IsCurrent()) {
		SetCaptureState(cricket::CS_STOPPED);
		//VideoCapturer::OnFrame(frame, frame.width(), frame.height());
		LOG(INFO) << " on frame called" ;
	} else {
		//webrtc::VideoFrame frame(buffer, 0, 0, webrtc::kVideoRotation_0);
		//this->vframe = frame;
		//vc->OnFrame(frame, frame.width(), frame.height());
		m_startThread->Invoke<void>(RTC_FROM_HERE, [this] {
			SetCaptureState(cricket::CS_STOPPED);
		});
	}
}

/*
void CustomVideoCapturer::writePacket(int8_t* data, int width, int height) {
	if (m_startThread->IsCurrent()) {
			VideoCapturer::OnFrame(webrtcFrame, webrtcFrame.width(), webrtcFrame.height());
		} else {
			this->vframe = webrtcFrame;
			//vc->OnFrame(frame, frame.width(), frame.height());
			m_startThread->Invoke<void>(RTC_FROM_HERE, [this] {
				OnFrame(vframe, vframe.width(), vframe.height());
			});
		}
}

 */


void CustomVideoCapturer::writeMockFrame(int width, int height) {

	webrtc::I420Buffer* buffer = mockBuffer.get();
	if (buffer == nullptr || buffer->width() != width || buffer->height() != height) {
		mockBuffer =
				webrtc::I420Buffer::Create(width, height, width, (width+1) /2, (width+1) /2 );

		webrtc::VideoFrame webrtcFrame(mockBuffer, 0, 0, webrtc::kVideoRotation_0);
		vframe = webrtcFrame;
	}

	m_startThread->Invoke<void>(RTC_FROM_HERE, [this] {
		OnFrame(vframe, vframe.width(), vframe.height());
	});

}

void CustomVideoCapturer::writeFrame(int8_t* data, int width, int height) {

	if (capture_state() != cricket::CS_RUNNING) {
		return;
	}

	rtc::scoped_refptr<webrtc::I420Buffer> buffer =
			webrtc::I420Buffer::Create(width, height, width, (width+1) /2, (width+1) /2 );

	size_t length = webrtc::CalcBufferSize(webrtc::kI420, width, height);

	if(0 != webrtc::ConvertToI420(webrtc::kI420, (unsigned char*)data, 0, 0, width, height, length, webrtc::kVideoRotation_0, buffer.get()) ){
		LOG(WARNING) << "Failed to convert frame to i420" << std::endl;
	}

	webrtc::VideoFrame webrtcFrame(buffer, 0, 0, webrtc::kVideoRotation_0);
	vframe = webrtcFrame;
	//vc->OnFrame(frame, frame.width(), frame.height());

	//VideoCapturer::OnFrame(vframe, vframe.width(), vframe.height());

	if (m_startThread->IsCurrent()) {
		VideoCapturer::OnFrame(webrtcFrame, webrtcFrame.width(), webrtcFrame.height());
	} else {
		this->vframe = webrtcFrame;
		//vc->OnFrame(frame, frame.width(), frame.height());
		m_startThread->Invoke<void>(RTC_FROM_HERE, [this] {
			OnFrame(vframe, vframe.width(), vframe.height());
		});
	}


}

bool CustomVideoCapturer::IsRunning()
{
	return capture_state() == cricket::CS_RUNNING;
}

bool CustomVideoCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
{
	if (!fourccs)
		return false;
	fourccs->push_back(cricket::FOURCC_I420);
	return true;
}

bool CustomVideoCapturer::GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format)
{

	LOG(WARNING) << __FUNCTION__ << " desired format width :" << desired.width << " height: " << desired.height;


	if (!best_format)
		return false;


	LOG(WARNING) << "best format null ";

	// Use the desired format as the best format.
	best_format->width = desired.width;
	best_format->height = desired.height;
	best_format->fourcc = cricket::FOURCC_I420;
	best_format->interval = desired.interval;
	return true;
}

bool CustomVideoCapturer::IsScreencast() const
{
	return false;
}




