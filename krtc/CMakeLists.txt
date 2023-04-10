cmake_minimum_required(VERSION 3.8)

if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif (POLICY CMP0091)

project(krtc)

file(GLOB all_src
	./*.cpp
	./base/*.cpp
	./media/*.cpp
	./device/*.cpp
	./net/*.cpp
	./tools/*.cpp
	./render/*.cpp
	./render/win/*.cpp
)

include_directories(
	${KRTC_DIR}
	${KRTC_THIRD_PARTY_DIR}/include
	${WEBRTC_DIR}
	${WEBRTC_DIR}/third_party/abseil-cpp
)

link_directories(
	${WEBRTC_LIB_DIR}
	${KRTC_THIRD_PARTY_DIR}/lib
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_definitions(-DKRTC_API_EXPORT
		-DNOMINMAX
	)
endif()

add_library(krtc SHARED ${all_src} "media/krtc_pull_impl.h" "media/krtc_pull_impl.cpp"  "media/krtc_media_base.h" "device/krtc_audio_track.h" "device/krtc_audio_track.cpp" "device/desktop_video_source.h" "device/desktop_video_source.cpp" "tools/timer.h" "tools/timer.cpp" "media/stats_collector.h" "media/stats_collector.cpp")
target_link_libraries(krtc
	webrtc
	winmm
	ws2_32
	Strmiids
	wldap32
	Crypt32
	iphlpapi
	msdmo
	dmoguids
	wmcodecdspuuid
	jsoncpp
	Secur32
	d3d11
	dxgi
)
