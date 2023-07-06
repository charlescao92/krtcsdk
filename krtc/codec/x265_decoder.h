#ifndef KRTCSDK_KRTC_CODEC_X265_DECODER_H_
#define KRTCSDK_KRTC_CODEC_X265_DECODER_H_

#include <memory>

extern "C" {
#include "ffmpeg/include/libavcodec/avcodec.h"
}  // extern "C"

#include "modules/video_coding/codecs/h265/include/h265.h"
#include "common_video/h265/h265_bitstream_parser.h"
#include "common_video/include/video_frame_buffer_pool.h"
#include "api/video_codecs/video_decoder.h"

namespace krtc {

    struct AVCodecContextDeleter {
        void operator()(AVCodecContext* ptr) const { avcodec_free_context(&ptr); }
    };
    struct AVFrameDeleter {
        void operator()(AVFrame* ptr) const { av_frame_free(&ptr); }
    };

    class H265DecoderImpl : public webrtc::VideoDecoder {
    public:
        H265DecoderImpl(const cricket::VideoCodec& codec);
        ~H265DecoderImpl() override;

        bool Configure(const Settings& settings) override;
        int32_t Release() override;

        int32_t RegisterDecodeCompleteCallback(
            webrtc::DecodedImageCallback* callback) override;

        // `missing_frames`, `fragmentation` and `render_time_ms` are ignored.
        int32_t Decode(const webrtc::EncodedImage& input_image,
            bool /*missing_frames*/,
            int64_t render_time_ms = -1) override;

        const char* ImplementationName() const override;

    private:
        // Called by FFmpeg when it needs a frame buffer to store decoded frames in.
        // The `VideoFrame` returned by FFmpeg at `Decode` originate from here. Their
        // buffers are reference counted and freed by FFmpeg using `AVFreeBuffer2`.
        static int AVGetBuffer2(AVCodecContext* context,
            AVFrame* av_frame,
            int flags);
        // Called by FFmpeg when it is done with a video frame, see `AVGetBuffer2`.
        static void AVFreeBuffer2(void* opaque, uint8_t* data);

        bool IsInitialized() const;

        // Reports statistics with histograms.
        void ReportInit();
        void ReportError();

        // Used by ffmpeg via `AVGetBuffer2()` to allocate I420 images.
        webrtc::VideoFrameBufferPool ffmpeg_buffer_pool_;
        // Used to allocate NV12 images if NV12 output is preferred.
        webrtc::VideoFrameBufferPool output_buffer_pool_;
        std::unique_ptr<AVCodecContext, AVCodecContextDeleter> av_context_;
        std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_;

        webrtc::DecodedImageCallback* decoded_image_callback_;

        bool has_reported_init_;
        bool has_reported_error_;

        webrtc::H265BitstreamParser h265_bitstream_parser_;

        // Decoder should produce this format if possible.
        const webrtc::VideoFrameBuffer::Type preferred_output_format_;
    };

}  // namespace krtc

#endif  // KRTCSDK_KRTC_CODEC_X265_DECODER_H_
