/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/platform_thread.h"
#include "file_audio_device.h"
#include "webrtc/system_wrappers/include/sleep.h"
#include "webrtc/base/thread.h"

namespace webrtc {

const int kRecordingFixedSampleRate = 48000;
const size_t kRecordingNumChannels = 2;
const int kPlayoutFixedSampleRate = 48000;
const size_t kPlayoutNumChannels = 2;
const size_t kPlayoutBufferSize =
		kPlayoutFixedSampleRate / 100 * kPlayoutNumChannels * 2;
const size_t kRecordingBufferSize =
		kRecordingFixedSampleRate / 100 * kRecordingNumChannels * 2;

VirtualFileAudioDevice::VirtualFileAudioDevice():
    										_ptrAudioBuffer(NULL),
											_recordingBuffer(NULL),
											_playoutBuffer(NULL),
											_recordingFramesLeft(0),
											_playoutFramesLeft(0),
											_recordingBufferSizeIn10MS(0),
											_recordingFramesIn10MS(0),
											_playoutFramesIn10MS(0),
											_playing(false),
											_recording(false),
											_playIsInitialized(false),
											_lastCallPlayoutMillis(0),
											_lastCallRecordMillis(0)
{
}

VirtualFileAudioDevice::~VirtualFileAudioDevice() {

}

int32_t VirtualFileAudioDevice::ActiveAudioLayer(
		AudioDeviceModule::AudioLayer& audioLayer) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

AudioDeviceGeneric::InitStatus VirtualFileAudioDevice::Init() {
	LOG(WARNING) << __FUNCTION__;
	return InitStatus::OK;
}

int32_t VirtualFileAudioDevice::Terminate() {
	LOG(WARNING) << __FUNCTION__;
	return 0;
}

bool VirtualFileAudioDevice::Initialized() const {
	LOG(WARNING) << __FUNCTION__;
	return true;
}

int16_t VirtualFileAudioDevice::PlayoutDevices() {
	LOG(WARNING) << __FUNCTION__;
	return 1;
}

int16_t VirtualFileAudioDevice::RecordingDevices() {
	LOG(WARNING) << __FUNCTION__;
	return 1;
}

int32_t VirtualFileAudioDevice::PlayoutDeviceName(uint16_t index,
		char name[kAdmMaxDeviceNameSize],
		char guid[kAdmMaxGuidSize]) {
	LOG(WARNING) << __FUNCTION__;
	const char* kName = "dummy_device";
	const char* kGuid = "dummy_device_unique_id";
	if (index < 1) {
		memset(name, 0, kAdmMaxDeviceNameSize);
		memset(guid, 0, kAdmMaxGuidSize);
		memcpy(name, kName, strlen(kName));
		memcpy(guid, kGuid, strlen(guid));
		return 0;
	}
	return -1;
}

int32_t VirtualFileAudioDevice::RecordingDeviceName(uint16_t index,
		char name[kAdmMaxDeviceNameSize],
		char guid[kAdmMaxGuidSize]) {
	LOG(WARNING) << __FUNCTION__;
	const char* kName = "dummy_device";
	const char* kGuid = "dummy_device_unique_id";
	if (index < 1) {
		memset(name, 0, kAdmMaxDeviceNameSize);
		memset(guid, 0, kAdmMaxGuidSize);
		memcpy(name, kName, strlen(kName));
		memcpy(guid, kGuid, strlen(guid));
		return 0;
	}
	return -1;
}

int32_t VirtualFileAudioDevice::SetPlayoutDevice(uint16_t index) {
	LOG(WARNING) << __FUNCTION__;
	if (index == 0) {
		_playout_index = index;
		return 0;
	}
	return -1;
}

int32_t VirtualFileAudioDevice::SetPlayoutDevice(
		AudioDeviceModule::WindowsDeviceType device) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SetRecordingDevice(uint16_t index) {
	LOG(WARNING) << __FUNCTION__ << " index: " << index;
	if (index == 0) {
		_record_index = index;
		return _record_index;
	}
	return -1;
}

int32_t VirtualFileAudioDevice::SetRecordingDevice(
		AudioDeviceModule::WindowsDeviceType device) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::PlayoutIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	if (_playout_index == 0) {
		available = true;
		return _playout_index;
	}
	available = false;
	return -1;
}

int32_t VirtualFileAudioDevice::InitPlayout() {
	LOG(WARNING) << __FUNCTION__;
	if (_ptrAudioBuffer) {
		// Update webrtc audio buffer with the selected parameters
		_ptrAudioBuffer->SetPlayoutSampleRate(kPlayoutFixedSampleRate);
		_ptrAudioBuffer->SetPlayoutChannels(kPlayoutNumChannels);
	}
	return 0;
}

bool VirtualFileAudioDevice::PlayoutIsInitialized() const {
	LOG(WARNING) << __FUNCTION__;
	return _playIsInitialized;
}

int32_t VirtualFileAudioDevice::RecordingIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	if (_record_index == 0) {
		available = true;
		return _record_index;
	}
	available = false;
	return -1;
}

int32_t VirtualFileAudioDevice::InitRecording() {
	LOG(WARNING) << __FUNCTION__;
	rtc::CritScope lock(&_critSect);

	if (_recording) {
		return -1;
	}

	_recordingFramesIn10MS = static_cast<size_t>(kRecordingFixedSampleRate / 100);

	if (_ptrAudioBuffer) {
		_ptrAudioBuffer->SetRecordingSampleRate(kRecordingFixedSampleRate);
		_ptrAudioBuffer->SetRecordingChannels(kRecordingNumChannels);
	}
	return 0;
}

bool VirtualFileAudioDevice::RecordingIsInitialized() const {
	LOG(WARNING) << __FUNCTION__;
	return _recordingFramesIn10MS != 0;
}

int32_t VirtualFileAudioDevice::StartPlayout() {
	LOG(WARNING) << __FUNCTION__;
	if (_playing) {
		return 0;
	}

	_playoutFramesIn10MS = static_cast<size_t>(kPlayoutFixedSampleRate / 100);
	_playing = true;
	_playoutFramesLeft = 0;

	if (!_playoutBuffer) {
		_playoutBuffer = new int8_t[kPlayoutBufferSize];
	}
	if (!_playoutBuffer) {
		_playing = false;
		return -1;
	}

	// PLAYOUT

	_ptrThreadPlay.reset(new rtc::PlatformThread(
			PlayThreadFunc, this, "webrtc_audio_module_play_thread"));
	_ptrThreadPlay->Start();
	_ptrThreadPlay->SetPriority(rtc::kRealtimePriority);

	LOG(LS_INFO) << "Started playout capture: ";
	return 0;
}

int32_t VirtualFileAudioDevice::StopPlayout() {
	{
		LOG(WARNING) << __FUNCTION__;
		rtc::CritScope lock(&_critSect);
		_playing = false;
	}

	// stop playout thread first
	if (_ptrThreadPlay) {
		_ptrThreadPlay->Stop();
		_ptrThreadPlay.reset();
	}

	rtc::CritScope lock(&_critSect);

	_playoutFramesLeft = 0;
	delete [] _playoutBuffer;
	_playoutBuffer = NULL;


	LOG(LS_INFO) << "Stopped playout capture ";

	_playIsInitialized = false;
	return 0;
}

bool VirtualFileAudioDevice::Playing() const {
	LOG(WARNING) << __FUNCTION__;
	return _playing;
}

int32_t VirtualFileAudioDevice::StartRecording() {
	LOG(WARNING) << __FUNCTION__;
	_recording = true;

	// Make sure we only create the buffer once.
	_recordingBufferSizeIn10MS = _recordingFramesIn10MS *
			kRecordingNumChannels *
			2;
	if (!_recordingBuffer) {
		_recordingBuffer = new int8_t[_recordingBufferSizeIn10MS];
	}

	//_ptrThreadRec.reset(new rtc::PlatformThread(
	//		RecThreadFunc, this, "webrtc_audio_module_capture_thread"));

	//_ptrThreadRec->Start();
	//_ptrThreadRec->SetPriority(rtc::kRealtimePriority);

	LOG(LS_INFO) << "Started recording ";

	return 0;
}


int32_t VirtualFileAudioDevice::StopRecording() {
	{
		LOG(WARNING) << __FUNCTION__;
		rtc::CritScope lock(&_critSect);
		_recording = false;
	}

	if (_ptrThreadRec) {
		_ptrThreadRec->Stop();
		_ptrThreadRec.reset();
	}

	rtc::CritScope lock(&_critSect);
	_recordingFramesLeft = 0;
	if (_recordingBuffer) {
		delete [] _recordingBuffer;
		_recordingBuffer = NULL;
	}


	LOG(LS_INFO) << "Stopped recording "
			<< std::endl;
	return 0;
}

bool VirtualFileAudioDevice::Recording() const {
	LOG(WARNING) << __FUNCTION__;
	return _recording;
}

int32_t VirtualFileAudioDevice::SetAGC(bool enable) {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

bool VirtualFileAudioDevice::AGC() const {
	LOG(WARNING) << __FUNCTION__;
	return false; }

int32_t VirtualFileAudioDevice::SetWaveOutVolume(uint16_t volumeLeft,
		uint16_t volumeRight) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::WaveOutVolume(uint16_t& volumeLeft,
		uint16_t& volumeRight) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::InitSpeaker() {
	LOG(WARNING) << __FUNCTION__;
	return 0; }

bool VirtualFileAudioDevice::SpeakerIsInitialized() const {
	LOG(WARNING) << __FUNCTION__;
	return false; }

int32_t VirtualFileAudioDevice::InitMicrophone() { LOG(WARNING) << __FUNCTION__;
return 0; }

bool VirtualFileAudioDevice::MicrophoneIsInitialized() const {
	LOG(WARNING) << __FUNCTION__;
	return true; }

int32_t VirtualFileAudioDevice::SpeakerVolumeIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SetSpeakerVolume(uint32_t volume) {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::SpeakerVolume(uint32_t& volume) const { LOG(WARNING) << __FUNCTION__;
return -1; }

int32_t VirtualFileAudioDevice::MaxSpeakerVolume(uint32_t& maxVolume) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::MinSpeakerVolume(uint32_t& minVolume) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SpeakerVolumeStepSize(uint16_t& stepSize) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::MicrophoneVolumeIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SetMicrophoneVolume(uint32_t volume) { LOG(WARNING) << __FUNCTION__;
return -1; }

int32_t VirtualFileAudioDevice::MicrophoneVolume(uint32_t& volume) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::MaxMicrophoneVolume(uint32_t& maxVolume) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::MinMicrophoneVolume(uint32_t& minVolume) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::MicrophoneVolumeStepSize(uint16_t& stepSize) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SpeakerMuteIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::SetSpeakerMute(bool enable) {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::SpeakerMute(bool& enabled) const {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::MicrophoneMuteIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SetMicrophoneMute(bool enable) {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::MicrophoneMute(bool& enabled) const {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::MicrophoneBoostIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

int32_t VirtualFileAudioDevice::SetMicrophoneBoost(bool enable) { LOG(WARNING) << __FUNCTION__;
return -1; }

int32_t VirtualFileAudioDevice::MicrophoneBoost(bool& enabled) const {
	LOG(WARNING) << __FUNCTION__;
	return -1; }

int32_t VirtualFileAudioDevice::StereoPlayoutIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	available = true;
	return 0;
}
int32_t VirtualFileAudioDevice::SetStereoPlayout(bool enable) {
	LOG(WARNING) << __FUNCTION__;
	return 0;
}

int32_t VirtualFileAudioDevice::StereoPlayout(bool& enabled) const {
	LOG(WARNING) << __FUNCTION__;
	enabled = true;
	return 0;
}

int32_t VirtualFileAudioDevice::StereoRecordingIsAvailable(bool& available) {
	LOG(WARNING) << __FUNCTION__;
	available = true;
	return 0;
}

int32_t VirtualFileAudioDevice::SetStereoRecording(bool enable) {
	LOG(WARNING) << __FUNCTION__;
	return 0;
}

int32_t VirtualFileAudioDevice::StereoRecording(bool& enabled) const {
	LOG(WARNING) << __FUNCTION__;
	enabled = true;
	return 0;
}

int32_t VirtualFileAudioDevice::SetPlayoutBuffer(
		const AudioDeviceModule::BufferType type,
		uint16_t sizeMS) {
	LOG(WARNING) << __FUNCTION__;
	return 0;
}

int32_t VirtualFileAudioDevice::PlayoutBuffer(AudioDeviceModule::BufferType& type,
		uint16_t& sizeMS) const {
	LOG(WARNING) << __FUNCTION__;
	type = _playBufType;
	return 0;
}

int32_t VirtualFileAudioDevice::PlayoutDelay(uint16_t& delayMS) const {
	//LOG(WARNING) << __FUNCTION__;
	//delayMS = 25;
	return 0;
}

int32_t VirtualFileAudioDevice::RecordingDelay(uint16_t& delayMS) const {
	LOG(WARNING) << __FUNCTION__;
	return 0;
}

int32_t VirtualFileAudioDevice::CPULoad(uint16_t& load) const {
	LOG(WARNING) << __FUNCTION__;
	return -1;
}

bool VirtualFileAudioDevice::PlayoutWarning() const { LOG(WARNING) << __FUNCTION__;
return false; }

bool VirtualFileAudioDevice::PlayoutError() const {
	LOG(WARNING) << __FUNCTION__;
	return false;
}

bool VirtualFileAudioDevice::RecordingWarning() const { LOG(WARNING) << __FUNCTION__;
return false; }

bool VirtualFileAudioDevice::RecordingError() const {
	LOG(WARNING) << __FUNCTION__;
	return false; }

void VirtualFileAudioDevice::ClearPlayoutWarning() {
	LOG(WARNING) << __FUNCTION__;
}

void VirtualFileAudioDevice::ClearPlayoutError() {
	LOG(WARNING) << __FUNCTION__;
}

void VirtualFileAudioDevice::ClearRecordingWarning() {
	LOG(WARNING) << __FUNCTION__;
}

void VirtualFileAudioDevice::ClearRecordingError() {
	LOG(WARNING) << __FUNCTION__;
}

void VirtualFileAudioDevice::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
	LOG(WARNING) << __FUNCTION__;
	rtc::CritScope lock(&_critSect);

	_ptrAudioBuffer = audioBuffer;

	// Inform the AudioBuffer about default settings for this implementation.
	// Set all values to zero here since the actual settings will be done by
	// InitPlayout and InitRecording later.
	_ptrAudioBuffer->SetRecordingSampleRate(0);
	_ptrAudioBuffer->SetPlayoutSampleRate(0);
	_ptrAudioBuffer->SetRecordingChannels(0);
	_ptrAudioBuffer->SetPlayoutChannels(0);
}

bool VirtualFileAudioDevice::PlayThreadFunc(void* pThis)
{
	return (static_cast<VirtualFileAudioDevice*>(pThis)->PlayThreadProcess());
}

bool VirtualFileAudioDevice::RecThreadFunc(void* pThis)
{
	return (static_cast<VirtualFileAudioDevice*>(pThis)->RecThreadProcess());
}

bool VirtualFileAudioDevice::PlayThreadProcess()
{
	//LOG(WARNING) << " PlayThreadProcess";

	if (!_playing) {
		return false;
	}

	int64_t currentTime = rtc::TimeMicros();
	_critSect.Enter();

	int64_t timeDiff = currentTime - _lastCallPlayoutMillis;
	if (_lastCallPlayoutMillis == 0 ||timeDiff >= 10000)
	{
		//	LOG(WARNING) << " request playout data";
		_critSect.Leave();
		_ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
		_critSect.Enter();
		_lastCallPlayoutMillis = currentTime;
	}
	_critSect.Leave();

	int64_t nextCallTime = _lastCallPlayoutMillis + 10000;
	int64_t waitTime = nextCallTime - rtc::TimeMicros();

	if (waitTime > 5000) {
		SleepMs(5);
	}
	else if (waitTime > 4000) {
		SleepMs(4);
	}
	else if (waitTime > 3000) {
		SleepMs(3);
	}
	else if (waitTime > 2000) {
		SleepMs(2);
	}
	else if (waitTime > 0) {
		SleepMs(1);
	}


	return true;
}

bool VirtualFileAudioDevice::WriteAudioFrame(int8_t* data, size_t sample_count) {
	if (!_recording) {
		return false;
	}

	//int64_t currentTime = rtc::TimeMillis();
	_critSect.Enter();

	//if (_lastCallRecordMillis == 0 || currentTime - _lastCallRecordMillis >= 10)
	{
		//if (_inputFile.is_open())
		{
			/*
			if (_inputFile.Read(_recordingBuffer, kRecordingBufferSize) > 0) {
				_ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
						_recordingFramesIn10MS);
			} else {
				_inputFile.Rewind();
			}
			 */
			_ptrAudioBuffer->SetRecordedBuffer(data,
					sample_count);
			//_lastCallRecordMillis = currentTime;
			_critSect.Leave();
			_ptrAudioBuffer->DeliverRecordedData();
			//			_critSect.Enter();
		}
	}

	//	_critSect.Leave();

	//int64_t deltaTimeMillis = rtc::TimeMillis() - currentTime;
	//if (deltaTimeMillis < 10) {
	//	SleepMs(10 - deltaTimeMillis);
	//}

	return true;

}

bool VirtualFileAudioDevice::RecThreadProcess()
{
	LOG(WARNING) << __FUNCTION__;
	if (!_recording) {
		return false;
	}

	int64_t currentTime = rtc::TimeMillis();
	_critSect.Enter();

	if (_lastCallRecordMillis == 0 ||
			currentTime - _lastCallRecordMillis >= 10) {
		//if (_inputFile.is_open())
		{
			/*
			if (_inputFile.Read(_recordingBuffer, kRecordingBufferSize) > 0) {
				_ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
						_recordingFramesIn10MS);
			} else {
				_inputFile.Rewind();
			}
			 */
			_ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
					_recordingFramesIn10MS);
			_lastCallRecordMillis = currentTime;
			_critSect.Leave();
			_ptrAudioBuffer->DeliverRecordedData();
			_critSect.Enter();
		}
	}

	_critSect.Leave();

	int64_t deltaTimeMillis = rtc::TimeMillis() - currentTime;
	if (deltaTimeMillis < 10) {
		SleepMs(10 - deltaTimeMillis);
	}

	return true;
}

}  // namespace webrtc
