#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <atomic>

#include <QMainWindow>
#include <QMetaType>

#include "krtc/krtc.h"
#include "YUVOpenGLWidget.h"

#include "defs.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow,
                   public krtc::KRTCEngineObserver
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartDeviceBtnClicked();
    void onStartPreviewBtnClicked();
    void onStartPushBtnClicked();
    void onStartPullBtnClicked();
    void onShowToast(const QString& toast, bool err);
    void onBeautyCheckBoxStateChanged(int state);
    void onBeautyDermHSliderValueChanged(int value);

    void OnCapturePureVideoFrameSlots(MediaFrameSharedPointer videoFrame);
    void OnPullVideoFrameSlots(MediaFrameSharedPointer videoFrame);

signals:
    void showToastSignal(const QString& toast, bool err);
    void networkInfoSignal(const QString& text);
    void startDeviceFailedSignal();
    void previewFailedSignal();
    void pushFailedSignal();
    void pullFailedSignal();
    void pureVideoFrameSignal(MediaFrameSharedPointer videoFrame);
    void pullVideoFrameSignal(MediaFrameSharedPointer videoFrame);

private:
    Ui::MainWindow *ui;
    
    void initWindow();
    void initVideoComboBox();
    void initAudioComboBox();

    bool startDevice();
    bool stopDevice();
    bool startPreview();
    bool stopPreview();
    bool startPush();
    bool stopPush();
    bool startPull();
    bool stopPull();

    void showToast(const QString& toast, bool err);

private:
    // KRTCEngineObserver
    virtual void OnAudioSourceSuccess() override;
    virtual void OnAudioSourceFailed(krtc::KRTCError err) override;
    virtual void OnVideoSourceSuccess() override;
    virtual void OnVideoSourceFailed(krtc::KRTCError err) override;
    virtual void OnPreviewSuccess() override;
    virtual void OnPreviewFailed(krtc::KRTCError err) override;
    virtual void OnNetworkInfo(uint64_t rtt_ms, uint64_t packets_lost, double fraction_lost) override;
    virtual void OnPushSuccess() override;
    virtual void OnPushFailed(krtc::KRTCError err) override;
    virtual void OnPullSuccess() override;
    virtual void OnPullFailed(krtc::KRTCError err) override;
    virtual void OnCapturePureVideoFrame(std::shared_ptr<krtc::MediaFrame> video_frame) override;
    virtual void OnPullVideoFrame(std::shared_ptr<krtc::MediaFrame> video_frame) override;
    virtual krtc::MediaFrame* OnPreprocessVideoFrame(krtc::MediaFrame* origin_frame);

private:
    krtc::IVideoHandler* cam_source_ = nullptr;
    krtc::IVideoHandler* desktop_source_ = nullptr;
    krtc::IAudioHandler* mic_source_ = nullptr;
    std::atomic<bool> audio_init_{ false };
    std::atomic<bool> video_init_{ false };
    krtc::IMediaHandler* krtc_preview_ = nullptr;
    krtc::IMediaHandler* krtc_pusher_ = nullptr;
    krtc::IMediaHandler* krtc_puller_ = nullptr;
    bool enable_beauty_ = false;
    int beauty_index_ = 20;
};

#endif // MAINWINDOW_H
