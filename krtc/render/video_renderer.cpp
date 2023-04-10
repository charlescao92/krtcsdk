// H:\webrtc\webrtc-checkout\src\test\video_renderer.cc

#include "krtc/render/video_renderer.h"

namespace krtc {

class NullRenderer : public VideoRenderer {
    void OnFrame(const webrtc::VideoFrame& video_frame) override {}
};

std::unique_ptr<VideoRenderer> VideoRenderer::Create(int hwnd, size_t width, size_t height)
{
    std::unique_ptr<VideoRenderer> renderer = CreatePlatformRenderer(hwnd, width, height);
    if (renderer != nullptr)
        return std::move(renderer);

    return std::make_unique<NullRenderer>();
}

}  // namespace krtc
