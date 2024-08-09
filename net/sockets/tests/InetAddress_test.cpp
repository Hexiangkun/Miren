#include "net/sockets/InetAddress.h"
#include "base/log/Logging.h"

#include <gtest/gtest.h>

using std::string;
using Miren::net::InetAddress;

TEST(inetAddress, testInetAddress)
{
  InetAddress addr0(1234);
  EXPECT_EQ(addr0.toIP(), std::string("0.0.0.0"));
  EXPECT_EQ(addr0.toIpPort(), std::string("0.0.0.0:1234"));
  EXPECT_EQ(addr0.toPort(), 1234);

  InetAddress addr1(4321, true);
  EXPECT_EQ(addr1.toIP(), std::string("127.0.0.1"));
  EXPECT_EQ(addr1.toIpPort(), std::string("127.0.0.1:4321"));
  EXPECT_EQ(addr1.toPort(), 4321);

  InetAddress addr2("1.2.3.4", 8888);
  EXPECT_EQ(addr2.toIP(), std::string("1.2.3.4"));
  EXPECT_EQ(addr2.toIpPort(), std::string("1.2.3.4:8888"));
  EXPECT_EQ(addr2.toPort(), 8888);

  InetAddress addr3("255.254.253.252", 65535);
  EXPECT_EQ(addr3.toIP(), std::string("255.254.253.252"));
  EXPECT_EQ(addr3.toIpPort(), std::string("255.254.253.252:65535"));
  EXPECT_EQ(addr3.toPort(), 65535);
}

TEST(inetAddress, testInetAddressResolve)
{
  InetAddress addr(80);
  if (InetAddress::resolve("google.com", &addr))
  {
    LOG_INFO << "google.com resolved to " << addr.toIpPort();
  }
  else
  {
    LOG_ERROR << "Unable to resolve google.com";
  }
}
