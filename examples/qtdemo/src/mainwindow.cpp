#include <map>

#include <QVariant>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
   
    initWindow();
}

MainWindow::~MainWindow()
{
	stopPush();
	stopPreview();
	stopDevice();
    delete ui;
}

void MainWindow::initWindow()
{
	setWindowTitle(QStringLiteral("srs webrtc推拉流qtdemo"));
    setMinimumSize(1400, 600);
	
	ui->leftWidget->setFixedWidth(250);

	ui->previewOpenGLWidget->setMinimumSize(400,400);
	ui->pullOpenGLWidget->setMinimumSize(400, 400);

#if 0
    ui->serverLineEdit->setText("http://charlescao92.cn:1985");
#else
	ui->serverLineEdit->setText("https://charlescao92.cn:1986");
#endif

    ui->uidLineEdit->setText("111");
    ui->pushStreamNamelineEdit->setText("livestream");
	ui->networkQosLabel->setText("");
	ui->statusLabel->setText("");

	ui->pullStreamNameLineEdit->setText("livestream");
	ui->pullUidLineEdit->setText("222");
	ui->cameraRadioButton->setChecked(true);

	ui->beautyDermHSlider->setRange(1, 40);
	ui->beautyDermHSlider->setSingleStep(10);
	ui->beautyDermHSlider->setValue(20);
	ui->beautyDermHSlider->setEnabled(false);

    connect(ui->startDeviceBtn, &QPushButton::clicked,
        this, &MainWindow::onStartDeviceBtnClicked);
    connect(ui->startPreviewBtn, &QPushButton::clicked,
        this, &MainWindow::onStartPreviewBtnClicked);
    connect(ui->startPushBtn, &QPushButton::clicked,
        this, &MainWindow::onStartPushBtnClicked);
    connect(ui->startPullBtn, &QPushButton::clicked,
        this, &MainWindow::onStartPullBtnClicked);
	connect(this, &MainWindow::showToastSignal,
		this, &MainWindow::onShowToast, Qt::QueuedConnection);
	connect(this, &MainWindow::pureVideoFrameSignal,
		this, &MainWindow::OnCapturePureVideoFrameSlots, Qt::QueuedConnection);
	connect(this, &MainWindow::pullVideoFrameSignal,
		this, &MainWindow::OnPullVideoFrameSlots, Qt::QueuedConnection);

	connect(this, &MainWindow::networkInfoSignal,
		[this](const QString& text) {
			ui->networkQosLabel->setText(text);
	});
	connect(this, &MainWindow::startDeviceFailedSignal,
		this, [this] {
			stopDevice();
			ui->startDeviceBtn->setText(QStringLiteral("启动音视频设备"));
		});
	connect(this, &MainWindow::previewFailedSignal,
		[this] {
			stopPreview();
			ui->startPreviewBtn->setText(QStringLiteral("本地预览"));
		});
	connect(this, &MainWindow::pushFailedSignal,
		[this] {
			stopPush();
			ui->startPushBtn->setText(QStringLiteral("开始推流"));
		});
	connect(this, &MainWindow::pullFailedSignal,
		[this] {
			stopPull();
			ui->startPullBtn->setText(QStringLiteral("开始拉流"));
		});
	connect(ui->beautyCheckBox, SIGNAL(stateChanged(int)),
		this, SLOT(onBeautyCheckBoxStateChanged(int)));
	connect(ui->beautyDermHSlider, SIGNAL(valueChanged(int)),
		this, SLOT(onBeautyDermHSliderValueChanged(int)));

	krtc::KRTCEngine::Init(this);

	initVideoComboBox();
	initAudioComboBox();

	qRegisterMetaType<MediaFrameSharedPointer>("MediaFrameSharedPointer");

}

void MainWindow::initVideoComboBox()
{
	int total = krtc::KRTCEngine::GetCameraCount();
	if (total <= 0) {
		return;
	}

	for (int i = 0; i < total; ++i) {
		char device_name[128] = { 0 };
		char device_id[128] = { 0 };
        krtc::KRTCEngine::GetCameraInfo(i, device_name, sizeof(device_name), device_id, sizeof(device_id));

		QVariant useVar;
		DeviceInfo info = { device_name, device_id };
		useVar.setValue(info);
		ui->videoDeviceCbx->addItem(QString::fromStdString(device_name), useVar);
	}
}

void MainWindow::initAudioComboBox()
{
	int total = krtc::KRTCEngine::GetMicCount();
	if (total <= 0) {
		return;
	}

	for (int i = 0; i < total; ++i) {
		char device_name[128] = { 0 };
		char device_id[128] = { 0 };
        krtc::KRTCEngine::GetMicInfo(i, device_name, sizeof(device_name), device_id, sizeof(device_id));

		QVariant useVar;
		DeviceInfo info = { device_name, device_id };
		useVar.setValue(info);
		ui->audioDeviceCbx->addItem(QString::fromStdString(device_name), useVar);
	}
}

void MainWindow::onStartDeviceBtnClicked()
{
	ui->startDeviceBtn->setEnabled(false);
	if (!audio_init_ || !video_init_) {
		if (startDevice()) {
			ui->startDeviceBtn->setText(QStringLiteral("停止音视频设备"));
		}
	}
	else {
		if (stopDevice()) {
			ui->startDeviceBtn->setText(QStringLiteral("启动音视频设备"));
		}
	}
	ui->startDeviceBtn->setEnabled(true);
}

void MainWindow::onStartPreviewBtnClicked()
{
	ui->startPreviewBtn->setEnabled(false);

	if (!krtc_preview_) {
		if (startPreview()) {
			ui->startPreviewBtn->setText(QStringLiteral("停止预览"));
		}
	}
	else {
		if (stopPreview()) {
			ui->startPreviewBtn->setText(QStringLiteral("本地预览"));
		}
	}

	ui->startPreviewBtn->setEnabled(true);
}

void MainWindow::onStartPushBtnClicked()
{
	ui->startPushBtn->setEnabled(false);
	if (!krtc_pusher_) {
		if (startPush()) {
			ui->startPushBtn->setText(QStringLiteral("停止推流"));
		}
	}
	else {
		if (stopPush()) {
			ui->startPushBtn->setText(QStringLiteral("开始推流"));
		}
	}

	ui->startPushBtn->setEnabled(true);
}

void MainWindow::onStartPullBtnClicked()
{
	ui->startPullBtn->setEnabled(false);
	if (!krtc_puller_) {
		if (startPull()) {
			ui->startPullBtn->setText(QStringLiteral("停止拉流"));
		}
	}
	else {
		if (stopPull()) {
			ui->startPullBtn->setText(QStringLiteral("开始拉流"));
		}
	}

	ui->startPullBtn->setEnabled(true);
}

void MainWindow::onShowToast(const QString& toast, bool err)
{
	if (err) {
		ui->statusLabel->setStyleSheet("color:red;");
	}
	else {
		ui->statusLabel->setStyleSheet("color:blue;");
	}
	ui->statusLabel->setText(toast);
}

void MainWindow::onBeautyCheckBoxStateChanged(int state)
{
	if (state == Qt::CheckState::Checked)
	{
		enable_beauty_ = true;
		ui->beautyDermHSlider->setEnabled(true);
	}
	else
	{
		enable_beauty_ = false;
		ui->beautyDermHSlider->setEnabled(false);
	}

}

void MainWindow::onBeautyDermHSliderValueChanged(int value)
{
	beauty_index_ = value;
}

void MainWindow::OnCapturePureVideoFrameSlots(MediaFrameSharedPointer videoFrame)
{
	ui->previewOpenGLWidget->updateFrame(videoFrame);
}

void MainWindow::OnPullVideoFrameSlots(MediaFrameSharedPointer videoFrame)
{
	ui->pullOpenGLWidget->updateFrame(videoFrame);
}

bool MainWindow::startDevice() {
	if (!video_init_) {
		if (ui->cameraRadioButton->isChecked()) {
			int index = ui->videoDeviceCbx->currentIndex();
			QVariant useVar = ui->videoDeviceCbx->itemData(index);
			std::string device_id = useVar.value<DeviceInfo>().device_id;
			cam_source_ = krtc::KRTCEngine::CreateCameraSource(device_id.c_str());
			cam_source_->Start();
		}
		else if (ui->desktopRadioButton->isChecked()) {
			desktop_source_ = krtc::KRTCEngine::CreateScreenSource();
			desktop_source_->Start();
		}
		
	}

	if (!audio_init_) {
		int index = ui->audioDeviceCbx->currentIndex();
		QVariant useVar = ui->audioDeviceCbx->itemData(index);

		std::string device_id = useVar.value<DeviceInfo>().device_id;
		mic_source_ = krtc::KRTCEngine::CreateMicSource(device_id.c_str());
		mic_source_->Start();
	}

	return true;
}

bool MainWindow::stopDevice() {
	if (video_init_ && cam_source_) {
		cam_source_->Stop();
		cam_source_->Destroy();
		cam_source_ = nullptr;
		video_init_ = false;
	}

	if (video_init_ && desktop_source_) {
		desktop_source_->Stop();
		desktop_source_->Destroy();
		desktop_source_ = nullptr;
		video_init_ = false;
	}

	if (audio_init_ && mic_source_) {
		mic_source_->Stop();
		mic_source_->Destroy();
		mic_source_ = nullptr;
		audio_init_ = false;
	}

	return true;
}

bool MainWindow::startPreview() {
	if (!cam_source_ && !desktop_source_) {
		showToast(QStringLiteral("预览失败：没有视频源"), true);
		return false;
	}

#if 0
	HWND hwnd = (HWND)ui->previewWidget->winId();
	if (!hwnd) {
		showToast(QStringLiteral("预览失败：没有显示窗口"), true);
		return false;
	}
#endif
	
    krtc_preview_ = krtc::KRTCEngine::CreatePreview();
	krtc_preview_->Start();

	return true;
}

bool MainWindow::stopPreview() {
	if (!krtc_preview_) {
		return false;
	}

	krtc_preview_->Stop();
	krtc_preview_ = nullptr;

	showToast(QStringLiteral("停止本地预览成功"), false);

	return true;
}

bool MainWindow::startPush() {
	if (!cam_source_ && !desktop_source_) {
		showToast(QStringLiteral("推流失败：没有视频源"), true);
		return false;
	}

	QString url = ui->serverLineEdit->text();
	if (url.isEmpty()) {
		showToast(QStringLiteral("推流失败：没有服务器地址"), true);
		return false;
	}
	QString channel = ui->pushStreamNamelineEdit->text();
	krtc_pusher_ = krtc::KRTCEngine::CreatePusher(url.toUtf8().constData(), channel.toUtf8().constData());
	krtc_pusher_->Start();

	return true;
}

bool MainWindow::stopPush() {
	if (!krtc_pusher_) {
		return true;
	}

	krtc_pusher_->Stop();
	krtc_pusher_->Destroy();
	krtc_pusher_ = nullptr;

	return true;
}

bool MainWindow::startPull()
{
	QString url = ui->serverLineEdit->text();
	if (url.isEmpty()) {
		showToast(QStringLiteral("拉流失败：没有服务器地址"), true);
		return false;
	}

	QString channel = ui->pullStreamNameLineEdit->text();
    krtc_puller_ = krtc::KRTCEngine::CreatePuller(url.toUtf8().constData(), channel.toUtf8().constData());
	krtc_puller_->Start();

	return true;
}

bool MainWindow::stopPull()
{
	if (!krtc_puller_) {
		return true;
	}

	krtc_puller_->Stop();
	krtc_puller_->Destroy();
	krtc_puller_ = nullptr;

	return true;
}

void MainWindow::showToast(const QString& toast, bool err)
{
	emit showToastSignal(toast, err);
}

void MainWindow::OnAudioSourceSuccess() {
	audio_init_ = true;
	showToast(QStringLiteral("麦克风启动成功"), false);
}

void MainWindow::OnAudioSourceFailed(krtc::KRTCError err)
{
	QString err_msg = QStringLiteral("麦克风启动失败: ") + QString::fromStdString(krtc::KRTCEngine::GetErrString(err));
	showToast(err_msg, true);

	emit startDeviceFailedSignal();
}

void MainWindow::OnVideoSourceSuccess() {
	video_init_ = true;
	showToast(QStringLiteral("摄像头启动成功"), false);
}

void MainWindow::OnVideoSourceFailed(krtc::KRTCError err)
{
	QString err_msg = QStringLiteral("摄像头启动失败: ") + QString::fromStdString(krtc::KRTCEngine::GetErrString(err));
	showToast(err_msg, true);
	emit startDeviceFailedSignal();
}

void MainWindow::OnPreviewSuccess() {
	showToast(QStringLiteral("本地预览成功"), false);
}

void MainWindow::OnPreviewFailed(krtc::KRTCError err) {
	QString err_msg = QStringLiteral("本地预览失败: ") + QString::fromStdString(krtc::KRTCEngine::GetErrString(err));
	showToast(err_msg, true);

	if (krtc_preview_) {
		krtc_preview_->Stop();
		krtc_preview_ = nullptr;
	}

	emit previewFailedSignal();

}

void MainWindow::OnPushSuccess() {
	showToast(QStringLiteral("推流成功"), false);
}

void MainWindow::OnPushFailed(krtc::KRTCError err) {
	QString err_msg = QStringLiteral("推流失败: ") + QString::fromStdString(krtc::KRTCEngine::GetErrString(err));
	showToast(err_msg, true);

	emit pushFailedSignal();
}

void MainWindow::OnPullSuccess() {
	showToast(QStringLiteral("拉流成功"), false);
}

void MainWindow::OnPullFailed(krtc::KRTCError err) {
	QString err_msg = QStringLiteral("拉流失败: ") + QString::fromStdString(krtc::KRTCEngine::GetErrString(err));
	showToast(err_msg, true);

	emit pullFailedSignal();
}

void MainWindow::OnCapturePureVideoFrame(std::shared_ptr<krtc::MediaFrame> frame)
{
	int src_width = frame->fmt.sub_fmt.video_fmt.width;
	int src_height = frame->fmt.sub_fmt.video_fmt.height;
	int stridey = frame->stride[0];
	int strideu = frame->stride[1];
	int stridev = frame->stride[2];
	int size = stridey * src_height + (strideu + stridev) * ((src_height + 1) / 2);

	MediaFrameSharedPointer video_frame(new krtc::MediaFrame(size));
	video_frame->fmt.sub_fmt.video_fmt.type = krtc::SubMediaType::kSubTypeI420;
	video_frame->fmt.sub_fmt.video_fmt.width = src_width;
	video_frame->fmt.sub_fmt.video_fmt.height = src_height;
	video_frame->stride[0] = stridey;
	video_frame->stride[1] = strideu;
	video_frame->stride[2] = stridev;
	video_frame->data_len[0] = stridey * src_height;
	video_frame->data_len[1] = strideu * ((src_height + 1) / 2);
	video_frame->data_len[2] = stridev * ((src_height + 1) / 2);
	video_frame->data[0] = new char[video_frame->data_len[0]];
	video_frame->data[1] = new char[video_frame->data_len[1]];
	video_frame->data[2] = new char[video_frame->data_len[2]];
	memcpy(video_frame->data[0], frame->data[0], frame->data_len[0]);
	memcpy(video_frame->data[1], frame->data[1], frame->data_len[1]);
	memcpy(video_frame->data[2], frame->data[2], frame->data_len[2]);

	emit pureVideoFrameSignal(video_frame);
}

void MainWindow::OnPullVideoFrame(std::shared_ptr<krtc::MediaFrame> frame)
{
	int src_width = frame->fmt.sub_fmt.video_fmt.width;
	int src_height = frame->fmt.sub_fmt.video_fmt.height;
	int stridey = frame->stride[0];
	int strideu = frame->stride[1];
	int stridev = frame->stride[2];
	int size = stridey * src_height + (strideu + stridev) * ((src_height + 1) / 2);

	MediaFrameSharedPointer video_frame(new krtc::MediaFrame(size));
	video_frame->fmt.sub_fmt.video_fmt.type = krtc::SubMediaType::kSubTypeI420;
	video_frame->fmt.sub_fmt.video_fmt.width = src_width;
	video_frame->fmt.sub_fmt.video_fmt.height = src_height;
	video_frame->stride[0] = stridey;
	video_frame->stride[1] = strideu;
	video_frame->stride[2] = stridev;
	video_frame->data_len[0] = stridey * src_height;
	video_frame->data_len[1] = strideu * ((src_height + 1) / 2);
	video_frame->data_len[2] = stridev * ((src_height + 1) / 2);
	video_frame->data[0] = new char[video_frame->data_len[0]];
	video_frame->data[1] = new char[video_frame->data_len[1]];
	video_frame->data[2] = new char[video_frame->data_len[2]];
	memcpy(video_frame->data[0], frame->data[0], frame->data_len[0]);
	memcpy(video_frame->data[1], frame->data[1], frame->data_len[1]);
	memcpy(video_frame->data[2], frame->data[2], frame->data_len[2]);

	emit pullVideoFrameSignal(video_frame);
}

static void CopyYUVToImage(uchar* dst, uint8_t* pY, uint8_t* pU, uint8_t* pV, int width, int height)
{
	uint32_t size = width * height;
	memcpy(dst, pY, size);
	memcpy(dst + size, pU, size / 4);
	memcpy(dst + size + size / 4, pV, size / 4);
}

static void CopyImageToYUV(uint8_t* pY, uint8_t* pU, uint8_t* pV, uchar* src, int width, int height)
{
	uint32_t size = width * height;
	memcpy(pY, src, size);
	memcpy(pU, src + size, size / 4);
	memcpy(pV, src + size + size / 4, size / 4);
}

krtc::MediaFrame* MainWindow::OnPreprocessVideoFrame(krtc::MediaFrame* origin_frame)
{
	if (!enable_beauty_) {
		return origin_frame;
	}

	int width = origin_frame->fmt.sub_fmt.video_fmt.width;
	int height = origin_frame->fmt.sub_fmt.video_fmt.height;

	// yuv转为cv::Mat
	cv::Mat yuvImage;
	yuvImage.create(height * 3 / 2, width, CV_8UC1);
	CopyYUVToImage(yuvImage.data, 
		(uint8_t*)origin_frame->data[0],
		(uint8_t*)origin_frame->data[1],
		(uint8_t*)origin_frame->data[2],
		width, height);
	cv::Mat rgbImage;
	cv::cvtColor(yuvImage, rgbImage, cv::COLOR_YUV2BGR_I420);

	// 美颜
	cv::Mat dst_image;
#if 0
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3), cv::Point(-1, -1));
	cv::Mat image_erode;
    erode(rgbImage, image_erode, element, cv::Point(-1, -1), 1);		//腐蚀
    erode(image_erode, dst_image, element, cv::Point(-1, -1), 1);	//腐蚀
#else
	bilateralFilter(rgbImage, dst_image, beauty_index_, beauty_index_ * 2, beauty_index_ * 2);
#endif
	// cv::Mat转为yuv
	cv::Mat dstYuvImage;
	cv::cvtColor(dst_image, dstYuvImage, cv::COLOR_BGR2YUV_I420);
	CopyImageToYUV(
		(uint8_t*)origin_frame->data[0],
		(uint8_t*)origin_frame->data[1],
		(uint8_t*)origin_frame->data[2], 
		dstYuvImage.data,
		width, height);

	return std::move(origin_frame);
}

void MainWindow::OnNetworkInfo(uint64_t rtt_ms, uint64_t packets_lost, double fraction_lost)
{
	char str[256] = { 0 };
	snprintf(str, sizeof(str), "rtt=%lldms 累计丢包数=%lld 丢包指数=%f",
		rtt_ms, packets_lost, fraction_lost);
	QString networkInfo = QString::fromLocal8Bit(str);
	emit networkInfoSignal(networkInfo);
}

