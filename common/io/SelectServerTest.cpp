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
 * SelectServerTest.cpp
 * Test fixture for the Socket classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <sstream>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"

using ola::ExportMap;
using ola::IntegerVariable;
using ola::io::LoopbackDescriptor;
using ola::io::SelectServer;
using ola::network::UdpSocket;

class SelectServerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SelectServerTest);
  CPPUNIT_TEST(testAddRemoveReadDescriptor);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST(testLoopCallbacks);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testAddRemoveReadDescriptor();
    void testTimeout();
    void testLoopCallbacks();

    void FatalTimeout() {
      CPPUNIT_ASSERT(false);
    }

    void TerminateTimeout() {
      if (m_ss) { m_ss->Terminate(); }
    }

    void SingleIncrementTimeout() {
      m_timeout_counter++;
    }

    void ReentrantTimeout(SelectServer *ss) {
      ss->RegisterSingleTimeout(
          0,
          ola::NewSingleCallback(this,
                                 &SelectServerTest::SingleIncrementTimeout));
      ss->RegisterSingleTimeout(
          5,
          ola::NewSingleCallback(this,
                                 &SelectServerTest::SingleIncrementTimeout));
    }

    bool IncrementTimeout() {
      if (m_ss && m_ss->IsRunning())
        m_timeout_counter++;
      return true;
    }

    void IncrementLoopCounter() { m_loop_counter++; }

  private:
    unsigned int m_timeout_counter;
    unsigned int m_loop_counter;
    ExportMap *m_map;
    SelectServer *m_ss;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SelectServerTest);


void SelectServerTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_map = new ExportMap();
  m_ss = new SelectServer(m_map);
  m_timeout_counter = 0;
  m_loop_counter = 0;
}


void SelectServerTest::tearDown() {
  delete m_ss;
  delete m_map;
}


/*
 * Check AddReadDescriptor/RemoveReadDescriptor works correctly and that the
 * export map is updated.
 */
void SelectServerTest::testAddRemoveReadDescriptor() {
  LoopbackDescriptor bad_socket;
  IntegerVariable *connected_socket_count =
    m_map->GetIntegerVar(SelectServer::K_CONNECTED_DESCRIPTORS_VAR);
  IntegerVariable *socket_count =
    m_map->GetIntegerVar(SelectServer::K_READ_DESCRIPTOR_VAR);
  CPPUNIT_ASSERT_EQUAL(0, connected_socket_count->Get());
  CPPUNIT_ASSERT_EQUAL(0, socket_count->Get());
  // adding and removin a non-connected socket should fail
  CPPUNIT_ASSERT(!m_ss->AddReadDescriptor(&bad_socket));
  CPPUNIT_ASSERT(!m_ss->RemoveReadDescriptor(&bad_socket));

  LoopbackDescriptor loopback_socket;
  loopback_socket.Init();
  CPPUNIT_ASSERT_EQUAL(0, connected_socket_count->Get());
  CPPUNIT_ASSERT_EQUAL(0, socket_count->Get());
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&loopback_socket));
  // Adding a second time should fail
  CPPUNIT_ASSERT(!m_ss->AddReadDescriptor(&loopback_socket));
  CPPUNIT_ASSERT_EQUAL(1, connected_socket_count->Get());
  CPPUNIT_ASSERT_EQUAL(0, socket_count->Get());

  // Add a udp socket
  UdpSocket udp_socket;
  CPPUNIT_ASSERT(udp_socket.Init());
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&udp_socket));
  CPPUNIT_ASSERT(!m_ss->AddReadDescriptor(&udp_socket));
  CPPUNIT_ASSERT_EQUAL(1, connected_socket_count->Get());
  CPPUNIT_ASSERT_EQUAL(1, socket_count->Get());

  // Check remove works
  CPPUNIT_ASSERT(m_ss->RemoveReadDescriptor(&loopback_socket));
  CPPUNIT_ASSERT_EQUAL(0, connected_socket_count->Get());
  CPPUNIT_ASSERT_EQUAL(1, socket_count->Get());
  CPPUNIT_ASSERT(m_ss->RemoveReadDescriptor(&udp_socket));
  CPPUNIT_ASSERT_EQUAL(0, connected_socket_count->Get());
  CPPUNIT_ASSERT_EQUAL(0, socket_count->Get());

  // Remove again should fail
  CPPUNIT_ASSERT(!m_ss->RemoveReadDescriptor(&loopback_socket));
}


/*
 * Timeout tests
 */
void SelectServerTest::testTimeout() {
  // check a single timeout
  m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::SingleIncrementTimeout));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, m_timeout_counter);

  // now check a timeout that adds another timeout
  m_timeout_counter = 0;

  m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::ReentrantTimeout, m_ss));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, m_timeout_counter);

  // check repeating timeouts
  // Some systems (VMs in particular) can't do 10ms resolution so we go for
  // larger numbers here.
  m_timeout_counter = 0;
  m_ss->RegisterRepeatingTimeout(
      100,
      ola::NewCallback(this, &SelectServerTest::IncrementTimeout));
  m_ss->RegisterSingleTimeout(
      980,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  // This seems to go as low as 7
  std::stringstream str;
  str << "Timeout counter was " << m_timeout_counter;
  CPPUNIT_ASSERT_MESSAGE(str.str(),
                         m_timeout_counter >= 5 && m_timeout_counter <= 9);

  // check timeouts are removed correctly
  ola::thread::timeout_id timeout1 = m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::FatalTimeout));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->RemoveTimeout(timeout1);
  m_ss->Run();
}


/*
 * Check that the loop closures are called
 */
void SelectServerTest::testLoopCallbacks() {
  m_ss->SetDefaultInterval(ola::TimeInterval(0, 100000));  // poll every 100ms
  m_ss->RunInLoop(ola::NewCallback(this,
                                   &SelectServerTest::IncrementLoopCounter));
  m_ss->RegisterSingleTimeout(
      500,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  // we should have at least 5 calls to IncrementLoopCounter
  CPPUNIT_ASSERT(m_loop_counter >= 5);
}
