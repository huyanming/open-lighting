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
 * PidStoreTest.cpp
 * Test fixture for the PidStore & Pid classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "common/rdm/PidStoreLoader.h"
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/messaging/Descriptor.h"
#include "ola/messaging/SchemaPrinter.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMEnums.h"


using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::rdm::PidDescriptor;
using ola::rdm::PidStore;
using ola::rdm::PidStoreLoader;
using ola::rdm::RootPidStore;
using std::auto_ptr;
using std::endl;
using std::string;
using std::vector;


class PidStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PidStoreTest);
  CPPUNIT_TEST(testPidDescriptor);
  CPPUNIT_TEST(testPidStore);
  CPPUNIT_TEST(testPidStoreLoad);
  CPPUNIT_TEST(testPidStoreFileLoad);
  CPPUNIT_TEST(testPidStoreLoadMissingFile);
  CPPUNIT_TEST(testPidStoreLoadDuplicateManufacturer);
  CPPUNIT_TEST(testPidStoreLoadDuplicateValue);
  CPPUNIT_TEST(testPidStoreLoadDuplicateName);
  CPPUNIT_TEST(testPidStoreLoadInvalidEstaPid);
  CPPUNIT_TEST(testInconsistentData);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPidDescriptor();
    void testPidStore();
    void testPidStoreLoad();
    void testPidStoreFileLoad();
    void testPidStoreLoadMissingFile();
    void testPidStoreLoadDuplicateManufacturer();
    void testPidStoreLoadDuplicateValue();
    void testPidStoreLoadDuplicateName();
    void testPidStoreLoadInvalidEstaPid();
    void testInconsistentData();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }
    void tearDown() {}

  private:
};


CPPUNIT_TEST_SUITE_REGISTRATION(PidStoreTest);


/*
 * Test that the PidDescriptor works.
 */
void PidStoreTest::testPidDescriptor() {
  // just use empty fields for now
  vector<const class FieldDescriptor*> fields;
  const Descriptor *get_request_descriptor = new Descriptor("GET Request",
                                                            fields);
  const Descriptor *get_response_descriptor = new Descriptor("GET Response",
                                                             fields);
  const Descriptor *set_request_descriptor = new Descriptor("SET Request",
                                                            fields);
  const Descriptor *set_response_descriptor = new Descriptor("SET Response",
                                                             fields);

  PidDescriptor pid("foo",
                    10,
                    get_request_descriptor,
                    get_response_descriptor,
                    set_request_descriptor,
                    set_response_descriptor,
                    PidDescriptor::NON_BROADCAST_SUB_DEVICE,
                    PidDescriptor::ANY_SUB_DEVICE);

  // basic checks
  CPPUNIT_ASSERT_EQUAL(string("foo"), pid.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(10), pid.Value());
  CPPUNIT_ASSERT_EQUAL(get_request_descriptor, pid.GetRequest());
  CPPUNIT_ASSERT_EQUAL(get_response_descriptor, pid.GetResponse());
  CPPUNIT_ASSERT_EQUAL(set_request_descriptor, pid.SetRequest());
  CPPUNIT_ASSERT_EQUAL(set_response_descriptor, pid.SetResponse());

  // check sub device constraints
  CPPUNIT_ASSERT(pid.IsGetValid(0));
  CPPUNIT_ASSERT(pid.IsGetValid(1));
  CPPUNIT_ASSERT(pid.IsGetValid(2));
  CPPUNIT_ASSERT(pid.IsGetValid(511));
  CPPUNIT_ASSERT(pid.IsGetValid(512));
  CPPUNIT_ASSERT(!pid.IsGetValid(513));
  CPPUNIT_ASSERT(!pid.IsGetValid(0xffff));
  CPPUNIT_ASSERT(pid.IsSetValid(0));
  CPPUNIT_ASSERT(pid.IsSetValid(1));
  CPPUNIT_ASSERT(pid.IsSetValid(2));
  CPPUNIT_ASSERT(pid.IsSetValid(511));
  CPPUNIT_ASSERT(pid.IsSetValid(512));
  CPPUNIT_ASSERT(!pid.IsSetValid(513));
  CPPUNIT_ASSERT(pid.IsSetValid(0xffff));
}


/**
 * Check the PidStore works.
 */
void PidStoreTest::testPidStore() {
  const PidDescriptor *foo_pid = new PidDescriptor(
      "foo", 0, NULL, NULL, NULL, NULL,
      PidDescriptor::NON_BROADCAST_SUB_DEVICE,
      PidDescriptor::ANY_SUB_DEVICE);
  const PidDescriptor *bar_pid = new PidDescriptor(
      "bar", 1, NULL, NULL, NULL, NULL,
      PidDescriptor::NON_BROADCAST_SUB_DEVICE,
      PidDescriptor::ANY_SUB_DEVICE);

  vector<const PidDescriptor*> pids;
  pids.push_back(foo_pid);
  pids.push_back(bar_pid);

  PidStore store(pids);

  // check value lookups
  CPPUNIT_ASSERT_EQUAL(foo_pid, store.LookupPID(0));
  CPPUNIT_ASSERT_EQUAL(bar_pid, store.LookupPID(1));
  CPPUNIT_ASSERT_EQUAL(static_cast<const PidDescriptor*>(NULL),
                       store.LookupPID(2));

  // check name lookups
  CPPUNIT_ASSERT_EQUAL(foo_pid, store.LookupPID("foo"));
  CPPUNIT_ASSERT_EQUAL(bar_pid, store.LookupPID("bar"));
  CPPUNIT_ASSERT_EQUAL(static_cast<const PidDescriptor*>(NULL),
                       store.LookupPID("baz"));

  // check all pids;
  vector<const PidDescriptor*> all_pids;
  store.AllPids(&all_pids);
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), all_pids.size());
  CPPUNIT_ASSERT_EQUAL(foo_pid, all_pids[0]);
  CPPUNIT_ASSERT_EQUAL(bar_pid, all_pids[1]);
}


/**
 * Check we can load a PidStore from a string
 */
void PidStoreTest::testPidStoreLoad() {
  PidStoreLoader loader;
  std::stringstream str;

  // check that this fails to load
  const RootPidStore *empty_root_store = loader.LoadFromStream(&str);
  CPPUNIT_ASSERT_EQUAL(static_cast<const RootPidStore*>(NULL),
                       empty_root_store);

  // now try a simple pid store config
  str.clear();
  str << "pid {" << endl <<
         "  name: \"PROXIED_DEVICES\"" << endl <<
         "  value: 16" << endl <<
         "  get_request {" << endl <<
         "  }" << endl <<
         "  get_response {" << endl <<
         "    field {" << endl <<
         "      type: GROUP" << endl <<
         "      name: \"uids\"" << endl <<
         "      field {" << endl <<
         "        type: UINT16" << endl <<
         "        name: \"manufacturer_id\"" << endl <<
         "      }" << endl <<
         "      field {" << endl <<
         "        type: UINT32" << endl <<
         "        name: \"device_id\"" << endl <<
         "      }" << endl <<
         "    }" << endl <<
         "  }" << endl <<
         "  get_sub_device_range: ROOT_DEVICE" << endl <<
         "}" << endl <<
         "manufacturer {" << endl <<
         "  manufacturer_id: 31344" << endl <<
         "  manufacturer_name: \"Open Lighting\"" << endl <<
         "}" << endl <<
         "version: 1" << endl;

  auto_ptr<const RootPidStore> root_store(loader.LoadFromStream(&str));
  CPPUNIT_ASSERT(root_store.get());

  // check version
  CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(1), root_store->Version());

  // check manufacturer pids
  const PidStore *open_lighting_store =
    root_store->ManufacturerStore(OPEN_LIGHTING_ESTA_CODE);
  CPPUNIT_ASSERT(open_lighting_store);
  CPPUNIT_ASSERT_EQUAL(0u, open_lighting_store->PidCount());

  // lookup by value
  CPPUNIT_ASSERT(root_store->GetDescriptor(16));
  CPPUNIT_ASSERT(!root_store->GetDescriptor(17));
  CPPUNIT_ASSERT(root_store->GetDescriptor(16, OPEN_LIGHTING_ESTA_CODE));
  CPPUNIT_ASSERT(!root_store->GetDescriptor(17, OPEN_LIGHTING_ESTA_CODE));

  // lookup by name
  CPPUNIT_ASSERT(root_store->GetDescriptor("PROXIED_DEVICES"));
  CPPUNIT_ASSERT(!root_store->GetDescriptor("DEVICE_INFO"));
  CPPUNIT_ASSERT(root_store->GetDescriptor("PROXIED_DEVICES",
                                           OPEN_LIGHTING_ESTA_CODE));
  CPPUNIT_ASSERT(!root_store->GetDescriptor("DEVICE_INFO",
                                            OPEN_LIGHTING_ESTA_CODE));

  // check lookups
  const PidStore *esta_store = root_store->EstaStore();
  CPPUNIT_ASSERT(esta_store);

  const PidDescriptor *pid_descriptor = esta_store->LookupPID(16);
  CPPUNIT_ASSERT(pid_descriptor);
  const PidDescriptor *pid_descriptor2 = esta_store->LookupPID(
      "PROXIED_DEVICES");
  CPPUNIT_ASSERT(pid_descriptor2);
  CPPUNIT_ASSERT_EQUAL(pid_descriptor, pid_descriptor2);

  // check name and value
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(16), pid_descriptor->Value());
  CPPUNIT_ASSERT_EQUAL(string("PROXIED_DEVICES"), pid_descriptor->Name());

  // check descriptors
  CPPUNIT_ASSERT(pid_descriptor->GetRequest());
  CPPUNIT_ASSERT(pid_descriptor->GetResponse());
  CPPUNIT_ASSERT_EQUAL(static_cast<const Descriptor*>(NULL),
                       pid_descriptor->SetRequest());
  CPPUNIT_ASSERT_EQUAL(static_cast<const Descriptor*>(NULL),
                       pid_descriptor->SetResponse());

  // check GET descriptors
  const Descriptor *get_request = pid_descriptor->GetRequest();
  CPPUNIT_ASSERT_EQUAL(0u, get_request->FieldCount());

  const Descriptor *get_response = pid_descriptor->GetResponse();
  CPPUNIT_ASSERT_EQUAL(1u, get_response->FieldCount());
  const FieldDescriptor *proxied_group = get_response->GetField(0);
  CPPUNIT_ASSERT(proxied_group);

  // this is ugly but it's a test
  const FieldDescriptorGroup *group_descriptor =
    dynamic_cast<const FieldDescriptorGroup*>(proxied_group);  // NOLINT
  CPPUNIT_ASSERT(group_descriptor);

  // check all the group properties
  CPPUNIT_ASSERT(!group_descriptor->FixedSize());
  CPPUNIT_ASSERT(!group_descriptor->LimitedSize());
  CPPUNIT_ASSERT_EQUAL(0u, group_descriptor->MaxSize());
  CPPUNIT_ASSERT_EQUAL(2u, group_descriptor->FieldCount());
  CPPUNIT_ASSERT(group_descriptor->FixedBlockSize());
  CPPUNIT_ASSERT_EQUAL(6u, group_descriptor->BlockSize());
  CPPUNIT_ASSERT_EQUAL(6u, group_descriptor->MaxBlockSize());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(0),
                       group_descriptor->MinBlocks());
  CPPUNIT_ASSERT_EQUAL(FieldDescriptorGroup::UNLIMITED_BLOCKS,
                       group_descriptor->MaxBlocks());
  CPPUNIT_ASSERT(!group_descriptor->FixedBlockCount());

  // Check this prints correctly
  ola::messaging::SchemaPrinter printer;
  get_response->Accept(printer);
  string expected = (
      "uids {\n  manufacturer_id: uint16\n  device_id: uint32\n}\n");
  CPPUNIT_ASSERT_EQUAL(expected, printer.AsString());

  // check sub device ranges
  CPPUNIT_ASSERT(pid_descriptor->IsGetValid(0));
  CPPUNIT_ASSERT(!pid_descriptor->IsGetValid(1));
  CPPUNIT_ASSERT(!pid_descriptor->IsGetValid(512));
  CPPUNIT_ASSERT(!pid_descriptor->IsGetValid(ola::rdm::ALL_RDM_SUBDEVICES));
  CPPUNIT_ASSERT(!pid_descriptor->IsSetValid(0));
  CPPUNIT_ASSERT(!pid_descriptor->IsSetValid(1));
  CPPUNIT_ASSERT(!pid_descriptor->IsSetValid(512));
  CPPUNIT_ASSERT(!pid_descriptor->IsSetValid(ola::rdm::ALL_RDM_SUBDEVICES));
}


/**
 * Check that loading from a file works
 */
void PidStoreTest::testPidStoreFileLoad() {
  PidStoreLoader loader;

  auto_ptr<const RootPidStore> root_store(loader.LoadFromFile(
      "./testdata/test_pids.proto"));
  CPPUNIT_ASSERT(root_store.get());
  // check version
  CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(1302986774),
                       root_store->Version());

  // Check all the esta pids are there
  const PidStore *esta_store = root_store->EstaStore();
  CPPUNIT_ASSERT(esta_store);

  vector<const PidDescriptor*> all_pids;
  esta_store->AllPids(&all_pids);
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(70), all_pids.size());

  // check for device info
  const PidDescriptor *device_info = esta_store->LookupPID("DEVICE_INFO");
  CPPUNIT_ASSERT(device_info);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(96), device_info->Value());
  CPPUNIT_ASSERT_EQUAL(string("DEVICE_INFO"), device_info->Name());

  // check descriptors
  CPPUNIT_ASSERT(device_info->GetRequest());
  CPPUNIT_ASSERT(device_info->GetResponse());
  CPPUNIT_ASSERT_EQUAL(static_cast<const Descriptor*>(NULL),
                       device_info->SetRequest());
  CPPUNIT_ASSERT_EQUAL(static_cast<const Descriptor*>(NULL),
                       device_info->SetResponse());

  ola::messaging::SchemaPrinter printer;
  device_info->GetResponse()->Accept(printer);
  string expected = (
      "protocol_major: uint8\nprotocol_minor: uint8\ndevice_model: uint16\n"
      "product_category: uint16\nsoftware_version: uint32\n"
      "dmx_footprint: uint16\ncurrent_personality: uint8\n"
      "personality_count: uint8\ndmx_start_address: uint16\n"
      "sub_device_count: uint16\nsensor_count: uint8\n");
  CPPUNIT_ASSERT_EQUAL(expected, printer.AsString());

  // check manufacturer pids
  const PidStore *open_lighting_store =
    root_store->ManufacturerStore(OPEN_LIGHTING_ESTA_CODE);
  CPPUNIT_ASSERT(open_lighting_store);
  CPPUNIT_ASSERT_EQUAL(1u, open_lighting_store->PidCount());

  const PidDescriptor *serial_number = open_lighting_store->LookupPID(
      "SERIAL_NUMBER");
  CPPUNIT_ASSERT(serial_number);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(32768), serial_number->Value());
  CPPUNIT_ASSERT_EQUAL(string("SERIAL_NUMBER"), serial_number->Name());

  // check descriptors
  CPPUNIT_ASSERT_EQUAL(static_cast<const Descriptor*>(NULL),
                       serial_number->GetRequest());
  CPPUNIT_ASSERT_EQUAL(static_cast<const Descriptor*>(NULL),
                       serial_number->GetResponse());
  CPPUNIT_ASSERT(serial_number->SetRequest());
  CPPUNIT_ASSERT(serial_number->SetResponse());

  printer.Reset();
  serial_number->SetRequest()->Accept(printer);
  string expected2 = "serial_number: uint32\n";
  CPPUNIT_ASSERT_EQUAL(expected2, printer.AsString());
}


/**
 * Check that loading a missing file fails.
 */
void PidStoreTest::testPidStoreLoadMissingFile() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      "./testdata/missing_file_pids.proto");
  CPPUNIT_ASSERT(!root_store);
}


/**
 * Check that loading a file with duplicate manufacturers fails.
 */
void PidStoreTest::testPidStoreLoadDuplicateManufacturer() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      "./testdata/duplicate_manufacturer.proto");
  CPPUNIT_ASSERT(!root_store);
}


/**
 * Check that loading file with duplicate pid values fails.
 */
void PidStoreTest::testPidStoreLoadDuplicateValue() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      "./testdata/duplicate_pid_value.proto");
  CPPUNIT_ASSERT(!root_store);
}


/**
 * Check that loading a file with duplicate pid names fails.
 */
void PidStoreTest::testPidStoreLoadDuplicateName() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      "./testdata/duplicate_pid_name.proto");
  CPPUNIT_ASSERT(!root_store);
}


/**
 * Check that loading a file with an out-of-range ESTA pid fails.
 */
void PidStoreTest::testPidStoreLoadInvalidEstaPid() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      "./testdata/invalid_esta_pid.proto");
  CPPUNIT_ASSERT(!root_store);
}


/**
 * Check that loading a file with an inconsistent descriptor fails.
 */
void PidStoreTest::testInconsistentData() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      "./testdata/inconsistent_pid.proto");
  CPPUNIT_ASSERT(!root_store);
}
