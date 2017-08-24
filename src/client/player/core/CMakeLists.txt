CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

SET(CORE_LIBRARY player_core)

SET(HEADERS_CORE_EVENTS
  ${SOURCE_ROOT}/client/player/core/events/events_base.h
  ${SOURCE_ROOT}/client/player/core/events/key_events.h
  ${SOURCE_ROOT}/client/player/core/events/mouse_events.h
  ${SOURCE_ROOT}/client/player/core/events/window_events.h
  ${SOURCE_ROOT}/client/player/core/events/stream_events.h
  ${SOURCE_ROOT}/client/player/core/events/events.h
  ${SOURCE_ROOT}/client/player/core/events/network_events.h
  ${SOURCE_ROOT}/client/player/core/events/lirc_events.h
)

SET(HEADERS_CORE_FRAMES
  ${SOURCE_ROOT}/client/player/core/frames/base_frame.h
  ${SOURCE_ROOT}/client/player/core/frames/audio_frame.h
  ${SOURCE_ROOT}/client/player/core/frames/video_frame.h
  ${SOURCE_ROOT}/client/player/core/frames/ring_buffer.h
  ${SOURCE_ROOT}/client/player/core/frames/frame_queue.h
)

SET(HEADERS_CORE
  ${SOURCE_ROOT}/client/player/core/types.h
  ${SOURCE_ROOT}/client/player/core/stream_statistic.h
  ${SOURCE_ROOT}/client/player/core/av_utils.h
  ${SOURCE_ROOT}/client/player/core/sdl_utils.h
  ${SOURCE_ROOT}/client/player/core/clock.h
  ${SOURCE_ROOT}/client/player/core/packet_queue.h
  ${SOURCE_ROOT}/client/player/core/decoder.h
  ${SOURCE_ROOT}/client/player/core/app_options.h
  ${SOURCE_ROOT}/client/player/core/audio_params.h
  ${SOURCE_ROOT}/client/player/core/stream.h
  ${SOURCE_ROOT}/client/player/core/application/sdl2_application.h
  ${SOURCE_ROOT}/client/player/core/video_state.h
  ${SOURCE_ROOT}/client/player/core/video_state_handler.h
  ${SOURCE_ROOT}/client/player/core/bandwidth_estimation.h

  ${HEADERS_CORE_EVENTS}
  ${HEADERS_CORE_FRAMES}
)

SET(SOURCES_CORE_EVENTS
  ${SOURCE_ROOT}/client/player/core/events/events_base.cpp
  ${SOURCE_ROOT}/client/player/core/events/key_events.cpp
  ${SOURCE_ROOT}/client/player/core/events/mouse_events.cpp
  ${SOURCE_ROOT}/client/player/core/events/window_events.cpp
  ${SOURCE_ROOT}/client/player/core/events/stream_events.cpp
  ${SOURCE_ROOT}/client/player/core/events/events.cpp
  ${SOURCE_ROOT}/client/player/core/events/network_events.cpp
  ${SOURCE_ROOT}/client/player/core/events/lirc_events.cpp
)

SET(SOURCES_CORE_FRAMES
  ${SOURCE_ROOT}/client/player/core/frames/base_frame.cpp
  ${SOURCE_ROOT}/client/player/core/frames/audio_frame.cpp
  ${SOURCE_ROOT}/client/player/core/frames/video_frame.cpp
  ${SOURCE_ROOT}/client/player/core/frames/frame_queue.cpp
  ${SOURCE_ROOT}/client/player/core/frames/ring_buffer.cpp
)

SET(SOURCES_CORE
  ${SOURCE_ROOT}/client/player/core/types.cpp
  ${SOURCE_ROOT}/client/player/core/stream_statistic.cpp
  ${SOURCE_ROOT}/client/player/core/av_utils.cpp
  ${SOURCE_ROOT}/client/player/core/sdl_utils.cpp
  ${SOURCE_ROOT}/client/player/core/clock.cpp
  ${SOURCE_ROOT}/client/player/core/packet_queue.cpp
  ${SOURCE_ROOT}/client/player/core/decoder.cpp
  ${SOURCE_ROOT}/client/player/core/app_options.cpp
  ${SOURCE_ROOT}/client/player/core/audio_params.cpp
  ${SOURCE_ROOT}/client/player/core/stream.cpp
  ${SOURCE_ROOT}/client/player/core/application/sdl2_application.cpp
  ${SOURCE_ROOT}/client/player/core/video_state.cpp
  ${SOURCE_ROOT}/client/player/core/video_state_handler.cpp
  ${SOURCE_ROOT}/client/player/core/bandwidth_estimation.cpp

  ${SOURCES_CORE_EVENTS}
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
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/core/hwaccels/ffmpeg_vdpau.cpp)
ENDIF(VDPAU_FOUND)
IF(VAAPI_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/core/hwaccels/ffmpeg_vaapi.cpp)
ENDIF(VAAPI_FOUND)
IF(DXVA2_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/core/hwaccels/ffmpeg_dxva2.cpp)
ENDIF(DXVA2_FOUND)
IF(CUDA_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/core/hwaccels/ffmpeg_cuvid.cpp)
ENDIF(CUDA_FOUND)
IF(VIDEOTOOLBOX_FOUND)
  SET(BUILD_CLIENT_CORE_SOURCES ${BUILD_CLIENT_CORE_SOURCES} ${SOURCE_ROOT}/client/player/core/hwaccels/ffmpeg_videotoolbox.cpp)
ENDIF(VIDEOTOOLBOX_FOUND)

ADD_LIBRARY(${CORE_LIBRARY} OBJECT ${BUILD_CLIENT_CORE_SOURCES}
 ${SOURCE_ROOT}/client/player/core/ffmpeg_internal.h ${SOURCE_ROOT}/client/player/core/ffmpeg_internal.cpp
)
TARGET_INCLUDE_DIRECTORIES(${CORE_LIBRARY} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_CORE})

IF(DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_CLIENT_CORE
    ${BUILD_CLIENT_CORE_SOURCES}
    ${SOURCE_ROOT}/client/player/core/ffmpeg_internal.h
    ${SOURCE_ROOT}/client/player/core/ffmpeg_internal.cpp
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_${CORE_LIBRARY} "${CHECK_SOURCES_CLIENT_CORE}")
  REGISTER_CHECK_INCLUDES_TARGET(${CORE_LIBRARY})
ENDIF(DEVELOPER_CHECK_STYLE)

IF(DEVELOPER_ENABLE_TESTS)
  SET(PROJECT_VIDEO_PERFORMANCE_TEST video_performance_test)
  ADD_EXECUTABLE(${PROJECT_VIDEO_PERFORMANCE_TEST}
    ${CMAKE_SOURCE_DIR}/tests/video_performance_test.cpp
    $<TARGET_OBJECTS:${CORE_LIBRARY}>
  )
  TARGET_INCLUDE_DIRECTORIES(${PROJECT_VIDEO_PERFORMANCE_TEST} PRIVATE
    ${SOURCE_ROOT}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${COMMON_INCLUDE_DIR}
  )
  TARGET_LINK_LIBRARIES(${PROJECT_VIDEO_PERFORMANCE_TEST}
    ${FFMPEG_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${DEPENDENS_LIBRARIES}
    ${COMMON_LIBRARIES}
    ${PLATFORM_LIBRARIES}
    ${BZIP2_LIBRARIES}
    ${ZLIB_LIBRARY}
    ${OPENSSL_LIBRARIES}
  )
ENDIF(DEVELOPER_ENABLE_TESTS)
