SET(SOURCE_ROOT ${CMAKE_SOURCE_DIR}/src)

ADD_DEFINITIONS(-DPROJECT_TEST_SOURCES_DIR="${CMAKE_SOURCE_DIR}/tests")

# network
SET(PROJECT_NETWORK_LIBRARY ${PROJECT_NAME_LOWERCASE}_network)

IF(BUILD_CLIENT)  # build client
  ADD_SUBDIRECTORY(client)
ENDIF(BUILD_CLIENT)

IF(BUILD_SERVER)  # build server
  ADD_SUBDIRECTORY(server)
ENDIF(BUILD_SERVER)

IF(DEVELOPER_ENABLE_TESTS)
  IF(DEVELOPER_ENABLE_UNIT_TESTS)
  ########## PREPARE GTEST LIBRARY ##########
    ADD_SUBDIRECTORY(${CMAKE_HOME_DIRECTORY}/tests/googletest googletest)
    INCLUDE_DIRECTORIES(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${SOURCE_ROOT})
  ########## PREPARE GTEST LIBRARY ##########
    ADD_EXECUTABLE(unit_tests
      ${CMAKE_SOURCE_DIR}/tests/unit_tests/test_parse_commands.cpp commands/commands.cpp client/commands.cpp
    )

    TARGET_LINK_LIBRARIES(unit_tests gtest gtest_main common)
    ADD_TEST_TARGET(unit_tests)
    SET_PROPERTY(TARGET unit_tests PROPERTY FOLDER "Unit tests")

    #Mock tests
    #ADD_EXECUTABLE(mock_tests
      #${CMAKE_SOURCE_DIR}/tests/mock_tests/test_connections.cpp
    #)
    #TARGET_LINK_LIBRARIES(mock_tests gmock gmock_main ${PROJECT_CORE_LIBRARY} common)
    #ADD_TEST_TARGET(mock_tests)
    #SET_PROPERTY(TARGET mock_tests PROPERTY FOLDER "Mock tests")
  ENDIF(DEVELOPER_ENABLE_UNIT_TESTS)
ENDIF(DEVELOPER_ENABLE_TESTS)
