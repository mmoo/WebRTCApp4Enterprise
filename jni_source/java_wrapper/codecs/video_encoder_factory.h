

#ifndef WEBRTC_MEDIA_ENGINE_ANT_MEDIA_INTERNALENCODERFACTORY_H_
#define WEBRTC_MEDIA_ENGINE_ANT_MEDIA_INTERNALENCODERFACTORY_H_

#include <vector>

#include "webrtc/media/engine/internalencoderfactory.h"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#include "mock_h264_encoder.h"
#include "../adapt_bitrate.h"

namespace antmedia {


class VideoEncoderFactory : public cricket::WebRtcVideoEncoderFactory {

 public:
	VideoEncoderFactory() {

	   if (webrtc::H264Encoder::IsSupported())
	   {
	     cricket::VideoCodec codec(cricket::kH264CodecName);
	     // TODO(magjed): Move setting these parameters into webrtc::H264Encoder
	     // instead.
	     codec.SetParam(cricket::kH264FmtpProfileLevelId,
	    		// "42C02A");  // this is baseline from profile_level_id_unittest
	    		cricket::kH264ProfileLevelConstrainedBaseline);
	     codec.SetParam(cricket::kH264FmtpLevelAsymmetryAllowed, "1");
	     supported_codecs_.push_back(std::move(codec));
	     LOG(WARNING) << " OK --------------- H264Encoder::IsSupported()";
	   }
	   else {
		   LOG(WARNING) << " NOT --------------- H264Encoder::IsSupported()";
	   }

  }
  virtual ~VideoEncoderFactory() {

  };

  void setBitrateAdaptor(IAdaptBitrate* bitrateAdaptor) {
	  this->bitrateAdaptor = bitrateAdaptor;

  }


  // WebRtcVideoEncoderFactory implementation.
  webrtc::VideoEncoder* CreateVideoEncoder(const cricket::VideoCodec& codec) override {
	  const webrtc::VideoCodecType codec_type =
	        webrtc::PayloadNameToCodecType(codec.name)
	            .value_or(webrtc::kVideoCodecUnknown);
	    switch (codec_type) {
	      case webrtc::kVideoCodecH264:
	    	  video_encoder = antmedia::MockH264Encoder::Create(codec);
	    	  video_encoder->setBitrateAdaptor(this->bitrateAdaptor);
	    	  return video_encoder;
	      default:
	        return nullptr;
	    }
  }
  const std::vector<cricket::VideoCodec>& supported_codecs() const override {
	  return supported_codecs_;
  };

  void DestroyVideoEncoder(webrtc::VideoEncoder* encoder) override {
	  delete encoder;
  };


  antmedia::MockH264Encoder* getVideoEncoder() {
	  return video_encoder;
  }

 private:
  std::vector<cricket::VideoCodec> supported_codecs_;
  antmedia::MockH264Encoder* video_encoder;
  IAdaptBitrate* bitrateAdaptor;

};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_ENGINE_ANT_MEDIA_INTERNALENCODERFACTORY_H_
