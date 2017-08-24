SET(PLAYER_LIBRARY player)

ADD_SUBDIRECTORY(core)

SET(PRIVATE_INCLUDE_DIRECTORIES_PLAYER
  ${CMAKE_CURRENT_BINARY_DIR}/core  # for ffmpeg_config.h
  ${SOURCE_ROOT}
  ${FFMPEG_INCLUDE_DIR}
  ${COMMON_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
  ${DEPENDENS_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

SET(BUILD_PLAYER_SOURCES
  ${SOURCE_ROOT}/client/player/cmdutils.h
  ${SOURCE_ROOT}/client/player/cmdutils.cpp
  ${SOURCE_ROOT}/client/player/utils.h
  ${SOURCE_ROOT}/client/player/utils.cpp
  ${SOURCE_ROOT}/client/player/sdl_utils.h
  ${SOURCE_ROOT}/client/player/sdl_utils.cpp
  ${SOURCE_ROOT}/client/player/av_sdl_utils.h
  ${SOURCE_ROOT}/client/player/av_sdl_utils.cpp

  ${SOURCE_ROOT}/client/player/ffmpeg_application.h
  ${SOURCE_ROOT}/client/player/ffmpeg_application.cpp
  ${SOURCE_ROOT}/client/player/config.h
  ${SOURCE_ROOT}/client/player/config.cpp
  ${SOURCE_ROOT}/client/player/player_options.h
  ${SOURCE_ROOT}/client/player/player_options.cpp
  ${SOURCE_ROOT}/client/player/isimple_player.h
  ${SOURCE_ROOT}/client/player/isimple_player.cpp
  ${SOURCE_ROOT}/client/player/stream_handler.h
  ${SOURCE_ROOT}/client/player/stream_handler.cpp
)

ADD_LIBRARY(${PLAYER_LIBRARY} OBJECT ${BUILD_PLAYER_SOURCES})
TARGET_INCLUDE_DIRECTORIES(${PLAYER_LIBRARY} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_PLAYER})

IF(DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_CLIENT_PLAYER_CORE
    ${BUILD_PLAYER_SOURCES}
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_${PLAYER_LIBRARY} "${CHECK_SOURCES_CLIENT_PLAYER_CORE}")
  REGISTER_CHECK_INCLUDES_TARGET(${PLAYER_LIBRARY})
ENDIF(DEVELOPER_CHECK_STYLE)
