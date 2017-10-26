
#ifndef WEBRTC_SDK_IAdaptBitrate_JNI_JNI_HELPERS_H_
#define WEBRTC_SDK_IAdaptBitrate_JNI_JNI_HELPERS_H_

namespace antmedia {
class IAdaptBitrate {
public:

	virtual void adaptBitrate(int bitrate) = 0;

	virtual ~IAdaptBitrate() {

	}


};

}

#endif
