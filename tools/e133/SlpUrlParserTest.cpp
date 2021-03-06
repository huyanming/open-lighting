/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SlpUrlParserTest.cpp
 * Test fixture for the SlpUrlParser class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/UID.h>
#include <string>

#include "tools/e133/SlpUrlParser.h"

using std::string;
using ola::network::IPV4Address;


class SlpUrlParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SlpUrlParserTest);
  CPPUNIT_TEST(testParseUrl);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

    void testParseUrl();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SlpUrlParserTest);


/*
 * Check that ParseSlpUrl works.
 */
void SlpUrlParserTest::testParseUrl() {
  ola::rdm::UID uid(0, 0);
  ola::rdm::UID expected_uid(0x7a70, 1);

  IPV4Address address;
  IPV4Address expected_ip;
  IPV4Address::FromString("192.168.1.204", &expected_ip);

  CPPUNIT_ASSERT(!ParseSlpUrl("", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("foo", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e133", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e133.esta", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e131.esta", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e133.esta:", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e131.esta://", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e133.esta://", &uid, &address));
  CPPUNIT_ASSERT(!ParseSlpUrl("service:e131.esta://10.0.0.1", &uid, &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta:10.0.0.1:5568", &uid, &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://foobar:5568/7a7000000001",
                   &uid,
                   &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://192.168.1.204:5568:7a7000000001",
                   &uid,
                   &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://192.168.1.204:5568", &uid, &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://192.168.1.204:5555/7a7000000001",
                   &uid,
                   &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://192.168.1.204:5568/7g7000000",
                   &uid,
                   &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://192.168.1.204:5568/7g7000000001",
                   &uid,
                   &address));
  CPPUNIT_ASSERT(
      !ParseSlpUrl("service:e133.esta://192.168.1.204:5568/7a7000000g01",
                   &uid,
                   &address));

  // finally the working case
  CPPUNIT_ASSERT(
      ParseSlpUrl("service:e133.esta://192.168.1.204:5568/7a7000000001",
                  &uid,
                  &address));
  CPPUNIT_ASSERT_EQUAL(expected_uid, uid);
  CPPUNIT_ASSERT_EQUAL(expected_ip, address);

  CPPUNIT_ASSERT(
      ParseSlpUrl("service:e133.esta://10.0.80.43:5568/4a6100000020",
                  &uid,
                  &address));
  ola::rdm::UID expected_uid2(19041, 32);
  IPV4Address::FromString("10.0.80.43", &expected_ip);
  CPPUNIT_ASSERT_EQUAL(expected_uid2, uid);
  CPPUNIT_ASSERT_EQUAL(expected_ip, address);
}
