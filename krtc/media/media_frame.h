#ifndef XRTCSDK_XRTC_MEDIA_BASE_MEDIA_FRAME_H_
#define XRTCSDK_XRTC_MEDIA_BASE_MEDIA_FRAME_H_

namespace krtc {

enum class MainMediaType {
    kMainTypeCommon,
    kMainTypeAudio,
    kMainTypeVideo,
    kMainTypeData
};

enum class SubMediaType {
    kSubTypeCommon,
    kSubTypeI420,
    kSubTypeH264,
    kSubTypePcm,
    kSubTypeOpus,
};

struct AudioFormat {
    SubMediaType type;
    size_t nbytes_per_sample; // 16bit
    size_t samples_per_channel; // 每个声道的采样数
    size_t channels;
    uint32_t samples_per_sec;
    uint32_t total_delay_ms;
    bool key_pressed;
};

struct VideoFormat {
    SubMediaType type;
    int width;
    int height;
    bool idr;
};

class MediaFormat {
public:
    MainMediaType media_type;
    union {
        AudioFormat audio_fmt;
        VideoFormat video_fmt;
    } sub_fmt;
};

/*
class KRTC_API MediaFrame {
public:
    MediaFrame(int size) : max_size(size) {
        memset(data, 0, sizeof(data));
        memset(data_len, 0, sizeof(data_len));
        memset(stride, 0, sizeof(stride));
    }

    ~MediaFrame() {
        if (data[0]) {
            delete[] data[0];
            data[0] = nullptr;
        }
        if (data[1]) {
            delete[] data[1];
            data[1] = nullptr;
        }
        if (data[2]) {
            delete[] data[2];
            data[2] = nullptr;
        }
    }

public:
    int max_size;                // 结构最大容量
    MediaFormat fmt;             // 媒体类型，视频或者音频
    char* data[4];               // 默认用4个平面数组来存储数据
    int data_len[4];             // 每个平面数组的字节大小
    int stride[4];               // 每一行的大小
    uint32_t ts = 0;             // 帧的时间戳
    int64_t capture_time_ms = 0; // 采集时间
};
*/

class MediaFrame {
public:
    MediaFrame(int size) : max_size(size) {
        memset(data, 0, sizeof(data));
        memset(data_len, 0, sizeof(data_len));
        memset(stride, 0, sizeof(stride));
       // data[0] = new char[size];
        data_len[0] = size;
    }

    ~MediaFrame() {
        if (data[0]) {
            delete[] data[0];
            data[0] = nullptr;
        }
    }
    
public:
    int max_size;
    MediaFormat fmt;
    char* data[4];
    int data_len[4];
    int stride[4];
    uint32_t ts = 0;
    int64_t capture_time_ms = 0;
};

} // namespace xrtc

#endif // XRTCSDK_XRTC_MEDIA_BASE_MEDIA_FRAME_H_