/*
 * audio_encoder_factory.h
 *
 *  Created on: Sep 18, 2017
 *      Author: mekya
 */

#ifndef SRC_WEBRTC_EXAMPLES_JAVA_WRAPPER_CODECS_AUDIO_ENCODER_FACTORY_H_
#define SRC_WEBRTC_EXAMPLES_JAVA_WRAPPER_CODECS_AUDIO_ENCODER_FACTORY_H_

#include "mock_opus_encoder.h"
#include "webrtc/modules/audio_coding/codecs/opus/audio_encoder_opus.h"

namespace antmedia {

struct NamedEncoderFactory {
	const char* name;
	rtc::Optional<AudioCodecInfo> (*QueryAudioEncoder)(
			const SdpAudioFormat& format);
	std::unique_ptr<AudioEncoder> (
			*MakeAudioEncoder)(int payload_type, const SdpAudioFormat& format);

	template <typename T>
	static NamedEncoderFactory ForEncoder() {
		auto constructor = [](int payload_type, const SdpAudioFormat& format) {
			auto opt_info = T::QueryAudioEncoder(format);
			if (opt_info) {
				return std::unique_ptr<AudioEncoder>(new T(payload_type, format));
			}
			return std::unique_ptr<AudioEncoder>();
		};

		return {T::GetPayloadName(), T::QueryAudioEncoder, constructor};
	}
};

NamedEncoderFactory encoder_factories[] = {
		NamedEncoderFactory::ForEncoder<MockOpusEncoder>(),
};

class AudioEncoderFactory:  public webrtc::AudioEncoderFactory {

	MockOpusEncoder* mockOpusEncoder = nullptr;

	std::vector<AudioCodecSpec> GetSupportedEncoders() override {
		static const SdpAudioFormat desired_encoders[] = {
				{"opus", 48000, 2, {{"minptime", "10"}, {"useinbandfec", "1"}}},
		};
		// Initialize thread-safely, once, on first use.
		static const std::vector<AudioCodecSpec> specs = [] {
				std::vector<AudioCodecSpec> specs;
				for (const auto& format : desired_encoders) {
					for (const auto& ef : encoder_factories) {
						if (STR_CASE_CMP(format.name.c_str(), ef.name) == 0) {
							auto opt_info = ef.QueryAudioEncoder(format);
							if (opt_info) {
								specs.push_back({format, *opt_info});
							}
						}
					}
				}
				return specs;
		}();
		return specs;
	}


	rtc::Optional<AudioCodecInfo> QueryAudioEncoder(
			const SdpAudioFormat& format) override {

		for (const auto& ef : encoder_factories) {
			if (STR_CASE_CMP(format.name.c_str(), ef.name) == 0) {
				return ef.QueryAudioEncoder(format);
			}
		}
		return rtc::Optional<AudioCodecInfo>();
	}

	std::unique_ptr<AudioEncoder> MakeAudioEncoder(
			int payload_type,
			const SdpAudioFormat& format) override {

		for (const auto& ef : encoder_factories) {
			if (STR_CASE_CMP(format.name.c_str(), ef.name) == 0) {
				std::unique_ptr<AudioEncoder> audioEncoder(ef.MakeAudioEncoder(payload_type, format));
				mockOpusEncoder = (MockOpusEncoder*)audioEncoder.get();
				return audioEncoder;
			}
		}
		return nullptr;
	}

	public:
	MockOpusEncoder* getAudioEncoder() {
		return mockOpusEncoder;
	}

};

}

#endif /* SRC_WEBRTC_EXAMPLES_JAVA_WRAPPER_CODECS_AUDIO_ENCODER_FACTORY_H_ */
