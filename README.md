# 介绍
基于native webrtc m111，实现一个sdk接口，支持srs的webrtc推拉流
webrtc-m111是vs2019最后支持的webrtc版本，从m112开始只能用vs2022来编译了。。
 
# 功能
1. 支持webrtc推流`桌面+麦克风`或者`摄像机+麦克风`
2. 支持webrtc拉流
3. 支持获取网络质量GetStats
4. 支持回调获取音频裸流流
5. 支持回调获取视频裸流
6. 提供qt使用demo
7. 支持linux下的webrtc推拉流
8. 支持https或者http推流流
9. 支持window下的h264 nvenc和qsv硬件编码
10. 回调支持视频编码前的处理，demo提供了磨皮美颜
11. 推流过程中，允许暂停往网络发送本地音频流，而是静音帧
12. 推流过程中，允许暂停往网络发送本地视频流，而是黑屏帧
