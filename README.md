# 介绍
https://www.yuque.com/caokunchao/rtendq/wc1ziicghs350meo   
基于native webrtc m96，实现一个sdk接口，支持srs的webrtc推拉流

# 功能
1. 支持webrtc推流桌面+麦克风或者摄像机+麦克风
2. 支持webrtc拉流
3. 支持获取网络质量GetStats
4. 支持回调获取音频频流
5. 支持回调获取视频裸流
6. 提供qt使用demo
7. 支持linux下的webrtc推拉流
8. 支持https或者http推流流
9. 支持nvenc和qsv硬件编码


# 待做
1. 推流过程中，允许或禁止往网络发送本地音频流，不影响录音
2. 推流过程中，允许或禁止往网络发送本地视频流
3. 回调支持视频编码前的处理，例如美颜滤镜
4. 回调支持自定义视频编码
5. 回调支持音频编码前的处理，例如变声
