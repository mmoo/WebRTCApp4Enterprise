/*
 * custom_video_capturer.h
 *
 *  Created on: Apr 19, 2017
 *      Author: mekya
 */

#ifndef WEBRTC_EXAMPLES_JAVA_WRAPPER_CUSTOM_VIDEO_CAPTURER_H_
#define WEBRTC_EXAMPLES_JAVA_WRAPPER_CUSTOM_VIDEO_CAPTURER_H_

#include "webrtc/media/base/videocapturer.h"
#include "base/macros.h"
#include "webrtc/base/thread.h"
#include "webrtc/media/base/videocapturerfactory.h"
#include "webrtc/base/platform_thread.h"
#include "webrtc/base/logging.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"

namespace antmedia {


class CustomVideoCapturer :  public cricket::VideoCapturer {

public:
    CustomVideoCapturer();
    virtual ~CustomVideoCapturer();

    // cricket::VideoCapturer implementation.
    virtual cricket::CaptureState Start(const cricket::VideoFormat& capture_format) override;
    virtual void Stop() override;
    virtual bool IsRunning() override;
    virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override;
    virtual bool GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format) override;
    virtual bool IsScreencast() const override;
    void stop() ;
    void writeFrame(int8_t* data, int width, int height);

private:
    DISALLOW_COPY_AND_ASSIGN(CustomVideoCapturer);

    //static void* grabCapture(void* arg);
    //void* grabCaptureInternal();

    //std::unique_ptr<rtc::PlatformThread> _captureThread;

    //to call the SignalFrameCaptured call on the main thread
   // void SignalFrameCapturedOnStartThread(const webrtc::VideoFrame& frame);

    //cv::VideoCapture m_VCapture; //opencv capture object
    rtc::Thread*  m_startThread; //video capture thread
    //int device_id;
    webrtc::VideoFrame vframe;

};

}


#endif /* WEBRTC_EXAMPLES_JAVA_WRAPPER_CUSTOM_VIDEO_CAPTURER_H_ */
