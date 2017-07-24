SET(SOURCE_ROOT ${CMAKE_SOURCE_DIR}/src)

SET(SERVICE_HOST_NAME fastotv.com CACHE STRING "Service host name")
SET(SERVICE_HOST_PORT 7040 CACHE STRING "Service port")

# User specific
SET(USER_LOGIN anon@fastogt.com CACHE STRING "User login")
SET(USER_PASSWORD 2ae66f90b7788ab8950e8f81b829c947 CACHE STRING "User password for ${USER_LOGIN}")
SET(USER_DEVICE_ID 5971d32fc976287338c015c0 CACHE STRING "User device for ${USER_LOGIN}")

ADD_DEFINITIONS(-DUSER_LOGIN="${USER_LOGIN}"
  -DUSER_PASSWORD="${USER_PASSWORD}"
  -DUSER_DEVICE_ID="${USER_DEVICE_ID}"

  -DSERVICE_HOST_NAME="${SERVICE_HOST_NAME}"
  -DSERVICE_HOST_PORT=${SERVICE_HOST_PORT}
  -DUNKNOWN_ICON_URI="http://fastotv.com/images/unknown_channel.png"
)

IF(DEVELOPER_ENABLE_TESTS)
  IF(DEVELOPER_ENABLE_UNIT_TESTS)
    FIND_PACKAGE(GTest REQUIRED)
    ADD_DEFINITIONS(-DPROJECT_TEST_SOURCES_DIR="${CMAKE_SOURCE_DIR}/tests")
  ENDIF(DEVELOPER_ENABLE_UNIT_TESTS)
ENDIF(DEVELOPER_ENABLE_TESTS)

#module
SET(PROJECT_CLIENT_SERVER_LIBRARY ${PROJECT_NAME_LOWERCASE}_client_server)

SET(HEADERS_COMMANDS
  commands/commands.h
)

SET(SOURCES_COMMANDS
  commands/commands.cpp
)

SET(HEADERS_INNER
  inner/inner_server_command_seq_parser.h
  inner/inner_client.h
)

SET(SOURCES_INNER
  inner/inner_server_command_seq_parser.cpp
  inner/inner_client.cpp
)

SET(HEADERS_SERIALIZER
  serializer/iserializer.h
  serializer/json_serializer.h
)

SET(SOURCES_SERIALIZER
  serializer/iserializer.cpp
  serializer/json_serializer.cpp
)

SET(SOURCES_SDS
  third-party/sds/sds.c
)

SET(CLIENT_SERVER_SOURCES
  ping_info.h ping_info.cpp
  channels_info.h channels_info.cpp
  auth_info.h auth_info.cpp
  server_info.h server_info.cpp
  client_info.h client_info.cpp
  channel_info.h channel_info.cpp
  epg_info.h epg_info.cpp
  programme_info.h programme_info.cpp
  client_server_types.h client_server_types.cpp
) # server and client common sources

SET(CLIENT_SERVER_SOURCES
  ${HEADERS_COMMANDS} ${SOURCES_COMMANDS}
  ${HEADERS_INNER} ${SOURCES_INNER}
  ${HEADERS_SERIALIZER} ${SOURCES_SERIALIZER}
  ${CLIENT_SERVER_SOURCES}
)

SET(PRIVATE_INCLUDE_DIRECTORIES_CLIENT_SERVER
  ${SOURCE_ROOT}
  ${SOURCE_ROOT}/third-party/sds
)
ADD_LIBRARY(${PROJECT_CLIENT_SERVER_LIBRARY} STATIC ${CLIENT_SERVER_SOURCES} ${SOURCES_SDS})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_CLIENT_SERVER_LIBRARY} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_CLIENT_SERVER})

IF(BUILD_CLIENT)  # build client
  ADD_SUBDIRECTORY(client)
ENDIF(BUILD_CLIENT)

IF(BUILD_SERVER)  # build server
  ADD_SUBDIRECTORY(server)
ENDIF(BUILD_SERVER)

IF (DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_CLIENT_SERVER
    ${CLIENT_SERVER_SOURCES}
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_client_server "${CHECK_SOURCES_CLIENT_SERVER}")
  REGISTER_CHECK_INCLUDES_TARGET(${PROJECT_CLIENT_SERVER_LIBRARY})
ENDIF(DEVELOPER_CHECK_STYLE)

IF(DEVELOPER_ENABLE_TESTS)
  IF(DEVELOPER_ENABLE_UNIT_TESTS)
    FIND_PACKAGE(Common REQUIRED)
    SET(PROJECT_UNIT_TEST unit_tests)
    SET(PRIVATE_INCLUDE_DIRECTORIES_TEST
      ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${SOURCE_ROOT} ${COMMON_INCLUDE_DIR}
    )
    ADD_EXECUTABLE(${PROJECT_UNIT_TEST}
      ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_serializer.cpp
      ${CMAKE_SOURCE_DIR}/tests/unit_tests/encode_decode.cpp
    )
    TARGET_INCLUDE_DIRECTORIES(${PROJECT_UNIT_TEST} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_TEST})
    TARGET_LINK_LIBRARIES(${PROJECT_UNIT_TEST}
      gtest gtest_main
      ${PROJECT_CLIENT_SERVER_LIBRARY} ${COMMON_LIBRARIES} json-c
    )
    ADD_TEST_TARGET(${PROJECT_UNIT_TEST})
    SET_PROPERTY(TARGET ${PROJECT_UNIT_TEST} PROPERTY FOLDER "Unit tests")

    #Mock tests
    #ADD_EXECUTABLE(mock_tests
      #${CMAKE_SOURCE_DIR}/tests/mock_tests/test_connections.cpp
    #)
    #TARGET_LINK_LIBRARIES(mock_tests gmock gmock_main ${PROJECT_CORE_LIBRARY} common)
    #ADD_TEST_TARGET(mock_tests)
    #SET_PROPERTY(TARGET mock_tests PROPERTY FOLDER "Mock tests")
  ENDIF(DEVELOPER_ENABLE_UNIT_TESTS)
ENDIF(DEVELOPER_ENABLE_TESTS)
