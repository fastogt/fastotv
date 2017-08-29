CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

SET(DRAW_LIBRARY events)

SET(PRIVATE_INCLUDE_DIRECTORIES_DRAW
  ${SOURCE_ROOT}
  ${COMMON_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
)

SET(HEADERS_CORE_EVENTS
  ${SOURCE_ROOT}/client/player/events/events_base.h
  ${SOURCE_ROOT}/client/player/events/key_events.h
  ${SOURCE_ROOT}/client/player/events/mouse_events.h
  ${SOURCE_ROOT}/client/player/events/window_events.h
  ${SOURCE_ROOT}/client/player/events/stream_events.h
  ${SOURCE_ROOT}/client/player/events/events.h
  ${SOURCE_ROOT}/client/player/events/network_events.h
  ${SOURCE_ROOT}/client/player/events/lirc_events.h
)

SET(SOURCES_CORE_EVENTS
  ${SOURCE_ROOT}/client/player/events/events_base.cpp
  ${SOURCE_ROOT}/client/player/events/key_events.cpp
  ${SOURCE_ROOT}/client/player/events/mouse_events.cpp
  ${SOURCE_ROOT}/client/player/events/window_events.cpp
  ${SOURCE_ROOT}/client/player/events/stream_events.cpp
  ${SOURCE_ROOT}/client/player/events/events.cpp
  ${SOURCE_ROOT}/client/player/events/network_events.cpp
  ${SOURCE_ROOT}/client/player/events/lirc_events.cpp
)

ADD_LIBRARY(${DRAW_LIBRARY} OBJECT ${HEADERS_CORE_EVENTS} ${SOURCES_CORE_EVENTS})
TARGET_INCLUDE_DIRECTORIES(${DRAW_LIBRARY} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_DRAW})

IF(DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_CLIENT_PLAYER_DRAW
    ${BUILD_DRAW_SOURCES}
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_${DRAW_LIBRARY} "${CHECK_SOURCES_CLIENT_PLAYER_DRAW}")
  REGISTER_CHECK_INCLUDES_TARGET(${DRAW_LIBRARY})
ENDIF(DEVELOPER_CHECK_STYLE)
