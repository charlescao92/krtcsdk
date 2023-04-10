//H:\webrtc\webrtc-checkout\src\test\win\d3d_renderer.cc

#include "krtc/render/win/d3d_renderer.h"

#include <common_video/libyuv/include/webrtc_libyuv.h>
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <rtc_base/task_utils/to_queued_task.h>
#include <api/video/video_frame.h>

#include "krtc/base/krtc_global.h"

namespace krtc {

std::unique_ptr<VideoRenderer> VideoRenderer::CreatePlatformRenderer(int hwnd,
                                                     size_t width,
                                                     size_t height) 
{
	return D3dRenderer::Create((HWND)hwnd, width, height);
}

D3dRenderer::D3dRenderer(HWND hwnd, size_t width, size_t height)
    : hwnd_(hwnd),
	width_(width),
    height_(height)  
{
  RTC_DCHECK_GT(width, 0);
  RTC_DCHECK_GT(height, 0);

}

D3dRenderer::~D3dRenderer() {
  Destroy();
}

void D3dRenderer::Destroy() {
	RTC_LOG(LS_INFO) << "D3D9RenderSink Destroy";
	if (d3d9_surface_) {
		d3d9_surface_->Release();
		d3d9_surface_ = nullptr;
	}

	if (d3d9_device_) {
		d3d9_device_->Release();
		d3d9_device_ = nullptr;
	}

	if (d3d9_) {
		d3d9_->Release();
		d3d9_ = nullptr;
	}

	if (rgb_buffer_) {
		delete[] rgb_buffer_;
		rgb_buffer_ = nullptr;
		rgb_buffer_size_ = 0;
	}
}

void D3dRenderer::HandleErr(const KRTCError& err)
{
	return;

	
}

bool D3dRenderer::TryInit(const webrtc::VideoFrame& frame) {
	KRTCError err = KRTCError::kNoErr;

	do {
		if (!d3d9_ || !d3d9_device_ || !d3d9_surface_) {
			break;
		}

		if (width_ != frame.width() ||
			height_ != frame.height())
		{
			break;
		}
		HandleErr(err);
		return true;
	} while (0);

	if (!IsWindow(hwnd_)) {
		if (isInvalidHwnd_) {
			return false;
		}
		isInvalidHwnd_ = true;
		RTC_LOG(LS_WARNING) << "Invalid hwnd";

		HandleErr(KRTCError::kPreviewNoRenderHwndErr);
		return false;
	}

	if (!GetClientRect(hwnd_, &rt_viewport_)) {
		RTC_LOG(LS_WARNING) << "GetClientRect failed, hwnd: " << hwnd_;

		HandleErr(KRTCError::kPreviewNoRenderHwndErr);
		return false;
	}

	// 1. ����d3d9����
	if (!d3d9_) {
		d3d9_ = Direct3DCreate9(D3D_SDK_VERSION);
		if (!d3d9_) {
			RTC_LOG(LS_WARNING) << "d3d9 create failed";
			HandleErr(KRTCError::kPreviewCreateRenderDeviceErr);
			return false;
		}
	}

	// 2. ����d3d9 device
	if (!d3d9_device_) {
		// ����һ��d3d9 device
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = true; // ����ģʽ
		// һ����̨�����������ݱ����Ƶ���Ļ����ô�����������ݾ�û������
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		HRESULT res = d3d9_->CreateDevice(
			D3DADAPTER_DEFAULT, // ָ��Ҫ��ʾ�������豸��Ĭ������ʾ��
			D3DDEVTYPE_HAL,     // ֧��Ӳ������
			hwnd_,              // ��Ⱦ���ھ��                                                    
			D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,// ָ�����㴦��ʽ�����&���߳�
			&d3dpp,
			&d3d9_device_
		);

		if (FAILED(res)) {
			RTC_LOG(LS_WARNING) << "d3d9 create device failed: " << res;
			HandleErr(KRTCError::kPreviewCreateRenderDeviceErr);
			return false;
		}
	}

	// 3. ������������
	if (d3d9_surface_) {
		d3d9_surface_->Release();
		d3d9_surface_ = nullptr;
	}

	HRESULT res = res = d3d9_device_->CreateOffscreenPlainSurface(
		frame.width(),						// ��������Ŀ��
		frame.height(),						// ��������ĸ߶�
		D3DFMT_X8R8G8B8,					// ͼ�����ظ�ʽ
		D3DPOOL_DEFAULT,					// ��Դ��Ӧ���ڴ�����
		&d3d9_surface_,						// ���ش�����surface
		NULL
	);

	if (FAILED(res)) {
		RTC_LOG(LS_WARNING) << "d3d9 create offscreen surface failed: " << res;
		HandleErr(KRTCError::kPreviewCreateRenderDeviceErr);
		return false;
	}

	width_ = frame.width();
	height_ = frame.height();

	return true;
}

std::unique_ptr<VideoRenderer> D3dRenderer::Create(
                                 HWND hwnd,
                                 size_t width,
                                 size_t height) 
{
  RTC_LOG(LS_INFO) << "D3dRenderer Create";

  D3dRenderer* d3d_renderer = new D3dRenderer(hwnd, width, height);

  return absl::WrapUnique(d3d_renderer);
}


void D3dRenderer::DoRender(const webrtc::VideoFrame& frame) {
	rtc::scoped_refptr<webrtc::VideoFrameBuffer> vfb = frame.video_frame_buffer();
	if (webrtc::VideoFrameBuffer::Type::kI420 == vfb.get()->type()) {
		// 1. ����RGB buffer����YUV��ʽת����RGB��ʽ
		int size = frame.width() * frame.height() * 4; // ����ARGB�����������ǳ���3
		if (rgb_buffer_size_ != size) {		// ������Ⱦ������ͼ���߷����仯
			if (rgb_buffer_) {
				delete[] rgb_buffer_;
			}

			rgb_buffer_ = new char[size];
			rgb_buffer_size_ = size;
		}

		ConvertFromI420(frame, webrtc::VideoType::kARGB, 0,
			(uint8_t*)rgb_buffer_);

		// YUV��ʽת����RGB
		/*libyuv::I420ToARGB((const uint8_t*)frame->data[0], frame->stride[0],
			(const uint8_t*)frame->data[1], frame->stride[1],
			(const uint8_t*)frame->data[2], frame->stride[2],
			(uint8_t*)rgb_buffer_, width_ * 4,
			width_, height_);*/
	}

	// 2. ��RGB���ݿ�������������
	// 2.1 ��������
	HRESULT res;
	D3DLOCKED_RECT d3d9_rect;
	res = d3d9_surface_->LockRect(&d3d9_rect, // ָ��������������ľ��νṹָ��
		NULL, // ��Ҫ�������������NULL����ʾ��������
		D3DLOCK_DONOTWAIT // ���Կ���������ʧ�ܵ�ʱ�򣬲�����Ӧ�ó���,����D3DERR_WASSTILLDRAWING
	);

	if (FAILED(res)) {
		RTC_LOG(LS_WARNING) << "d3d9 surface LockRect failed: " << res;
		return;
	}

	// 2.2 ��RGB���ݿ�������������
	// ��������ĵ�ַ
	byte* pdest = (byte*)d3d9_rect.pBits;
	// ��������ÿһ�е����ݴ�С
	int stride = d3d9_rect.Pitch;

	if (webrtc::VideoFrameBuffer::Type::kI420 == vfb.get()->type()) {
		int video_width = frame.width();
		int video_height = frame.height();
		int video_stride = video_width * 4;

		if (video_stride == stride) {
			memcpy(pdest, rgb_buffer_, rgb_buffer_size_);
		}
		else if (video_stride < stride) {
			char* src = rgb_buffer_;
			for (int i = 0; i < video_height; ++i) {
				memcpy(pdest, src, video_stride);
				pdest += stride;
				src += video_stride;
			}
		}
	}

	// 2.3 �������
	d3d9_surface_->UnlockRect();

	// 3. �����̨����
	d3d9_device_->Clear(0,          // �ڶ���������������Ĵ�С
		NULL,                       // ��Ҫ����ľ������飬�����NULL����ʾ�������
		D3DCLEAR_TARGET,            // �����ȾĿ��
		D3DCOLOR_XRGB(30, 30, 30),  // ʹ�õ���ɫֵ
		1.0f, 0);

	// 4. ��������������ݿ�������̨�������
	d3d9_device_->BeginScene();
	// 4.1 ��ȡ��̨�������
	IDirect3DSurface9* pback_buffer = nullptr;
	d3d9_device_->GetBackBuffer(0, // ����ʹ�õĽ�����������
		0,							// ��̨������������
		D3DBACKBUFFER_TYPE_MONO,	// ��̨�����������
		&pback_buffer);

	// 4.2����ͼ�����ʾ����Ŀ�߱�
	// ��ʾ����Ŀ��
	float w1 = rt_viewport_.right - rt_viewport_.left;
	float h1 = rt_viewport_.bottom - rt_viewport_.top;
	// ͼ��Ŀ��
	float w2 = (float)width_;
	float h2 = (float)height_;
	// ����Ŀ������Ĵ�С
	int dst_w = 0;
	int dst_h = 0;
	int x, y = 0;

	if (w1 > (w2 * h1) / h2) { // ԭʼ����ʾ���򣬿�ȸ���һЩ
		dst_w = (w2 * h1) / h2;
		dst_h = h1;
		x = (w1 - dst_w) / 2;
		y = 0;
	}
	else {   // ԭʼ����ʾ���򣬸߶ȸ���һЩ
		dst_w = w1;
		dst_h = (h2 * w1) / w2;
		x = 0;
		y = (h1 - dst_h) / 2;
	}

	RECT dest_rect{ x, y, x + dst_w, y + dst_h };

	// 4.3.����������������ݵ���̨�������
	d3d9_device_->StretchRect(d3d9_surface_, // Դ����
		NULL,           // Դ����ľ������������NULL����ʾ��������
		pback_buffer,   // Ŀ�����
		NULL,//&dest_rect,			// Ŀ�����ľ����������NULL����ʾ��������
		D3DTEXF_LINEAR  // �����㷨�����Բ�ֵ
	);
	d3d9_device_->EndScene();

	// 5. ��ʾͼ�񣬱��淭ת
	d3d9_device_->Present(NULL, // ��̨�������ľ������� NULL��ʾ��������
		NULL,	// ��ʾ���� NULL��ʾ�����ͻ���ʾ����
		NULL,   // ��ʾ���ڣ� NULL��ʾ������
		NULL);

	// 6. �ͷź�̨����
	pback_buffer->Release();
}

void D3dRenderer::OnFrame(const webrtc::VideoFrame& frame) {
    // worker_threadִ����Ⱦ����
    KRTCGlobal::Instance()->worker_thread()->PostTask(webrtc::ToQueuedTask([=] {
        if (!TryInit(frame)) {
            return;
        }
        DoRender(frame);
    }));
}

}  // namespace krtc
