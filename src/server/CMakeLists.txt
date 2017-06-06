CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

SET(PROJECT_SERVER_NAME ${PROJECT_NAME_LOWERCASE}_server)

SET(HEADERS_COMMANDS
  ${SOURCE_ROOT}/commands/commands.h
)

SET(SOURCES_COMMANDS
  ${SOURCE_ROOT}/commands/commands.cpp
)

SET(HEADERS_INNER
  ${SOURCE_ROOT}/inner/inner_server_command_seq_parser.h
  ${SOURCE_ROOT}/inner/inner_client.h
)

SET(SOURCES_INNER
  ${SOURCE_ROOT}/inner/inner_server_command_seq_parser.cpp
  ${SOURCE_ROOT}/inner/inner_client.cpp
)

SET(HEADERS_SERIALIZER
  ${SOURCE_ROOT}/serializer/iserializer.h
  ${SOURCE_ROOT}/serializer/json_serializer.h
)

SET(SOURCES_SERIALIZER
  ${SOURCE_ROOT}/serializer/iserializer.cpp
  ${SOURCE_ROOT}/serializer/json_serializer.cpp
)

SET(SOURCES_SDS
  ${SOURCE_ROOT}/third-party/sds/sds.c
)

SET(CLIENT_SERVER_SOURCES
  ${SOURCE_ROOT}/ping_info.h ${SOURCE_ROOT}/ping_info.cpp
  ${SOURCE_ROOT}/auth_info.h ${SOURCE_ROOT}/auth_info.cpp
  ${SOURCE_ROOT}/server_info.h ${SOURCE_ROOT}/server_info.cpp
  ${SOURCE_ROOT}/client_info.h ${SOURCE_ROOT}/client_info.cpp
  ${SOURCE_ROOT}/channels_info.h ${SOURCE_ROOT}/channels_info.cpp
  ${SOURCE_ROOT}/url.h ${SOURCE_ROOT}/url.cpp
  ${SOURCE_ROOT}/client_server_types.h ${SOURCE_ROOT}/client_server_types.cpp
)

IF(OS_WINDOWS)
  SET(PLATFORM_HDRS)
  SET(PLATFORM_SRCS)
  SET(PLATFORM_LIBRARIES ws2_32)
ELSEIF(OS_MACOSX)
  SET(PLATFORM_HDRS)
  SET(PLATFORM_SRCS)
  SET(PLATFORM_LIBRARIES)
ELSEIF(OS_LINUX)
  SET(PLATFORM_HDRS)
  SET(PLATFORM_SRCS)
  SET(PLATFORM_LIBRARIES)
ELSEIF(OS_FREEBSD)
  SET(PLATFORM_HDRS)
  SET(PLATFORM_SRCS)
  SET(PLATFORM_LIBRARIES)
ENDIF(OS_WINDOWS)

IF(USE_PTHREAD)
  SET(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} pthread)
ENDIF(USE_PTHREAD)

SET(HEADERS_REDIS
  redis/redis_connect.h
  redis/redis_storage.h
  redis/redis_config.h

  redis/redis_sub_config.h
  redis/redis_pub_sub.h
  redis/redis_pub_sub_handler.h
)

SET(SOURCES_REDIS
  redis/redis_connect.cpp
  redis/redis_storage.cpp
  redis/redis_config.cpp

  redis/redis_pub_sub.cpp
  redis/redis_pub_sub_handler.cpp
  redis/redis_sub_config.cpp
)

SET(HEADERS_INNER_SERVER
  commands.h
  inner/inner_tcp_server.h
  inner/inner_tcp_client.h
  inner/inner_tcp_handler.h
  inner/inner_external_notifier.h
)

SET(SOURCES_INNER_SERVER
  inner/inner_tcp_server.cpp
  inner/inner_tcp_client.cpp
  inner/inner_tcp_handler.cpp
  inner/inner_external_notifier.cpp
  commands.cpp
)

SET(BUILD_SERVER_SOURCES
  server_host.cpp server_host.h
  user_info.h user_info.cpp
  user_state_info.h user_state_info.cpp
  responce_info.h responce_info.cpp
  config.h config.cpp
  ${HEADERS_REDIS} ${SOURCES_REDIS}
  ${HEADERS_COMMANDS} ${SOURCES_COMMANDS}

  ${HEADERS_INNER} ${SOURCES_INNER}
  ${HEADERS_SERIALIZER} ${SOURCES_SERIALIZER}
  ${CLIENT_SERVER_SOURCES}

  ${HEADERS_INNER_SERVER} ${SOURCES_INNER_SERVER}
  ${HEADERS_PARSE_COMMANDS} ${SOURCES_PARSE_COMMANDS}
)
SET(SERVER_CONFIG_FILE_NAME "${PROJECT_SERVER_NAME}.conf")
SET(SERVER_CONFIG_FILE_PATH "/etc/${SERVER_CONFIG_FILE_NAME}")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/install/${PROJECT_NAME_LOWERCASE}/server/server_config.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${SERVER_CONFIG_FILE_NAME}" @ONLY IMMEDIATE)
ADD_DEFINITIONS(-DSERVER_CONFIG_FILE_PATH="${SERVER_CONFIG_FILE_PATH}")
ADD_DEFINITIONS(-DPROJECT_NAME_SERVER_TITLE="${PROJECT_SERVER_NAME}")
ADD_DEFINITIONS(-DPROJECT_NAME_SERVER="${PROJECT_NAME_LOWERCASE}")

FIND_PACKAGE(Common REQUIRED)

ADD_SUBDIRECTORY(${SOURCE_ROOT}/third-party/redis redis)

IF(NOT TARGET inih)
  ADD_SUBDIRECTORY(${SOURCE_ROOT}/third-party/ini inih)
ENDIF()

IF(NOT TARGET json-c)
  ADD_SUBDIRECTORY(${SOURCE_ROOT}/third-party/json-c json-c)
ENDIF()
SET(PRIVATE_INCLUDE_DIRECTORIES_SERVER
  ${SOURCE_ROOT}
  ${SOURCE_ROOT}/third-party/ini
  ${SOURCE_ROOT}/third-party/redis/deps
  ${SOURCE_ROOT}/third-party/sds
  ${COMMON_INCLUDE_DIR}
)

ADD_EXECUTABLE(${PROJECT_SERVER_NAME}
  main.cpp
  ${BUILD_SERVER_SOURCES} ${SOURCES_SDS}
)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_SERVER_NAME} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_SERVER})
TARGET_LINK_LIBRARIES(${PROJECT_SERVER_NAME}
  json-c inih hiredis ${COMMON_LIBRARIES} ${PLATFORM_LIBRARIES}
)

#prepare executable
IF(PROJECT_BUILD_TYPE_VERSION STREQUAL "release")
  STRIP_TARGET(${PROJECT_SERVER_NAME})
ENDIF(PROJECT_BUILD_TYPE_VERSION STREQUAL "release")

# Start to install
INSTALL(TARGETS ${PROJECT_SERVER_NAME} DESTINATION ${TARGET_INSTALL_DESTINATION} COMPONENT APPLICATIONS)
IF(NOT EXISTS ${SERVER_CONFIG_FILE_PATH})
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SERVER_CONFIG_FILE_NAME} DESTINATION /etc/)
ENDIF(NOT EXISTS ${SERVER_CONFIG_FILE_PATH})

IF (DEVELOPER_CHECK_STYLE)
  SET(CHECK_SOURCES_SERVER
    main.cpp ${BUILD_SERVER_SOURCES}
  )
  REGISTER_CHECK_STYLE_TARGET(check_style_server "${CHECK_SOURCES_SERVER}")
  REGISTER_CHECK_INCLUDES_TARGET(${PROJECT_SERVER_NAME})
ENDIF(DEVELOPER_CHECK_STYLE)

IF(DEVELOPER_ENABLE_TESTS)
  IF(DEVELOPER_ENABLE_UNIT_TESTS)
    SET(PRIVATE_INCLUDE_DIRECTORIES_SERVER_TEST
      ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${SOURCE_ROOT} ${COMMON_INCLUDE_DIR}
    )

    SET(PROJECT_UNIT_TEST_CLIENT unit_tests_server)
    ADD_EXECUTABLE(${PROJECT_UNIT_TEST_CLIENT}
      ${CMAKE_SOURCE_DIR}/tests/unit_tests/server/test_parse_commands.cpp commands.cpp
      ${SOURCE_ROOT}/commands/commands.cpp

      ${CMAKE_SOURCE_DIR}/tests/unit_tests/server/test_serializer.cpp
      user_info.cpp
      user_state_info.cpp
      responce_info.cpp
      ${SOURCE_ROOT}/channels_info.cpp
      ${SOURCE_ROOT}/auth_info.cpp
      ${SOURCE_ROOT}/url.cpp
    )
    TARGET_INCLUDE_DIRECTORIES(${PROJECT_UNIT_TEST_CLIENT} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES_SERVER_TEST})
    TARGET_LINK_LIBRARIES(${PROJECT_UNIT_TEST_CLIENT} gtest gtest_main ${COMMON_LIBRARIES} json-c)
    ADD_TEST_TARGET(${PROJECT_UNIT_TEST_CLIENT})
    SET_PROPERTY(TARGET ${PROJECT_UNIT_TEST_CLIENT} PROPERTY FOLDER "Unit tests")
  ENDIF(DEVELOPER_ENABLE_UNIT_TESTS)
ENDIF(DEVELOPER_ENABLE_TESTS)
