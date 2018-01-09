

#include "custom_audio_device_module.h"
#include "file_audio_device.h"
#include "webrtc/base/thread.h"


namespace antmedia {

rtc::scoped_refptr<webrtc::AudioDeviceModule>
CustomAudioDeviceModule::Create(const int32_t id, AudioLayer audio_layer)
{

	LOG(WARNING) << __FUNCTION__ << rtc::Thread::Current()->name();

	// Create the generic ref counted (platform independent) implementation.
	rtc::scoped_refptr<CustomAudioDeviceModule> audioDevice =
			new rtc::RefCountedObject<CustomAudioDeviceModule>(id, audio_layer);

	// Ensure that the current platform is supported.
	if (audioDevice->CheckPlatform() == -1) {
		return nullptr;
	}

	// Create the platform-dependent implementation.
	if (audioDevice->CreateObjects() == -1) {
		return nullptr;
	}

	// Ensure that the generic audio buffer can communicate with the
	// platform-specific parts.
	if (audioDevice->AttachAudioBuffer() == -1) {
		return nullptr;
	}

	return audioDevice;
}


CustomAudioDeviceModule::CustomAudioDeviceModule(const int32_t id,
		const AudioLayer audioLayer)
:AudioDeviceModuleImpl(id, audioLayer)
{
	LOG(INFO) << __FUNCTION__;
}

void CustomAudioDeviceModule::newFrameAvailable(int sample_count) {


	VirtualFileAudioDevice* audioDevice = (VirtualFileAudioDevice*)_ptrAudioDevice;
	audioDevice->NewFrameAvailable(sample_count);

}



int32_t CustomAudioDeviceModule::CreateObjects() {
	AudioDeviceGeneric* ptrAudioDevice(NULL);

	//ptrAudioDevice =  new CustomAudioDevice(0);
	//FileAudioDeviceFactory::CreateFileAudioDevice(Id());

	ptrAudioDevice = (AudioDeviceGeneric*)new VirtualFileAudioDevice();

	//ptrAudioDevice = new AudioDeviceMac(0);
	if (ptrAudioDevice == NULL) {
		LOG(LERROR)
	        		 << "unable to create the platform specific audio device implementation";
		return -1;
	}

	_ptrAudioDevice = ptrAudioDevice;

	return 0;
}

bool CustomAudioDeviceModule::WriteAudioFrame(int8_t* data, size_t sample_count) {
	//LOG(WARNING) << __FUNCTION__ << rtc::TimeMillis();
	VirtualFileAudioDevice* audioDevice = (VirtualFileAudioDevice*)_ptrAudioDevice;
	return audioDevice->WriteAudioFrame(data, sample_count);
}

}
