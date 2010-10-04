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
 * RDMHttpModule.h
 * This module acts as the http -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_RDMHTTPMODULE_H_
#define OLAD_RDMHTTPMODULE_H_

#include <map>
#include <queue>
#include <string>
#include <vector>
#include "ola/OlaCallbackClient.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/UID.h"
#include "olad/HttpServer.h"
#include "olad/HttpModule.h"

namespace ola {

using std::string;
using ola::rdm::UID;


/*
 * The module that deals with RDM requests.
 * TODO(simon): factor out the common functionality like RegisterHandler into a
 *   parent class.
 */
class RDMHttpModule: public HttpModule {
  public:
    RDMHttpModule(HttpServer *http_server,
                  class OlaCallbackClient *client);
    ~RDMHttpModule();

    int RunRDMDiscovery(const HttpRequest *request, HttpResponse *response);

    int JsonUIDs(const HttpRequest *request, HttpResponse *response);
    int JsonSupportedPIDs(const HttpRequest *request, HttpResponse *response);
    int JsonSupportedSections(const HttpRequest *request,
                              HttpResponse *response);
    int JsonSectionInfo(const HttpRequest *request,
                        HttpResponse *response);

    void PruneUniverseList(const vector<OlaUniverse> &universes);

  private:
    typedef struct {
      string manufacturer;
      string device;
      bool active;
    } resolved_uid;

    typedef enum {
      RESOLVE_MANUFACTURER,
      RESOLVE_DEVICE,
    } uid_resolve_action;

    typedef struct {
      map<UID, resolved_uid> resolved_uids;
      std::queue<std::pair<UID, uid_resolve_action> > pending_uids;
      bool uid_resolution_running;
      bool active;
    } uid_resolution_state;

    HttpServer *m_server;
    class OlaCallbackClient *m_client;
    ola::rdm::RDMAPI m_rdm_api;
    map<unsigned int, uid_resolution_state*> m_universe_uids;

    RDMHttpModule(const RDMHttpModule&);
    RDMHttpModule& operator=(const RDMHttpModule&);

    // uid resolution methods
    void HandleUIDList(HttpResponse *response,
                       unsigned int universe_id,
                       const ola::rdm::UIDSet &uids,
                       const string &error);

    void ResolveNextUID(unsigned int universe_id);

    void UpdateUIDManufacturerLabel(unsigned int universe,
                                    UID uid,
                                    const ola::rdm::ResponseStatus &status,
                                    const string &device_label);

    void UpdateUIDDeviceLabel(unsigned int universe,
                              UID uid,
                              const ola::rdm::ResponseStatus &status,
                              const string &device_label);

    uid_resolution_state *GetUniverseUids(unsigned int universe);
    uid_resolution_state *GetUniverseUidsOrCreate(unsigned int universe);

    // supported params / sections
    void SupportedParamsHandler(HttpResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                const vector<uint16_t> &pids);
    void SupportedSectionsHandler(HttpResponse *response,
                                  const ola::rdm::ResponseStatus &status,
                                  const vector<uint16_t> &pids);

    // section methods
    string ProcessDeviceInfo(const HttpRequest *request,
                             HttpResponse *response,
                             unsigned int universe_id,
                             const UID &uid);

    void GetDeviceInfoHandler(HttpResponse *response,
                              const ola::rdm::ResponseStatus &status,
                              const ola::rdm::DeviceDescriptor &device);

    string ProcessStartAddress(const HttpRequest *request,
                               HttpResponse *response,
                               unsigned int universe_id,
                               const UID &uid);

    void GetStartAddressHandler(HttpResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                uint16_t address);



    // util methods
    bool CheckForInvalidId(const HttpRequest *request,
                           unsigned int *universe_id);

    bool CheckForInvalidUid(const HttpRequest *request, UID **uid);

    bool CheckForRDMSuccess(const ola::rdm::ResponseStatus &status);

    void HandleBoolResponse(HttpResponse *response, const string &error);

    static const char BACKEND_DISCONNECTED_ERROR[];
    static const char ID_KEY[];
    static const char SECTION_KEY[];
    static const char UID_KEY[];
};
}  // ola
#endif  // OLAD_RDMHTTPMODULE_H_