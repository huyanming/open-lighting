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
 * RDMCommandTest.cpp
 * Test fixture for the RDMCommand classes
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>
#include <iomanip>
#include <memory>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"

using ola::io::IOQueue;
using ola::network::HostToNetwork;
using ola::rdm::GuessMessageType;
using ola::rdm::RDMCommand;
using ola::rdm::RDMDiscoveryRequest;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::RDMSetResponse;
using ola::rdm::UID;
using ola::testing::ASSERT_DATA_EQUALS;
using std::auto_ptr;
using std::string;

class RDMCommandTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMCommandTest);
  CPPUNIT_TEST(testRDMCommand);
  CPPUNIT_TEST(testOutputStream);
  CPPUNIT_TEST(testRequestInflation);
  CPPUNIT_TEST(testResponseInflation);
  CPPUNIT_TEST(testNackWithReason);
  CPPUNIT_TEST(testGetResponseFromData);
  CPPUNIT_TEST(testCombineResponses);
  CPPUNIT_TEST(testPackWithParams);
  CPPUNIT_TEST(testGuessMessageType);
  CPPUNIT_TEST(testDiscoveryCommand);
  CPPUNIT_TEST(testUnMuteRequest);
  CPPUNIT_TEST(testMuteCommand);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testRDMCommand();
    void testOutputStream();
    void testRequestInflation();
    void testResponseInflation();
    void testNackWithReason();
    void testGetResponseFromData();
    void testCombineResponses();
    void testPackWithParams();
    void testGuessMessageType();
    void testDiscoveryCommand();
    void testUnMuteRequest();
    void testMuteCommand();

  private:
    void PackAndVerify(const RDMCommand &command,
                       const uint8_t *expected,
                       unsigned int expected_length);
    void UpdateChecksum(uint8_t *expected,
                        unsigned int expected_length);

    bool VerifyMatches(
        const uint8_t *data1,
        unsigned int data1_length,
        const uint8_t *data2,
        unsigned int datq2_length);

    static uint8_t EXPECTED_GET_BUFFER[];
    static uint8_t EXPECTED_SET_BUFFER[];
    static uint8_t EXPECTED_GET_RESPONSE_BUFFER[];
    static uint8_t EXPECTED_DISCOVERY_REQUEST[];
    static uint8_t EXPECTED_MUTE_REQUEST[];
    static uint8_t EXPECTED_UNMUTE_REQUEST[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMCommandTest);


uint8_t RDMCommandTest::EXPECTED_GET_BUFFER[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x20, 1, 40, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};

uint8_t RDMCommandTest::EXPECTED_SET_BUFFER[] = {
  1, 28,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x30, 1, 40, 4,  // command, param id, param data length
  0xa5, 0xa5, 0xa5, 0xa5,  // param data
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_GET_RESPONSE_BUFFER[] = {
  1, 28,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x21, 1, 40, 4,  // command, param id, param data length
  0x5a, 0x5a, 0x5a, 0x5a,  // param data
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_DISCOVERY_REQUEST[] = {
  1, 36,  // sub code & length
  255, 255, 255, 255, 255, 255,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 1, 12,  // command, param id, param data length
  1, 2, 0, 0, 3, 4,  // lower uid
  5, 6, 0, 0, 7, 8,  // upper uid
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_MUTE_REQUEST[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 2, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_UNMUTE_REQUEST[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 3, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};


/*
 * Fill in the checksums
 */
void RDMCommandTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  UpdateChecksum(EXPECTED_GET_BUFFER, sizeof(EXPECTED_GET_BUFFER));
  UpdateChecksum(EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  UpdateChecksum(EXPECTED_GET_RESPONSE_BUFFER,
                 sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  UpdateChecksum(EXPECTED_DISCOVERY_REQUEST,
                 sizeof(EXPECTED_DISCOVERY_REQUEST));
  UpdateChecksum(EXPECTED_MUTE_REQUEST,
                 sizeof(EXPECTED_MUTE_REQUEST));
  UpdateChecksum(EXPECTED_UNMUTE_REQUEST,
                 sizeof(EXPECTED_UNMUTE_REQUEST));
}


/**
 * Verify two memory locations match.
 */
bool RDMCommandTest::VerifyMatches(
    const uint8_t *data1,
    unsigned int data1_length,
    const uint8_t *data2,
    unsigned int data2_length) {
  bool matches = data1_length == data2_length;
  if (!matches) {
    OLA_INFO << "Size " << data1_length << " != " << data2_length;
    return matches;
  }

  matches = !memcmp(data1, data2, data1_length);
  if (!matches) {
    for (unsigned int i = 0; i < data1_length; i++)
      OLA_INFO << std::hex << static_cast<int>(data1[i]) << " " <<
        static_cast<int>(data2[i]);
  }
  return matches;
}


/*
 * Test the RDMCommands work.
 */
void RDMCommandTest::testRDMCommand() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest command(source,
                        destination,
                        0,  // transaction #
                        1,  // port id
                        0,  // message count
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length

  CPPUNIT_ASSERT_EQUAL(source, command.SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, command.DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, command.TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, command.PortId());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, command.MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, command.SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND, command.CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, command.ParamId());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t*>(NULL), command.ParamData());
  CPPUNIT_ASSERT_EQUAL(0u, command.ParamDataSize());
  CPPUNIT_ASSERT_EQUAL(25u, command.Size());
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, command.CommandType());
  PackAndVerify(command, EXPECTED_GET_BUFFER, sizeof(EXPECTED_GET_BUFFER));

  // try one with extra long data
  uint8_t *data = new uint8_t[232];
  RDMGetRequest long_command(source,
                             destination,
                             0,  // transaction #
                             1,  // port id
                             0,  // message count
                             10,  // sub device
                             123,  // param id
                             data,  // data
                             232);  // data length

  CPPUNIT_ASSERT_EQUAL(231u, long_command.ParamDataSize());
  CPPUNIT_ASSERT_EQUAL(256u, long_command.Size());

  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command3(source,
                         destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // message count
                         10,  // sub device
                         296,  // param id
                         reinterpret_cast<uint8_t*>(&data_value),  // data
                         sizeof(data_value));  // data length

  CPPUNIT_ASSERT_EQUAL(29u, command3.Size());
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, command.CommandType());
  PackAndVerify(command3, EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  delete[] data;
}


/*
 * Test write to an output stream works.
 */
void RDMCommandTest::testOutputStream() {
  IOQueue output;
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest command(source,
                        destination,
                        0,  // transaction #
                        1,  // port id
                        0,  // message count
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length
  command.Write(&output);
  CPPUNIT_ASSERT_EQUAL(command.Size(), output.Size());

  uint8_t *raw_command = new uint8_t[output.Size()];
  unsigned int raw_command_size = output.Peek(raw_command, output.Size());
  CPPUNIT_ASSERT_EQUAL(raw_command_size, command.Size());

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_GET_BUFFER,
                     sizeof(EXPECTED_GET_BUFFER),
                     raw_command,
                     raw_command_size);
  output.Pop(raw_command_size);
  CPPUNIT_ASSERT_EQUAL(0u, output.Size());
  delete[] raw_command;

  // now try a command with data
  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command2(source,
                         destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // message count
                         10,  // sub device
                         296,  // param id
                         reinterpret_cast<uint8_t*>(&data_value),  // data
                         sizeof(data_value));  // data length

  CPPUNIT_ASSERT_EQUAL(29u, command2.Size());

  command2.Write(&output);
  CPPUNIT_ASSERT_EQUAL(command2.Size(), output.Size());

  raw_command = new uint8_t[output.Size()];
  raw_command_size = output.Peek(raw_command, output.Size());
  CPPUNIT_ASSERT_EQUAL(raw_command_size, command2.Size());

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_SET_BUFFER,
                     sizeof(EXPECTED_SET_BUFFER),
                     raw_command,
                     raw_command_size);
  output.Pop(raw_command_size);
  CPPUNIT_ASSERT_EQUAL(0u, output.Size());
  delete[] raw_command;
}



/*
 * Test that we can inflate RDM request messages correctly
 */
void RDMCommandTest::testRequestInflation() {
  UID source(1, 2);
  UID destination(3, 4);
  RDMRequest *command = RDMRequest::InflateFromData(NULL, 10);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMRequest*>(NULL), command);

  string empty_string;
  command = RDMRequest::InflateFromData(empty_string);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMRequest*>(NULL), command);

  // now try a proper command but with no length
  command = RDMRequest::InflateFromData(EXPECTED_GET_BUFFER, 0);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMRequest*>(NULL), command);

  command = RDMRequest::InflateFromData(
      EXPECTED_GET_BUFFER,
      sizeof(EXPECTED_GET_BUFFER));
  CPPUNIT_ASSERT(NULL != command);

  RDMGetRequest expected_command(source,
                                 destination,
                                 0,  // transaction #
                                 1,  // port id
                                 0,  // message count
                                 10,  // sub device
                                 296,  // param id
                                 NULL,  // data
                                 0);  // data length
  CPPUNIT_ASSERT(expected_command == *command);
  delete command;

  string get_request_str(reinterpret_cast<char*>(EXPECTED_GET_BUFFER),
                         sizeof(EXPECTED_GET_BUFFER));
  command = RDMRequest::InflateFromData(get_request_str);
  CPPUNIT_ASSERT(NULL != command);
  CPPUNIT_ASSERT(expected_command == *command);
  delete command;

  // now try a set request
  command = RDMRequest::InflateFromData(
      EXPECTED_SET_BUFFER,
      sizeof(EXPECTED_SET_BUFFER));
  CPPUNIT_ASSERT(NULL != command);
  uint8_t expected_data[] = {0xa5, 0xa5, 0xa5, 0xa5};
  CPPUNIT_ASSERT_EQUAL(4u, command->ParamDataSize());
  CPPUNIT_ASSERT(0 == memcmp(expected_data, command->ParamData(),
                             command->ParamDataSize()));
  delete command;

  // set request as a string
  string set_request_string(reinterpret_cast<char*>(EXPECTED_SET_BUFFER),
                            sizeof(EXPECTED_SET_BUFFER));
  command = RDMRequest::InflateFromData(set_request_string);
  CPPUNIT_ASSERT(NULL != command);
  CPPUNIT_ASSERT_EQUAL(4u, command->ParamDataSize());
  CPPUNIT_ASSERT(0 == memcmp(expected_data, command->ParamData(),
                             command->ParamDataSize()));
  delete command;

  // change the param length and make sure the checksum fails
  uint8_t *bad_packet = new uint8_t[sizeof(EXPECTED_GET_BUFFER)];
  memcpy(bad_packet, EXPECTED_GET_BUFFER, sizeof(EXPECTED_GET_BUFFER));
  bad_packet[22] = 255;

  command = RDMRequest::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_BUFFER));
  CPPUNIT_ASSERT(NULL == command);

  get_request_str[22] = 255;
  command = RDMRequest::InflateFromData(get_request_str);
  CPPUNIT_ASSERT(NULL == command);

  // now make sure we can't pass a bad param length larger than the buffer
  UpdateChecksum(bad_packet, sizeof(EXPECTED_GET_BUFFER));
  command = RDMRequest::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_BUFFER));
  CPPUNIT_ASSERT(NULL == command);
  delete[] bad_packet;

  // change the param length of another packet and make sure the checksum fails
  bad_packet = new uint8_t[sizeof(EXPECTED_SET_BUFFER)];
  memcpy(bad_packet, EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  bad_packet[22] = 5;
  UpdateChecksum(bad_packet, sizeof(EXPECTED_SET_BUFFER));
  command = RDMRequest::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_SET_BUFFER));
  CPPUNIT_ASSERT(NULL == command);
  delete[] bad_packet;

  // now try to inflate a response
  command = RDMRequest::InflateFromData(
      EXPECTED_GET_RESPONSE_BUFFER,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMRequest*>(NULL), command);

  string response_string(reinterpret_cast<char*>(EXPECTED_GET_RESPONSE_BUFFER),
                         sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command = RDMRequest::InflateFromData(response_string);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMRequest*>(NULL), command);
}


/*
 * Calculate the checksum for a packet, and verify
 */
void RDMCommandTest::PackAndVerify(const RDMCommand &command,
                                   const uint8_t *expected,
                                   unsigned int expected_length) {
  // now check packing
  unsigned int buffer_size = command.Size();
  uint8_t *buffer = new uint8_t[buffer_size];
  CPPUNIT_ASSERT(command.Pack(buffer, &buffer_size));

  for (unsigned int i = 0 ; i < expected_length; i++) {
    std::stringstream str;
    str << "Offset " << i << ", expected " << static_cast<int>(expected[i]) <<
      ", got " << static_cast<int>(buffer[i]);
    CPPUNIT_ASSERT_MESSAGE(str.str(), buffer[i] == expected[i]);
  }
  delete[] buffer;

  // now check string packing
  string str_buffer;
  CPPUNIT_ASSERT(command.Pack(&str_buffer));
  CPPUNIT_ASSERT_EQUAL((size_t) expected_length, str_buffer.size());

  for (unsigned int i = 0 ; i < expected_length; i++) {
    std::stringstream str;
    uint8_t c = str_buffer[i];
    str << "Offset " << i << ", expected " << std::hex <<
      static_cast<int>(expected[i]) << ", got " << std::hex <<
      static_cast<int>(c);
    CPPUNIT_ASSERT_MESSAGE(str.str(), c == expected[i]);
  }
}


/*
 * Test that we can inflate RDM response correctly
 */
void RDMCommandTest::testResponseInflation() {
  UID source(1, 2);
  UID destination(3, 4);
  ola::rdm::rdm_response_code code;
  RDMResponse *command = RDMResponse::InflateFromData(NULL, 10, &code);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMResponse*>(NULL), command);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_INVALID_RESPONSE, code);

  string empty_string;
  command = RDMResponse::InflateFromData(empty_string, &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_PACKET_TOO_SHORT, code);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMResponse*>(NULL), command);

  command = RDMResponse::InflateFromData(EXPECTED_GET_RESPONSE_BUFFER, 0,
                                         &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_PACKET_TOO_SHORT, code);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMResponse*>(NULL), command);

  command = RDMResponse::InflateFromData(
      EXPECTED_GET_RESPONSE_BUFFER,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  CPPUNIT_ASSERT(NULL != command);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_COMPLETED_OK, code);
  uint8_t expected_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  CPPUNIT_ASSERT_EQUAL(4u, command->ParamDataSize());
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_RESPONSE, command->CommandType());
  CPPUNIT_ASSERT(0 == memcmp(expected_data, command->ParamData(),
                             command->ParamDataSize()));

  uint32_t data_value = 0x5a5a5a5a;
  RDMGetResponse expected_command(source,
                                  destination,
                                  0,  // transaction #
                                  1,  // port id
                                  0,  // message count
                                  10,  // sub device
                                  296,  // param id
                                  reinterpret_cast<uint8_t*>(&data_value),
                                  sizeof(data_value));  // data length
  CPPUNIT_ASSERT(expected_command == *command);
  delete command;

  // now try from a string
  string response_string(reinterpret_cast<char*>(EXPECTED_GET_RESPONSE_BUFFER),
                         sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command = RDMResponse::InflateFromData(response_string, &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_COMPLETED_OK, code);
  CPPUNIT_ASSERT(NULL != command);
  CPPUNIT_ASSERT_EQUAL(4u, command->ParamDataSize());
  CPPUNIT_ASSERT(0 == memcmp(expected_data, command->ParamData(),
                             command->ParamDataSize()));
  CPPUNIT_ASSERT(expected_command == *command);

  // change the param length and make sure the checksum fails
  uint8_t *bad_packet = new uint8_t[sizeof(EXPECTED_GET_RESPONSE_BUFFER)];
  memcpy(bad_packet, EXPECTED_GET_RESPONSE_BUFFER,
         sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  bad_packet[22] = 255;
  delete command;

  command = RDMResponse::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_CHECKSUM_INCORRECT, code);
  CPPUNIT_ASSERT(NULL == command);

  response_string[22] = 255;
  command = RDMResponse::InflateFromData(response_string, &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_CHECKSUM_INCORRECT, code);
  CPPUNIT_ASSERT(NULL == command);

  // now make sure we can't pass a bad param length larger than the buffer
  UpdateChecksum(bad_packet, sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command = RDMResponse::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_PARAM_LENGTH_MISMATCH, code);
  CPPUNIT_ASSERT(NULL == command);
  delete[] bad_packet;

  // change the param length of another packet and make sure the checksum fails
  bad_packet = new uint8_t[sizeof(EXPECTED_SET_BUFFER)];
  memcpy(bad_packet, EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  bad_packet[22] = 5;
  UpdateChecksum(bad_packet, sizeof(EXPECTED_SET_BUFFER));
  command = RDMResponse::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_SET_BUFFER),
      &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_PARAM_LENGTH_MISMATCH, code);
  CPPUNIT_ASSERT(NULL == command);
  delete[] bad_packet;

  // now try to inflate a request
  command = RDMResponse::InflateFromData(
      EXPECTED_GET_BUFFER,
      sizeof(EXPECTED_GET_BUFFER),
      &code);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMResponse*>(NULL), command);

  string request_string(reinterpret_cast<char*>(EXPECTED_GET_BUFFER),
                        sizeof(EXPECTED_GET_BUFFER));
  command = RDMResponse::InflateFromData(request_string, &code);
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_INVALID_COMMAND_CLASS, code);
  CPPUNIT_ASSERT_EQUAL(static_cast<RDMResponse*>(NULL), command);
}


/*
 * Calculate a checksum for a packet and update it
 */
void RDMCommandTest::UpdateChecksum(uint8_t *expected,
                                    unsigned int expected_length) {
  unsigned int checksum = RDMCommand::START_CODE;
  for (unsigned int i = 0 ; i < expected_length - 2; i++)
    checksum += expected[i];

  expected[expected_length - 2] = checksum >> 8;
  expected[expected_length - 1] = checksum & 0xff;
}


/**
 * Test that we can build nack messages
 */
void RDMCommandTest::testNackWithReason() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  RDMResponse *response = NackWithReason(&get_command,
                                         ola::rdm::NR_UNKNOWN_PID);
  uint16_t reason = ola::rdm::NR_UNKNOWN_PID;
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(destination, response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(source, response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) ola::rdm::RDM_NACK_REASON,
                       response->ResponseType());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE,
                       response->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, response->ParamId());
  CPPUNIT_ASSERT_EQUAL(HostToNetwork(reason),
                       *(reinterpret_cast<uint16_t*>(response->ParamData())));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(reason),
                       response->ParamDataSize());
  delete response;

  response = NackWithReason(&get_command,
                            ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  reason = ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE;
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(destination, response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(source, response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) ola::rdm::RDM_NACK_REASON,
                       response->ResponseType());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE,
                       response->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, response->ParamId());
  CPPUNIT_ASSERT_EQUAL(HostToNetwork(reason),
                       *(reinterpret_cast<uint16_t*>(response->ParamData())));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(reason),
                       response->ParamDataSize());
  delete response;

  RDMSetRequest set_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  response = NackWithReason(&set_command,
                            ola::rdm::NR_WRITE_PROTECT);
  reason = ola::rdm::NR_WRITE_PROTECT;
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(destination, response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(source, response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) ola::rdm::RDM_NACK_REASON,
                       response->ResponseType());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::SET_COMMAND_RESPONSE,
                       response->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, response->ParamId());
  CPPUNIT_ASSERT_EQUAL(HostToNetwork(reason),
                       *(reinterpret_cast<uint16_t*>(response->ParamData())));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(reason),
                       response->ParamDataSize());
  delete response;
}


void RDMCommandTest::testGetResponseFromData() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  RDMResponse *response = GetResponseFromData(&get_command, NULL, 0);
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(destination, response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(source, response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) ola::rdm::RDM_ACK, response->ResponseType());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE,
                       response->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, response->ParamId());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t*>(NULL),
                       response->ParamData());
  CPPUNIT_ASSERT_EQUAL(0u, response->ParamDataSize());
  delete response;

  RDMSetRequest set_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  response = GetResponseFromData(&set_command, NULL, 0);
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(destination, response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(source, response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) ola::rdm::RDM_ACK, response->ResponseType());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::SET_COMMAND_RESPONSE,
                       response->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, response->ParamId());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t*>(NULL),
                       response->ParamData());
  CPPUNIT_ASSERT_EQUAL(0u, response->ParamDataSize());
  delete response;

  uint32_t data_value = 0xa5a5a5a5;
  response = GetResponseFromData(
      &get_command,
      reinterpret_cast<const uint8_t*>(&data_value),
      sizeof(data_value));
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(destination, response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(source, response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) ola::rdm::RDM_ACK, response->ResponseType());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE,
                       response->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, response->ParamId());
  CPPUNIT_ASSERT_EQUAL(data_value,
                       *(reinterpret_cast<uint32_t*>(response->ParamData())));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(data_value),
                       response->ParamDataSize());
  delete response;
}


/**
 * Check that CombineResponses() works.
 */
void RDMCommandTest::testCombineResponses() {
  UID source(1, 2);
  UID destination(3, 4);
  uint16_t param_id = 296;

  uint32_t data_value = 0x5a5a5a5a;
  RDMGetResponse response1(source,
                           destination,
                           0,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           reinterpret_cast<uint8_t*>(&data_value),
                           sizeof(data_value));  // data length

  uint32_t data_value2 = 0xa5a5a5a5;
  RDMGetResponse response2(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           reinterpret_cast<uint8_t*>(&data_value2),
                           sizeof(data_value2));  // data length

  const RDMResponse *combined_response = RDMResponse::CombineResponses(
      &response1,
      &response2);
  CPPUNIT_ASSERT(combined_response);
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE,
                       combined_response->CommandClass());
  CPPUNIT_ASSERT_EQUAL(source, combined_response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, combined_response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, combined_response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, combined_response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, combined_response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(param_id, combined_response->ParamId());
  CPPUNIT_ASSERT_EQUAL(8u, combined_response->ParamDataSize());
  const uint8_t *combined_data = combined_response->ParamData();
  const uint32_t expected_data[] = {0x5a5a5a5a, 0xa5a5a5a5};
  CPPUNIT_ASSERT(0 == memcmp(expected_data,
                             combined_data,
                             sizeof(expected_data)));
  delete combined_response;


  // try to combine with a response with no data
  RDMGetResponse response3(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           NULL,
                           0);

  combined_response = RDMResponse::CombineResponses(
      &response1,
      &response3);
  CPPUNIT_ASSERT(combined_response);
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE,
                       combined_response->CommandClass());
  CPPUNIT_ASSERT_EQUAL(source, combined_response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, combined_response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, combined_response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, combined_response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, combined_response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(param_id, combined_response->ParamId());
  CPPUNIT_ASSERT_EQUAL(4u, combined_response->ParamDataSize());
  combined_data = combined_response->ParamData();
  CPPUNIT_ASSERT_EQUAL(data_value,
                       *(reinterpret_cast<const uint32_t*>(combined_data)));
  delete combined_response;

  // combining a GetResponse with a SetResponse is invalid
  RDMSetResponse response4(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           NULL,
                           0);
  combined_response = RDMResponse::CombineResponses(
      &response1,
      &response4);
  CPPUNIT_ASSERT(!combined_response);
  combined_response = RDMResponse::CombineResponses(
      &response4,
      &response1);
  CPPUNIT_ASSERT(!combined_response);

  // combine two set responses
  RDMSetResponse response5(source,
                           destination,
                           0,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           reinterpret_cast<uint8_t*>(&data_value),
                           sizeof(data_value));  // data length


  combined_response = RDMResponse::CombineResponses(
      &response5,
      &response4);
  CPPUNIT_ASSERT(combined_response);
  CPPUNIT_ASSERT_EQUAL(RDMCommand::SET_COMMAND_RESPONSE,
                       combined_response->CommandClass());
  CPPUNIT_ASSERT_EQUAL(source, combined_response->SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, combined_response->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, combined_response->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, combined_response->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, combined_response->SubDevice());
  CPPUNIT_ASSERT_EQUAL(param_id, combined_response->ParamId());
  CPPUNIT_ASSERT_EQUAL(4u, combined_response->ParamDataSize());
  combined_data = combined_response->ParamData();
  CPPUNIT_ASSERT_EQUAL(data_value,
                       *(reinterpret_cast<const uint32_t*>(combined_data)));
  delete combined_response;
}


/**
 * Test that cloning works
 */
void RDMCommandTest::testPackWithParams() {
  UID source(1, 2);
  UID destination(3, 4);
  UID new_source(7, 8);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  uint8_t *data = new uint8_t[get_command.Size()];
  unsigned int length = get_command.Size();
  CPPUNIT_ASSERT(get_command.PackWithControllerParams(
      data, &length, new_source, 99, 10));

  RDMRequest *command = RDMRequest::InflateFromData(data, length);
  CPPUNIT_ASSERT(command);

  CPPUNIT_ASSERT_EQUAL(new_source, command->SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, command->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 99, command->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, command->PortId());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, command->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, command->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND, command->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, command->ParamId());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t*>(NULL), command->ParamData());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, command->ParamDataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 25, command->Size());
  delete[] data;
  delete command;

  string packed_request;
  CPPUNIT_ASSERT(get_command.PackWithControllerParams(
        &packed_request, new_source, 99, 10));

  command = RDMRequest::InflateFromData(packed_request);
  CPPUNIT_ASSERT(command);
  CPPUNIT_ASSERT_EQUAL(new_source, command->SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, command->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 99, command->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, command->PortId());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, command->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, command->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND, command->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, command->ParamId());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t*>(NULL), command->ParamData());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, command->ParamDataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 25, command->Size());
  delete command;
}


/**
 * Check that GuessMessageType works
 */
void RDMCommandTest::testGuessMessageType() {
  ola::rdm::rdm_message_type message_type;
  RDMCommand::RDMCommandClass command_class;
  CPPUNIT_ASSERT(!GuessMessageType(&message_type, &command_class, NULL, 0));
  CPPUNIT_ASSERT(!GuessMessageType(&message_type, &command_class, NULL, 20));
  CPPUNIT_ASSERT(!GuessMessageType(
        &message_type, &command_class, EXPECTED_GET_BUFFER, 0));
  CPPUNIT_ASSERT(!GuessMessageType(
        &message_type, &command_class, EXPECTED_GET_BUFFER, 18));
  CPPUNIT_ASSERT(!GuessMessageType(
        &message_type, &command_class, EXPECTED_GET_BUFFER, 19));

  CPPUNIT_ASSERT(GuessMessageType(
        &message_type,
        &command_class,
        EXPECTED_GET_BUFFER,
        sizeof(EXPECTED_GET_BUFFER)));
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, message_type);
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND, command_class);

  CPPUNIT_ASSERT(GuessMessageType(
        &message_type,
        &command_class,
        EXPECTED_SET_BUFFER,
        sizeof(EXPECTED_SET_BUFFER)));
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, message_type);
  CPPUNIT_ASSERT_EQUAL(RDMCommand::SET_COMMAND, command_class);

  CPPUNIT_ASSERT(GuessMessageType(
        &message_type,
        &command_class,
        EXPECTED_GET_RESPONSE_BUFFER,
        sizeof(EXPECTED_GET_RESPONSE_BUFFER)));
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_RESPONSE, message_type);
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND_RESPONSE, command_class);
}


/**
 * Check the discovery command
 */
void RDMCommandTest::testDiscoveryCommand() {
  UID source(1, 2);
  UID lower(0x0102, 0x0304);
  UID upper(0x0506, 0x0708);

  auto_ptr<RDMDiscoveryRequest> request(
      NewDiscoveryUniqueBranchRequest(source, lower, upper, 1));

  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, request->CommandType());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  CPPUNIT_ASSERT_EQUAL(37u, request->Size());
  unsigned int length = request->Size();

  uint8_t *data = new uint8_t[length];
  CPPUNIT_ASSERT(request->Pack(data, &length));

  bool matches = VerifyMatches(
      EXPECTED_DISCOVERY_REQUEST,
      sizeof(EXPECTED_DISCOVERY_REQUEST),
      data,
      length);
  CPPUNIT_ASSERT(matches);
  delete[] data;
}


/**
 * Check the mute command
 */
void RDMCommandTest::testMuteCommand() {
  UID source(1, 2);
  UID destination(3, 4);
  auto_ptr<RDMDiscoveryRequest> request(
      NewMuteRequest(source, destination, 1));

  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, request->CommandType());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  CPPUNIT_ASSERT_EQUAL(25u, request->Size());
  unsigned int length = request->Size();
  uint8_t *data = new uint8_t[length];
  CPPUNIT_ASSERT(request->Pack(data, &length));

  bool matches = VerifyMatches(
      EXPECTED_MUTE_REQUEST,
      sizeof(EXPECTED_MUTE_REQUEST),
      data,
      length);
  CPPUNIT_ASSERT(matches);
  delete[] data;
}


/**
 * Check the UnMute Command
 */
void RDMCommandTest::testUnMuteRequest() {
  UID source(1, 2);
  UID destination(3, 4);
  auto_ptr<RDMDiscoveryRequest> request(
      NewUnMuteRequest(source, destination, 1));

  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, request->CommandType());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  CPPUNIT_ASSERT_EQUAL(25u, request->Size());
  unsigned int length = request->Size();
  uint8_t *data = new uint8_t[length];
  CPPUNIT_ASSERT(request->Pack(data, &length));

  bool matches = VerifyMatches(
      EXPECTED_UNMUTE_REQUEST,
      sizeof(EXPECTED_UNMUTE_REQUEST),
      data,
      length);
  CPPUNIT_ASSERT(matches);
  delete[] data;
}
