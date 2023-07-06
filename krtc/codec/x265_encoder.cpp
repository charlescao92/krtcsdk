#include "x265_encoder.h"
#include "common_encoder.h"

#include <limits>
#include <string>

#include "absl/strings/match.h"
#include "common_video/h265/h265_common.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"
#include "modules/video_coding/utility/simulcast_utility.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/metrics.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/scale.h"
#include "third_party/libyuv/include/libyuv/video_common.h"
#include <modules/video_coding/include/video_error_codes.h>
#include <modules/video_coding/include/video_codec_interface.h>

namespace krtc {

	X265Encoder::X265Encoder(const cricket::VideoCodec& codec)
		: packetization_mode_(webrtc::H265PacketizationMode::SingleNalUnit),
		max_payload_size_(0),
		number_of_cores_(0),
		encoded_image_callback_(nullptr),
		has_reported_init_(false),
		has_reported_error_(false)
	{
		RTC_CHECK(absl::EqualsIgnoreCase(codec.name, cricket::kH265CodecName));

		// packetization-mode=1
		packetization_mode_ = webrtc::H265PacketizationMode::NonInterleaved;
	
		encoded_images_.reserve(webrtc::kMaxSimulcastStreams);
		configurations_.reserve(webrtc::kMaxSimulcastStreams);
		x265_encoders_.reserve(webrtc::kMaxSimulcastStreams);
		downscaled_buffers_.reserve(webrtc::kMaxSimulcastStreams - 1);

	}

	X265Encoder::~X265Encoder()
	{
		Release();
	}

	int32_t X265Encoder::InitEncode(const webrtc::VideoCodec* inst,
		int32_t number_of_cores,
		size_t max_payload_size)
	{
		ReportInit();
		if (!inst || inst->codecType != webrtc::kVideoCodecH265) {
			ReportError();
			return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
		}
		if (inst->maxFramerate == 0) {
			ReportError();
			return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
		}
		if (inst->width < 1 || inst->height < 1) {
			ReportError();
			return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
		}

		int32_t release_ret = Release();
		if (release_ret != WEBRTC_VIDEO_CODEC_OK) {
			ReportError();
			return release_ret;
		}

		int number_of_streams = webrtc::SimulcastUtility::NumberOfSimulcastStreams(*inst);
		bool doing_simulcast = (number_of_streams > 1);

		if (doing_simulcast && !webrtc::SimulcastUtility::ValidSimulcastParameters(*inst, number_of_streams)) {
			return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
		}

		assert(number_of_streams == 1);

		encoded_images_.resize(number_of_streams);
		x265_encoders_.resize(number_of_streams);
		x265_pictures_.resize(number_of_streams);
		x265_params_.resize(number_of_streams);		
		configurations_.resize(number_of_streams);
		
#if X265_ENCODE_DATA_SAVE
		outfiles_.resize(number_of_streams);
#endif

		number_of_cores_ = number_of_cores;
		max_payload_size_ = max_payload_size;
		codec_ = *inst;

		// Code expects simulcastStream resolutions to be correct, make sure they are
		// filled even when there are no simulcast layers.
		if (codec_.numberOfSimulcastStreams == 0) {
			codec_.simulcastStream[0].width = codec_.width;
			codec_.simulcastStream[0].height = codec_.height;
		}

		for (int i = 0, idx = number_of_streams - 1; i < number_of_streams; ++i, --idx) {
			// Set internal settings from codec_settings
			configurations_[i].simulcast_idx = idx;
			configurations_[i].sending = false;
			configurations_[i].width = codec_.simulcastStream[idx].width;
			configurations_[i].height = codec_.simulcastStream[idx].height;
			configurations_[i].max_frame_rate = static_cast<float>(codec_.maxFramerate);
			configurations_[i].frame_dropping_on = codec_.H265()->frameDroppingOn;
			configurations_[i].key_frame_interval = codec_.H265()->keyFrameInterval;

			// Codec_settings uses kbits/second; encoder uses bits/second.
			configurations_[i].max_bps = codec_.maxBitrate * 1000;
			configurations_[i].target_bps = codec_.maxBitrate * 1000 / 2;

			x265_param_default(&x265_params_[i]);
			x265_params_[i].bRepeatHeaders = 1; // write sps,pps before keyframe
			x265_params_[i].internalCsp = X265_CSP_I420;
			x265_params_[i].sourceWidth = configurations_[i].width;
			x265_params_[i].sourceHeight = configurations_[i].height;
			x265_params_[i].fpsNum = static_cast<int>(configurations_[i].max_frame_rate);
			x265_params_[i].fpsDenom = 1;
			x265_params_[i].bframes = 0;
			x265_params_[i].keyframeMin = configurations_[i].key_frame_interval;
			x265_params_[i].keyframeMax = configurations_[i].key_frame_interval;
			x265_params_[i].rc.rateControlMode = X265_RC_CRF;	//X265_RC_ABR X265_RC_CRF X265_RC_CQP
			x265_params_[i].rc.bitrate = configurations_[i].target_bps / 1000; // kbps
			x265_encoders_[i] = x265_encoder_open(&x265_params_[i]);

#if X265_ENCODE_DATA_SAVE
			std::string path = "f://h265_out_file_" + std::to_string(i) + ".h265";
			outfiles_[i].open(path.c_str(), std::ios::trunc | std::ios::binary);
			if (!outfiles_[i].is_open()) {
				RTC_LOG(LS_ERROR) << "x265 encode out file open failed...";
			}
#endif
			
			if (x265_encoders_[i] == nullptr) {
				RTC_LOG(LS_ERROR) << "Failed to open encoder.";
				Release();
				ReportError();
				return WEBRTC_VIDEO_CODEC_ERROR;
			}

			x265_pictures_[i] = x265_picture_alloc();
			x265_picture_init(&x265_params_[i], x265_pictures_[i]);

			// Initialize encoded image. Default buffer size: size of unencoded data.
			const size_t new_capacity = webrtc::CalcBufferSize(webrtc::VideoType::kI420,
				codec_.simulcastStream[idx].width, codec_.simulcastStream[idx].height);
			encoded_images_[i].SetEncodedData(webrtc::EncodedImageBuffer::Create(new_capacity));
			encoded_images_[i]._encodedWidth = codec_.simulcastStream[idx].width;
			encoded_images_[i]._encodedHeight = codec_.simulcastStream[idx].height;
			encoded_images_[i].set_size(0);

		}

		webrtc::SimulcastRateAllocator init_allocator(codec_);
		webrtc::VideoBitrateAllocation allocation = init_allocator.GetAllocation(
			codec_.maxBitrate * 1000 / 2, codec_.maxFramerate);
		SetRates(RateControlParameters(allocation, codec_.maxFramerate));
		return WEBRTC_VIDEO_CODEC_OK;
	}

	int32_t X265Encoder::Release()
	{
		while (!x265_encoders_.empty())
		{
			x265_encoder* encoder = x265_encoders_.back();
			if (encoder) {
				x265_encoder_close(encoder);
			}
			x265_encoders_.pop_back();
		}

		while (!x265_pictures_.empty())
		{
			x265_picture* picture = x265_pictures_.back();
			if (picture) {
				x265_picture_free(picture);
			}
			x265_pictures_.pop_back();
		}
		x265_params_.clear();
		x265_params_.shrink_to_fit();

		configurations_.clear();
		encoded_images_.clear();
		downscaled_buffers_.clear();

#if X265_ENCODE_DATA_SAVE
		for (int i=0; i < outfiles_.size(); i++)
		{
			if (outfiles_[i].is_open()) {
				outfiles_[i].close();
			}
		}
		outfiles_.clear();
		outfiles_.shrink_to_fit();
		
#endif

		return WEBRTC_VIDEO_CODEC_OK;
	}

	int32_t X265Encoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback)
	{
		encoded_image_callback_ = callback;
		return WEBRTC_VIDEO_CODEC_OK;
	}

	void X265Encoder::SetRates(const RateControlParameters& parameters)
	{
		if (x265_encoders_.empty()) {
			RTC_LOG(LS_WARNING) << "SetRates() while uninitialized.";
			return;
		}

		if (parameters.framerate_fps < 1.0) {
			RTC_LOG(LS_WARNING) << "Invalid frame rate: " << parameters.framerate_fps;
			return;
		}

		if (parameters.bitrate.get_sum_bps() == 0) {
			// Encoder paused, turn off all encoding.
			for (size_t i = 0; i < configurations_.size(); ++i)
				configurations_[i].SetStreamState(false);
			return;
		}

		codec_.maxFramerate = static_cast<uint32_t>(parameters.framerate_fps);

		size_t stream_idx = x265_encoders_.size() - 1;
		for (size_t i = 0; i < x265_encoders_.size(); ++i, --stream_idx) {
			// Update layer config.
			configurations_[i].target_bps =
				parameters.bitrate.GetSpatialLayerSum(stream_idx);
			configurations_[i].max_frame_rate = parameters.framerate_fps;

			if (configurations_[i].target_bps) {
				configurations_[i].SetStreamState(true);

				uint32_t fpsNum = static_cast<uint32_t>(configurations_[i].max_frame_rate);
				int bitrate = configurations_[i].target_bps / 1000; // kbps

				if (x265_params_[i].fpsNum != fpsNum || x265_params_[i].rc.bitrate != bitrate) {
					x265_params_[i].fpsNum = fpsNum;
					x265_params_[i].fpsDenom = 1;
					x265_params_[i].rc.bitrate = bitrate;
					int ret = x265_encoder_reconfig(x265_encoders_[i], &x265_params_[i]);
					if (ret < 0) {
						RTC_LOG(LS_ERROR) << "x265_encoder_reconfig failed, fps:" << fpsNum << ",bitrate:" << bitrate;
					}
				}
			}
			else {
				configurations_[i].SetStreamState(false);
			}
		}
	}

	int32_t X265Encoder::Encode(const webrtc::VideoFrame& input_frame,
		const std::vector<webrtc::VideoFrameType>* frame_types)
	{
		if (x265_encoders_.empty()) {
			ReportError();
			return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
		}

		if (!encoded_image_callback_) {
			RTC_LOG(LS_WARNING)
				<< "InitEncode() has been called, but a callback function "
				<< "has not been set with RegisterEncodeCompleteCallback()";
			ReportError();
			return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
		}

		rtc::scoped_refptr<const webrtc::I420BufferInterface> frame_buffer =
			input_frame.video_frame_buffer()->ToI420();
		if (!frame_buffer) {
			RTC_LOG(LS_ERROR) << "Failed to convert "
				<< VideoFrameBufferTypeToString(
					input_frame.video_frame_buffer()->type())
				<< " image to I420. Can't encode frame.";
			return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
		}

		RTC_CHECK(frame_buffer->type() == webrtc::VideoFrameBuffer::Type::kI420 ||
			frame_buffer->type() == webrtc::VideoFrameBuffer::Type::kI420A);

		bool send_key_frame = false;
		for (size_t i = 0; i < configurations_.size(); ++i) {
			if (configurations_[i].key_frame_request && configurations_[i].sending) {
				send_key_frame = true;
				break;
			}
		}

		if (!send_key_frame && frame_types) {
			for (size_t i = 0; i < configurations_.size(); ++i) {
				const size_t simulcast_idx =
					static_cast<size_t>(configurations_[i].simulcast_idx);
				if (configurations_[i].sending && simulcast_idx < frame_types->size() &&
					(*frame_types)[simulcast_idx] == webrtc::VideoFrameType::kVideoFrameKey) {
					send_key_frame = true;
					break;
				}
			}
		}

		RTC_DCHECK_EQ(configurations_[0].width, frame_buffer->width());
		RTC_DCHECK_EQ(configurations_[0].height, frame_buffer->height());

		// Encode image for each layer.
		for (size_t i = 0; i < x265_encoders_.size(); ++i) {
			if (!configurations_[i].sending) {
				continue;
			}

			if (frame_types != nullptr) {
				// Skip frame?
				if ((*frame_types)[i] == webrtc::VideoFrameType::kEmptyFrame) {
					continue;
				}
			}

			if (send_key_frame) {
				//x265_encoders_[i]->ForceIntraFrame(true);
				configurations_[i].key_frame_request = false;
			}
		
			x265_pictures_[i]->stride[0] = frame_buffer->StrideY();
			x265_pictures_[i]->stride[1] = frame_buffer->StrideU();
			x265_pictures_[i]->stride[2] = frame_buffer->StrideV();
			x265_pictures_[i]->planes[0] = const_cast<uint8_t*>(frame_buffer->DataY());
			x265_pictures_[i]->planes[1] = const_cast<uint8_t*>(frame_buffer->DataU());
			x265_pictures_[i]->planes[2] = const_cast<uint8_t*>(frame_buffer->DataV());

			std::vector<uint8_t> frame_packet;
			bool enc_ret = EncodeFrame((int)i, input_frame, frame_packet);
			if (!enc_ret) {
				ReportError();
				return WEBRTC_VIDEO_CODEC_ERROR;
			}

			if (frame_packet.size() == 0) {
				RTC_LOG(LS_ERROR) << "x265_encoder_encode frame_packet.size() is 0.";
				return WEBRTC_VIDEO_CODEC_OK;
			}
			
			encoded_images_[i]._encodedWidth = configurations_[i].width;
			encoded_images_[i]._encodedHeight = configurations_[i].height;
			encoded_images_[i].SetTimestamp(input_frame.timestamp());
			encoded_images_[i].ntp_time_ms_ = input_frame.ntp_time_ms();
			encoded_images_[i].capture_time_ms_ = input_frame.render_time_ms();
			encoded_images_[i].rotation_ = input_frame.rotation();
			encoded_images_[i].SetColorSpace(input_frame.color_space());
			encoded_images_[i].content_type_ = (codec_.mode == webrtc::VideoCodecMode::kScreensharing)
				? webrtc::VideoContentType::SCREENSHARE
				: webrtc::VideoContentType::UNSPECIFIED;
			encoded_images_[i].timing_.flags = webrtc::VideoSendTiming::kInvalid;
			//encoded_images_[i]._frameType = ConvertToVideoFrameType(info.eFrameType); // 是关键帧还是普通帧
			encoded_images_[i].SetSpatialIndex(configurations_[i].simulcast_idx);

			// Split encoded image up into fragments. This also updates
			// |encoded_image_|.
			RtpFragmentize(&encoded_images_[i], frame_packet);

			// Encoder can skip frames to save bandwidth in which case
			if (encoded_images_[i].size() > 0) {
				// Parse QP.
				h265_bitstream_parser_.ParseBitstream(encoded_images_[i]);
				encoded_images_[i].qp_ =
					h265_bitstream_parser_.GetLastSliceQp().value_or(-1);

				// Deliver encoded image.
				webrtc::CodecSpecificInfo codec_specific;
				codec_specific.codecType = webrtc::kVideoCodecH265;
				codec_specific.codecSpecific.H265.packetization_mode = packetization_mode_;
				//codec_specific.codecSpecific.H265.temporal_idx = webrtc::kNoTemporalIdx;
				//codec_specific.codecSpecific.H265.idr_frame = (info.eFrameType == videoFrameTypeIDR);
				//codec_specific.codecSpecific.H265.base_layer_sync = false;

				encoded_image_callback_->OnEncodedImage(encoded_images_[i], &codec_specific);
			}
		}

		return WEBRTC_VIDEO_CODEC_OK;
	}

	void X265Encoder::ReportInit()
	{
		if (has_reported_init_)
			return;
		has_reported_init_ = true;
	}

	void X265Encoder::ReportError()
	{
		if (has_reported_error_)
			return;
		has_reported_error_ = true;
	}

	webrtc::VideoEncoder::EncoderInfo X265Encoder::GetEncoderInfo() const
	{
		EncoderInfo info;
		info.supports_native_handle = false;
		info.implementation_name = "X265Encoder";
		//info.scaling_settings = VideoEncoder::ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
		info.is_hardware_accelerated = false;
		info.has_internal_source = false;
		info.supports_simulcast = true;
		info.preferred_pixel_formats = { webrtc::VideoFrameBuffer::Type::kI420 };
		return info;
	}

	void X265Encoder::LayerConfig::SetStreamState(bool send_stream)
	{
		if (send_stream && !sending) {
			// Need a key frame if we have not sent this stream before.
			key_frame_request = true;
		}
		sending = send_stream;
	}

	bool X265Encoder::EncodeFrame(int index, const webrtc::VideoFrame& input_frame,
		std::vector<uint8_t>& frame_packet)
	{
		frame_packet.clear();

		if (x265_encoders_.empty() || !x265_encoders_[index]) {
			RTC_LOG(LS_ERROR)
				<< "x265_encoders is empty, or current x265_encoder is null!" << index;
			return false;
		}

		x265_nal* nal = nullptr;
		uint32_t i_nal = 0;
		int ret = x265_encoder_encode(x265_encoders_[index], &nal, &i_nal, x265_pictures_[index], nullptr);
		if (ret < 0) {
			RTC_LOG(LS_WARNING) << "x265_encoder_encode failed...";
			return false;
		}

		for (uint32_t i = 0; i < i_nal; ++i)
		{
#if X265_ENCODE_DATA_SAVE
			if (outfiles_[index].is_open()) {
				outfiles_[index].write(reinterpret_cast<char*>(nal[i].payload), nal[i].sizeBytes);
			}
#endif
			frame_packet.insert(frame_packet.end(), nal[i].payload, nal[i].payload + nal[i].sizeBytes);
		}

		return true;

	}

}  // namespace krtc
