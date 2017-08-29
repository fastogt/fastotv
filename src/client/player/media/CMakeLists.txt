CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

SET(MEDIA_LIBRARY media)

SET(HEADERS_CORE_FRAMES
  ${SOURCE_ROOT}/client/player/media/frames/base_frame.h
  ${SOURCE_ROOT}/client/player/media/frames/audio_frame.h
  ${SOURCE_ROOT}/client/player/media/frames/video_frame.h
  ${SOURCE_ROOT}/client/player/media/frames/ring_buffer.h
  ${SOURCE_ROOT}/client/player/media/frames/frame_queue.h
)

SET(HEADERS_CORE
  ${SOURCE_ROOT}/client/player/media/types.h
  ${SOURCE_ROOT}/client/player/media/stream_statistic.h
  ${SOURCE_ROOT}/client/player/media/av_utils.h
  ${SOURCE_ROOT}/client/player/media/sdl_utils.h
  ${SOURCE_ROOT}/client/player/media/clock.h
  ${SOURCE_ROOT}/client/player/media/packet_queue.h
  ${SOURCE_ROOT}/client/player/media/decoder.h
  ${SOURCE_ROOT}/client/player/media/app_options.h
  ${SOURCE_ROOT}/client/player/media/audio_params.h
  ${SOURCE_ROOT}/client/player/media/stream.h
  ${SOURCE_ROOT}/client/player/media/video_state.h
  ${SOURCE_ROOT}/client/player/media/video_state_handler.h
  ${SOURCE_ROOT}/client/player/media/bandwidth_estimation.h

  ${HEADERS_CORE_FRAMES}
)

SET(SOURCES_CORE_FRAMES
  ${SOURCE_ROOT}/client/player/media/frames/base_frame.cpp
  ${SOURCE_ROOT}/client/player/media/frames/audio_frame.cpp
  ${SOURCE_ROOT}/client/player/media/frames/video_frame.cpp
  ${SOURCE_ROOT}/client/player/media/frames/frame_queue.cpp
  ${SOURCE_ROOT}/client/player/media/frames/ring_buffer.cpp
)

SET(SOURCES_CORE
  ${SOURCE_ROOT}/client/player/media/types.cpp
  ${SOURCE_ROOT}/client/player/media/stream_statistic.cpp
  ${SOURCE_ROOT}/client/player/media/av_utils.cpp
  ${SOURCE_ROOT}/client/player/media/sdl_utils.cpp
  ${SOURCE_ROOT}/client/player/media/clock.cpp
  ${SOURCE_ROOT}/client/player/media/packet_queue.cpp
  ${SOURCE_ROOT}/client/player/media/decoder.cpp
  ${SOURCE_ROOT}/client/player/media/app_options.cpp
  ${SOURCE_ROOT}/client/player/media/audio_params.cpp
  ${SOURCE_ROOT}/client/player/media/stream.cpp
  ${SOURCE_ROOT}/client/player/media/video_state.cpp
  ${SOURCE_ROOT}/client/player/media/video_state_handler.cpp
  ${SOURCE_ROOT}/client/player/media/bandwidth_estimation.cpp

  ${SOURCES_CORE_FRAMES}
)

CONFIGURE_FILE(ffmpeg_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg_config.h @ONLY IMMEDIATE)

# core
SET(BUILD_CLIENT_CORE_SOURCES
  ${HEADERS_CORE} ${SOURCES_CORE}
)

SET(PRIVATE_INCLUDE_DIRECTORIES_CORE
 ${CMAKE_CURRENT_BINARY_DIR}  # for ffmpeg_config.h
 ${SOURCE_ROOT}
 ${FFMPEG_INCLUDE_DIR}
 ${COMMON_INCLUDE_DIR}
 ${SDL2_INCLUDE_DIRS}
 ${DEPENDENS_INCLUDE_DIRS}
 ${OPENSSL_INCLUDE_DIR}
)

IF(VDPAU_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/media/hwaccels/ffmpeg_vdpau.cpp)
ENDIF(VDPAU_FOUND)
IF(VAAPI_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/media/hwaccels/ffmpeg_vaapi.cpp)
ENDIF(VAAPI_FOUND)
IF(DXVA2_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/media/hwaccels/ffmpeg_dxva2.cpp)
ENDIF(DXVA2_FOUND)
IF(CUDA_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/media/hwaccels/ffmpeg_cuvid.cpp)
ENDIF(CUDA_FOUND)
IF(VIDEOTOOLBOX_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/media/hwaccels/ffmpeg_videotoolbox.cpp)
ENDIF(VIDEOTOOLBOX_FOUND)

ADD_LIBRARY(${MEDIA_LIBRARY} OBJECT ${BUILD_CLIENT_CORE_SOURCES}
 ${SOURCE_ROOT}/client/player/media/ffmpeg_internal.h ${SOURCE_ROOT}/client/player/media/ffmpeg_internal.cpp
)
TARGET_INCLUDE_DIRECTORIES(${MEDIA_LIBRARY} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_CORE})

IF(DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_CLIENT_CORE
    ${BUILD_CLIENT_CORE_SOURCES}
    ${SOURCE_ROOT}/client/player/media/ffmpeg_internal.h
    ${SOURCE_ROOT}/client/player/media/ffmpeg_internal.cpp
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_${MEDIA_LIBRARY} "${CHECK_SOURCES_CLIENT_CORE}")
  REGISTER_CHECK_INCLUDES_TARGET(${MEDIA_LIBRARY})
ENDIF(DEVELOPER_CHECK_STYLE)

IF(DEVELOPER_ENABLE_TESTS)
ENDIF(DEVELOPER_ENABLE_TESTS)
