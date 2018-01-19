

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_ANT_MEDIA_MOCK_H264_ENCODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_ANT_MEDIA_MOCK_H264_ENCODER_H_


extern "C" {
//#include "third_party/ffmpeg/libavcodec/avcodec.h"
//#include "third_party/ffmpeg/libavformat/avformat.h"
//#include "third_party/ffmpeg/libavutil/imgutils.h"
#include "third_party/ffmpeg/libavutil/intreadwrite.h"
#include "third_party/ffmpeg/libavformat/avc.h"
}  // extern "C"

#include "webrtc/modules/video_coding/codecs/h264/h264_encoder_impl.h"
#include "webrtc/base/logging.h"
#include "webrtc/system_wrappers/include/clock.h"
#include "webrtc/common_types.h"
#include "webrtc/base/thread.h"
#include <iostream>
#include "../adapt_bitrate.h"


using namespace webrtc;

namespace antmedia {

class EncodedPacket {

public:
	EncodedPacket(uint8_t* p_extradata, int p_extradata_size, uint8_t* p_packet_data, int p_packet_data_size,
			int p_width, int p_height, bool p_isKeyFrame, long timestamp) {
		this->extradata = p_extradata;
		this->extradata_size = p_extradata_size;
		this->packet_data = p_packet_data;
		this->packet_data_size = p_packet_data_size;
		this->width = p_width;
		this->height = p_height;
		this->isKeyFrame = p_isKeyFrame;
		this->timestamp = timestamp;
	}

	~EncodedPacket() {
		if (extradata != nullptr) {
			delete[] extradata;
		}

		if (packet_data != nullptr) {
			delete[] packet_data;
		}
	}

	uint8_t* extradata;
	int extradata_size;
	uint8_t* packet_data;
	int packet_data_size;
	int width;
	int height;
	bool isKeyFrame;
	long timestamp;



};


class MockH264Encoder : public webrtc::H264EncoderImpl {


private:
	rtc::CriticalSection _critSect;
public:

	//bool initialized = false;
	webrtc::Clock* clock_;
	int64_t delta_ntp_internal_ms_;
	const int kMsToRtpTimestamp = 90;
	bool configSent = false;
	//RTPFragmentationHeader frag_header;

	std::queue<EncodedPacket*> encodedPacketQueue;

	MockH264Encoder(const cricket::VideoCodec& codec) : webrtc::H264EncoderImpl(codec) {

		clock_ = webrtc::Clock::GetRealTimeClock();

		delta_ntp_internal_ms_ = clock_->CurrentNtpInMilliseconds() -
				clock_->TimeInMilliseconds();

	}
	static MockH264Encoder* Create(const cricket::VideoCodec& codec) {
		LOG(WARNING) << "Creating MockH264Encoder.";

		return new MockH264Encoder(codec);
	}

	void setBitrateAdaptor(IAdaptBitrate* bitrateAdaptor) {
		this->bitrateAdaptor = bitrateAdaptor;

	}


	int32_t InitEncode(const webrtc::VideoCodec* codec_settings,
			int32_t number_of_cores,
			size_t max_payload_size) override
					{

		LOG(WARNING) <<" --- Encoder Init --- width:" << codec_settings->width <<
				" start bitrate: " << codec_settings->startBitrate <<
				" target bitrate: " << codec_settings->targetBitrate <<
				" max bitrate: "<< codec_settings->maxBitrate <<
				" max frame rate: " << codec_settings->maxFramerate << std::endl;

		//int initEncodeResult = H264EncoderImpl::InitEncode(codec_settings,
		//		number_of_cores,
		//		max_payload_size);

		/*
		if (!initialized) {
			avcodec_register_all();
			av_register_all();
			avformat_network_init();
			initialized = true;
		}
		 */


		LOG(WARNING) <<" --- Encoder Init Leaving ---";
		return WEBRTC_VIDEO_CODEC_OK;
					}



	int32_t Release() override {

		//int ret = H264EncoderImpl::Release();
		LOG(WARNING) <<" --- Encoder Release ---";
		return WEBRTC_VIDEO_CODEC_OK;
	}

	int32_t addEncodedPacketData(uint8_t* extradata, int extradata_size, uint8_t* packet_data, int packet_data_size, int width, int height, bool isKeyFrame, long timestamp) {

		uint8_t* p_extradata = nullptr;
		if (extradata_size > 0) {
			p_extradata = new uint8_t[extradata_size];
			memcpy(p_extradata, extradata, extradata_size);
		}

		uint8_t* p_packet_data = new uint8_t[packet_data_size];
		memcpy(p_packet_data, packet_data, packet_data_size);

		EncodedPacket* packet = new EncodedPacket(p_extradata, extradata_size, p_packet_data, packet_data_size, width, height, isKeyFrame, timestamp);

		_critSect.Enter();
		encodedPacketQueue.push(packet);
		_critSect.Leave();

		return 0;
	}



	/**
	 *extradata can be in
	 */
	int32_t writeConfPacket(uint8_t* extradata, int extradata_size, uint8_t* packet_data, int packet_size, int width,
					int height, bool isKeyFrame, long timestamp) {

		uint16_t configFragCount = 0;
		uint16_t sps_size, pps_size;
		if (extradata_size > 0) {
			if (extradata[0] == 1) {
				//in avc format
				sps_size = AV_RB16(&extradata[6]);
				pps_size = AV_RB16(&extradata[9 + sps_size]);

				if (ff_avc_write_annexb_extradata(extradata, &extradata,
						&extradata_size) != 0) {
					LOG(WARNING) << "returning error extradata size ";
					return WEBRTC_VIDEO_CODEC_ERROR;
				}
				configFragCount = 2;
			}
			else {
				//in annexb format
				uint8_t* newExtraData = NULL;
				int ret = ff_avc_parse_nal_units_buf(extradata, &newExtraData, &extradata_size);
				if (ret < 0) {
					LOG(WARNING) << "returning error ff_avc_parse_nal_units_buf ";
					std::cerr << "--- returning error ff_avc_parse_nal_units_buf " << std::endl;
					return WEBRTC_VIDEO_CODEC_ERROR;
				}
				sps_size = AV_RB32(&newExtraData[0]);
				pps_size = AV_RB32(&newExtraData[4 + sps_size]);

				av_free(newExtraData);


			}
		}

		int frag_count = 0;
		int offset = 0;
		int length = 0;
		int frag_offset[128]; // = new int[128];
		int frag_length[128];

		int nal_offset = 0;
		uint8_t* reformatted_data = NULL;
		if (packet_data[0] == 0 && packet_data[1] == 0 &&
				(packet_data[2] == 1 || (packet_data[2] == 0 && packet_data[3] == 1) ))
		{
			//annexb format
			int ret = ff_avc_parse_nal_units_buf(packet_data, &reformatted_data, &packet_size);
			if (ret < 0) {
				std::cerr << "--- returning error ff_avc_parse_nal_units_buf for packet" << std::endl;
				return WEBRTC_VIDEO_CODEC_ERROR;
			}
			packet_data = reformatted_data;
		}

		uint8_t data[packet_size];
		{
			do {
				size_t nal_size = AV_RB32(&packet_data[offset]);
				int nal_type = packet_data[offset+4] & 0x1F;
				//std::cerr << "\nnal type: " << nal_type << " pk data " << (int) packet_data[offset+4] << " \n";
				if (nal_type == 7 || nal_type == 8 || nal_type == 5 || nal_type == 1)
				{
					AV_WB32(&data[nal_offset], 0x00000001);

					frag_offset[frag_count] = nal_offset + 4;
					frag_length[frag_count] = nal_size;

					memcpy(data + nal_offset + 4, packet_data + offset + 4, nal_size);

					//std::cerr << "nal size: " << nal_size << " pkt size: " << packet_size;
					//std::cerr << "pkt data 0 :" <<  (int)pkt.data[offset + 4];
					//std::cerr << "pkt data 1 :" <<  (int)pkt.data[offset + 5];
					nal_offset += 4 + nal_size;
					frag_count++;

				}
				offset += nal_size + 4;

			} while (offset < packet_size);

		}

		if (reformatted_data != NULL) {
			av_free(reformatted_data);
		}


		webrtc::RTPFragmentationHeader frag_header;

		frag_header.VerifyAndAllocateFragmentationHeader(frag_count + configFragCount);

		if (configFragCount > 0) {
			frag_header.fragmentationOffset[0] = 4;
			frag_header.fragmentationLength[0] = sps_size;

			frag_header.fragmentationOffset[1] = 8 + sps_size;
			frag_header.fragmentationLength[1] = pps_size;
		}

		for (int i = 0; i < frag_count; i++) {
			frag_header.fragmentationOffset[configFragCount + i] = extradata_size + frag_offset[i];
			frag_header.fragmentationLength[configFragCount + i] = frag_length[i];
		}

		if (encoded_image_._size < (nal_offset + extradata_size)) {
			std::cerr << "\n ------ resetting buffer -----------" << std::endl;
			encoded_image_._size = packet_size + extradata_size;
			encoded_image_._buffer = new uint8_t[packet_size + extradata_size];
			encoded_image_buffer_.reset(encoded_image_._buffer);
		}

		if (extradata_size > 0) {
			memcpy(encoded_image_._buffer, extradata, extradata_size);
		}

		memcpy(encoded_image_._buffer + extradata_size, data, nal_offset);

		encoded_image_._encodedWidth = width;
		encoded_image_._encodedHeight = height;
		//encoded_image_._timeStamp = 0; //input_frame.timestamp();


		//timestamp is in milliseconds
		//std::cerr << " video timestamp " << timestamp;
		encoded_image_.ntp_time_ms_ = timestamp; // clock_->TimeInMilliseconds() + delta_ntp_internal_ms_; // input_frame.ntp_time_ms();
		encoded_image_.capture_time_ms_ = encoded_image_.ntp_time_ms_;
		encoded_image_._timeStamp = kMsToRtpTimestamp * static_cast<uint32_t>(encoded_image_.ntp_time_ms_); //timestamp

		//encoded_image_.capture_time_ms_ = 0; //input_frame.render_time_ms();
		encoded_image_.rotation_ = webrtc::kVideoRotation_0;
		encoded_image_.content_type_ = (mode_ == webrtc::kScreensharing)
											    																																																												  ? webrtc::VideoContentType::SCREENSHARE
											    																																																														  : webrtc::VideoContentType::UNSPECIFIED;
		encoded_image_._frameType =  isKeyFrame
				? webrtc::kVideoFrameKey
						: webrtc::kVideoFrameDelta;
		// ConvertToVideoFrameType(info.eFrameType);

		encoded_image_._length = nal_offset + extradata_size;
		//std::cerr << "Encoded image length: " << encoded_image_._length;

		if (encoded_image_._length > 0) {
			// Parse QP.
			h264_bitstream_parser_.ParseBitstream(encoded_image_._buffer,
					encoded_image_._length);
			h264_bitstream_parser_.GetLastSliceQp(&encoded_image_.qp_);

			// Deliver encoded image.
			webrtc::CodecSpecificInfo codec_specific;
			codec_specific.codecType = webrtc::kVideoCodecH264;
			codec_specific.codecSpecific.H264.packetization_mode = packetization_mode_;
			//std::cerr  << "---OnEncodedImage  ---" << std::endl;

			encoded_image_callback_->OnEncodedImage(encoded_image_, &codec_specific,
					&frag_header);
			//std::cerr  << "--- write video packet 8 --- " << std::endl;
		}
		return WEBRTC_VIDEO_CODEC_OK;

	}



	int32_t SetRateAllocation(
			const BitrateAllocation& bitrate_allocation,
			uint32_t framerate) override {
		if (bitrate_allocation.get_sum_bps() <= 0 || framerate <= 0)
			return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;

		target_bps_ = bitrate_allocation.get_sum_bps();
		max_frame_rate_ = static_cast<float>(framerate);

		//SBitrateInfo target_bitrate;
		//memset(&target_bitrate, 0, sizeof(SBitrateInfo));
		//target_bitrate.iLayer = SPATIAL_LAYER_ALL,
		//target_bitrate.iBitrate = target_bps_;
		//openh264_encoder_->SetOption(ENCODER_OPTION_BITRATE,
		//                             &target_bitrate);
		//openh264_encoder_->SetOption(ENCODER_OPTION_FRAME_RATE, &max_frame_rate_);
		std::cerr  << "--- SetRateAllocation --- target bps " << target_bps_ << " max frame rate: " << max_frame_rate_ <<std::endl;

		if (this->bitrateAdaptor != nullptr) {
			this->bitrateAdaptor->adaptBitrate(target_bps_);
		}

		return WEBRTC_VIDEO_CODEC_OK;
	}

	int32_t writePacket(uint8_t* packet_data, int packet_size, int width, int height, bool isKeyFrame, long timestamp) {

		return writeConfPacket(nullptr, 0, packet_data, packet_size, width, height, isKeyFrame, timestamp);
	}


	int32_t Encode(const webrtc::VideoFrame& frame,
			const webrtc::CodecSpecificInfo* codec_specific_info,
			const std::vector<webrtc::FrameType>* frame_types) override {

		if (!encoded_image_callback_) {
			LOG(LS_WARNING) << "InitEncode() has been called, but a callback function "
					<< "has not been set with RegisterEncodeCompleteCallback()";
			ReportError();
			return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
		}

		_critSect.Enter();
		if (encodedPacketQueue.empty()) {
			std::cerr << "--- Encoded Video Packet Queue is Empty -- " << std::endl;
			_critSect.Leave();
			return WEBRTC_VIDEO_CODEC_ERROR;
		}

		int queueSize = encodedPacketQueue.size();
		if (queueSize > 5) {
				std::cerr << " -- Number of video packet in the queue  " << encodedPacketQueue.size() << std::endl;
				for (int i = 2; i < queueSize; i++) {
					EncodedPacket* packet = encodedPacketQueue.front();
					encodedPacketQueue.pop();
					delete packet;
				}
				std::cerr << " -- dropping packets and new queue size  " << encodedPacketQueue.size() << std::endl;
			}

		EncodedPacket* packet = encodedPacketQueue.front();
		encodedPacketQueue.pop();
		_critSect.Leave();

		writeConfPacket(packet->extradata, packet->extradata_size, packet->packet_data, packet->packet_data_size, packet->width, packet->height, packet->isKeyFrame, packet->timestamp);


		delete packet;

		return WEBRTC_VIDEO_CODEC_OK;

	}

private:
	IAdaptBitrate* bitrateAdaptor = nullptr;

};

}

#endif
