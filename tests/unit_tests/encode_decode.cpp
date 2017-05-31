#include <gtest/gtest.h>

#include "client_server_types.h"

TEST(ClientServer, EncodeDecode) {
  const std::string data = "alex_ #%v reer ll";

  std::string encoded = fasto::fastotv::Encode(data);
  ASSERT_TRUE(!encoded.empty());
  std::string decoded = fasto::fastotv::Decode(encoded);

  ASSERT_EQ(data, decoded);
}
