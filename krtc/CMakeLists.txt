cmake_minimum_required(VERSION 3.8)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    set(CMAKE_CXX_FLAGS "-fno-rtti -g -pipe -W -Wall -fPIC")
endif()

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
	if(MSVC)
		add_definitions(-DNOMINMAX)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	endif()
endif()

file(GLOB all_src
	./*.cpp
	./base/*.cpp
	./media/*.cpp
	./device/*.cpp
	./tools/*.cpp
	./render/*.cpp
	./render/win/*.cpp
    ./codec/*.cpp
    ./codec/encoder/*.cpp
    ./codec/nvcodec/NvEncoder/NvEncoder.cpp
    ./codec/nvcodec/NvEncoder/NvEncoderD3D11.cpp
    ./codec/qsvcodec/*.cpp
    ./codec/qsvcodec/src/*.cpp
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    file(GLOB exclude_src
        ./render/win/*.cpp
        ./codec/*.cpp
        ./codec/encoder/*.cpp
        ./codec/nvcodec/NvEncoder/NvEncoder.cpp
        ./codec/nvcodec/NvEncoder/NvEncoderD3D11.cpp
        ./codec/qsvcodec/*.cpp
        ./codec/qsvcodec/src/*.cpp
    )
    list(REMOVE_ITEM all_src ${exclude_src})
endif()

include_directories(
    ${KRTC_DIR}
    ${KRTC_THIRD_PARTY_DIR}/include
    ${WEBRTC_INCLUDE_DIR}
    ${WEBRTC_INCLUDE_DIR}/third_party/abseil-cpp
    ${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include
    ${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include/libyuv
    ${WEBRTC_INCLUDE_DIR}/third_party/jsoncpp/source/include
    ${KRTC_DIR}/krtc
	${KRTC_DIR}/krtc/codec
	${KRTC_DIR}/krtc/codec/encoder
    ${KRTC_DIR}/krtc/codec/nvcodec
	${KRTC_DIR}/krtc/codec/qsvcodec
	${KRTC_DIR}/krtc/codec/qsvcodec/include
)

link_directories(
    ${WEBRTC_LIB_DIR}
    ${KRTC_THIRD_PARTY_DIR}/lib
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DKRTC_API_EXPORT
        -DCURL_STATICLIB
        -DNOMINMAX
        -D_CRT_SECURE_NO_WARNINGS
        -DWEBRTC_WIN
        -DUNICODE
        -D_UNICODE
        -DWIN32_LEAN_AND_MEAN
        -DUSE_EXTERNAL_ENCOER
        -DRTC_ENABLE_WIN_WGC
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DKRTC_API_EXPORT
        -DWEBRTC_POSIX
        -DWEBRTC_LINUX
        -DUSE_GLIB=1)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_library(krtc SHARED ${all_src} "codec/common_encoder.h" "device/audio_device_data_observer.cpp" "device/audio_device_data_observer.h")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_library(krtc_static STATIC ${all_src})
    set_target_properties(krtc_static PROPERTIES  OUTPUT_NAME "krtc")
    set_target_properties(krtc_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

    add_library(krtc SHARED ${all_src})
    set_target_properties(krtc PROPERTIES  OUTPUT_NAME "krtc")
    set_target_properties(krtc PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    set_target_properties(krtc PROPERTIES VERSION 1.0 SOVERSION 1)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(krtc
        winmm
	    ws2_32
	    strmiids
	    wldap32
	    crypt32
	    iphlpapi
	    msdmo
	    dmoguids
	    wmcodecdspuuid
	    secur32
	    d3d11
        dxva2
	    dxgi
        Dwmapi
        ${WEBRTC_LIB_DIR}/webrtc.lib
        ${KRTC_THIRD_PARTY_DIR}/lib/libcurl.lib
        ${KRTC_THIRD_PARTY_DIR}/lib/libssh2.lib
        ${KRTC_THIRD_PARTY_DIR}/lib/zlibstatic.lib
        ${KRTC_THIRD_PARTY_DIR}/lib/jsoncpp_static.lib
        ${KRTC_THIRD_PARTY_DIR}/lib/libssl.lib
	    ${KRTC_THIRD_PARTY_DIR}/lib/libcrypto.lib 
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(krtc
        -lcurl
        -lwebrtc
        -ljsoncpp
        -lpthread 
        -ldl
	)
endif()
