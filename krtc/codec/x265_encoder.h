#ifndef KRTCSDK_KRTC_CODEC_X265_ENCODER_H_
#define KRTCSDK_KRTC_CODEC_X265_ENCODER_H_

#include <memory>
#include <vector>
#include <string.h>
#include <fstream>

#include <api/video/i420_buffer.h>
#include <media/base/codec.h>
#include <common_video/h265/h265_bitstream_parser.h>
#include <modules/video_coding/codecs/h265/include/h265_globals.h>
#include <modules/video_coding/utility/quality_scaler.h>

#include <x265/x265.h>

namespace krtc {

#define	X265_ENCODE_DATA_SAVE 0

	class X265Encoder : public webrtc::VideoEncoder {
	public:
		struct LayerConfig
		{
			int simulcast_idx = 0;
			int width = -1;
			int height = -1;
			bool sending = true;
			bool key_frame_request = false;
			float max_frame_rate = 0;
			uint32_t target_bps = 0;
			uint32_t max_bps = 0;
			bool frame_dropping_on = false;
			int key_frame_interval = 0;
			int num_temporal_layers = 1;

			void SetStreamState(bool send_stream);
		};

	public:
		explicit X265Encoder(const cricket::VideoCodec& codec);
		~X265Encoder() override;

		int32_t InitEncode(const webrtc::VideoCodec* codec_settings,
			int32_t number_of_cores,
			size_t max_payload_size) override;
		int32_t Release() override;

		int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) override;

		void SetRates(const RateControlParameters& parameters) override;

		// The result of encoding - an EncodedImage and RTPFragmentationHeader - are
		// passed to the encode complete callback.
		int32_t Encode(const webrtc::VideoFrame& frame,
			const std::vector<webrtc::VideoFrameType>* frame_types) override;

		EncoderInfo GetEncoderInfo() const override;

		// Exposed for testing.
		webrtc::H265PacketizationMode PacketizationModeForTesting() const {
			return packetization_mode_;
		}

	private:
		webrtc::H265BitstreamParser h265_bitstream_parser_;
		// Reports statistics with histograms.
		void ReportInit();
		void ReportError();

		bool EncodeFrame(int index, const webrtc::VideoFrame& input_frame,
			std::vector<uint8_t>& frame_packet);

		std::vector<LayerConfig> configurations_;
		std::vector<webrtc::EncodedImage> encoded_images_;
		std::vector<rtc::scoped_refptr<webrtc::I420Buffer>> downscaled_buffers_;

		webrtc::VideoCodec codec_;
		webrtc::H265PacketizationMode packetization_mode_;
		size_t max_payload_size_;
		int32_t number_of_cores_;
		webrtc::EncodedImageCallback* encoded_image_callback_;

		bool has_reported_init_;
		bool has_reported_error_;
		int num_temporal_layers_;

		std::vector<x265_encoder*> x265_encoders_;
		std::vector<x265_picture*> x265_pictures_;
		std::vector<x265_param> x265_params_;
#if X265_ENCODE_DATA_SAVE
		std::vector<std::ofstream> outfiles_;
#endif
	};

}  // namespace webrtc

#endif  // KRTCSDK_KRTC_CODEC_X265_ENCODER_H_
