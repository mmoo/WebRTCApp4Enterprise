

//#ifndef CUSTOM_AUDIO_DEVICE_MODULE_JAVA_WRAPPER_H_
//#define CUSTOM_AUDIO_DEVICE_MODULE_JAVA_WRAPPER_H_

#include "webrtc/base/refcount.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/modules/audio_device/audio_device_impl.h"
#include "webrtc/modules/audio_device/dummy/file_audio_device_factory.h"

#include "webrtc/base/logging.h"

using namespace webrtc;

namespace antmedia {


class CustomAudioDeviceModule : public AudioDeviceModuleImpl {


public:

	  static rtc::scoped_refptr<webrtc::AudioDeviceModule> Create(
	      const int32_t id,
		  AudioLayer audio_layer);



	  CustomAudioDeviceModule(const int32_t id,const AudioLayer audioLayer);

	  int32_t CreateObjects();

	  bool WriteAudioFrame(int8_t* data, size_t sample_count);

	  void newFrameAvailable(int sample_count);

};


}

//#endif


