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


SET(HEADERS_WIDGETS
  ${SOURCE_ROOT}/client/player/gui/widgets/window.h
  ${SOURCE_ROOT}/client/player/gui/widgets/font_window.h
  ${SOURCE_ROOT}/client/player/gui/widgets/label.h
  ${SOURCE_ROOT}/client/player/gui/widgets/line_edit.h
  ${SOURCE_ROOT}/client/player/gui/widgets/button.h
  ${SOURCE_ROOT}/client/player/gui/widgets/icon_label.h
  ${SOURCE_ROOT}/client/player/gui/widgets/list_box.h
)

SET(SOURCES_WIDGETS
  ${SOURCE_ROOT}/client/player/gui/widgets/window.cpp
  ${SOURCE_ROOT}/client/player/gui/widgets/font_window.cpp
  ${SOURCE_ROOT}/client/player/gui/widgets/label.cpp
  ${SOURCE_ROOT}/client/player/gui/widgets/line_edit.cpp
  ${SOURCE_ROOT}/client/player/gui/widgets/button.cpp
  ${SOURCE_ROOT}/client/player/gui/widgets/icon_label.cpp
  ${SOURCE_ROOT}/client/player/gui/widgets/list_box.cpp
)

SET(BUILD_GUI_SOURCES
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
  ${HEADERS_WIDGETS}
  ${SOURCES_WIDGETS}
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
