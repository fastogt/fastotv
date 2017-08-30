CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

SET(GUI_LIBRARY gui)

SET(PRIVATE_INCLUDE_DIRECTORIES_GUI
  ${SOURCE_ROOT}
  ${COMMON_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
)

SET(PRIVATE_INCLUDE_DIRECTORIES_GUI
  ${SOURCE_ROOT}
  ${COMMON_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
)

SET(HEADERS_EVENTS
  ${SOURCE_ROOT}/client/player/gui/events/key_events.h
  ${SOURCE_ROOT}/client/player/gui/events/mouse_events.h
  ${SOURCE_ROOT}/client/player/gui/events/window_events.h
  ${SOURCE_ROOT}/client/player/gui/events/events.h
)

SET(SOURCES_EVENTS
  ${SOURCE_ROOT}/client/player/gui/events/key_events.cpp
  ${SOURCE_ROOT}/client/player/gui/events/mouse_events.cpp
  ${SOURCE_ROOT}/client/player/gui/events/window_events.cpp
  ${SOURCE_ROOT}/client/player/gui/events/events.cpp
)


SET(BUILD_GUI_SOURCES
  ${SOURCE_ROOT}/client/player/gui/window.h
  ${SOURCE_ROOT}/client/player/gui/window.cpp
  ${SOURCE_ROOT}/client/player/gui/label.h
  ${SOURCE_ROOT}/client/player/gui/label.cpp
  ${SOURCE_ROOT}/client/player/gui/keypress_label.h
  ${SOURCE_ROOT}/client/player/gui/keypress_label.cpp
  ${SOURCE_ROOT}/client/player/gui/icon_label.h
  ${SOURCE_ROOT}/client/player/gui/icon_label.cpp

  ${SOURCE_ROOT}/client/player/gui/events_base.h
  ${SOURCE_ROOT}/client/player/gui/events_base.cpp
  ${SOURCE_ROOT}/client/player/gui/stream_events.h
  ${SOURCE_ROOT}/client/player/gui/stream_events.cpp
  ${SOURCE_ROOT}/client/player/gui/lirc_events.h
  ${SOURCE_ROOT}/client/player/gui/lirc_events.cpp
  ${SOURCE_ROOT}/client/player/gui/sdl2_application.h
  ${SOURCE_ROOT}/client/player/gui/sdl2_application.cpp
  ${SOURCE_ROOT}/client/player/gui/network_events.h
  ${SOURCE_ROOT}/client/player/gui/network_events.cpp

  ${HEADERS_EVENTS}
  ${SOURCES_EVENTS}
)

ADD_LIBRARY(${GUI_LIBRARY} OBJECT ${BUILD_GUI_SOURCES})
TARGET_INCLUDE_DIRECTORIES(${GUI_LIBRARY} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_GUI})

IF(DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_CLIENT_PLAYER_GUI
    ${BUILD_GUI_SOURCES}
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_${GUI_LIBRARY} "${CHECK_SOURCES_CLIENT_PLAYER_GUI}")
  REGISTER_CHECK_INCLUDES_TARGET(${GUI_LIBRARY})
ENDIF(DEVELOPER_CHECK_STYLE)
