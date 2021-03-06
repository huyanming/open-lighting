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
 * EndpointManager.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <vector>
#include "tools/e133/EndpointManager.h"

using std::vector;

/**
 * Register a E133Endpoint
 * @param endpoint_id the endpoint index
 * @param endpoint E133Endpoint to register
 * @return true if the registration succeeded, false otherwise.
 */
bool EndpointManager::RegisterEndpoint(uint16_t endpoint_id,
                                       E133Endpoint *endpoint) {
  if (!endpoint_id) {
    OLA_WARN << "Can't register the root endpoint";
  }

  endpoint_map::iterator iter = m_endpoint_map.find(endpoint_id);
  if (iter == m_endpoint_map.end()) {
    m_endpoint_map[endpoint_id] = endpoint;
    RunNotifications(endpoint_id, ADD);
    return true;
  }
  return false;
}


/**
 * Unregister a E133Endpoint
 * @param endpoint_id the index of the endpont to de-register
 */
void EndpointManager::UnRegisterEndpoint(uint16_t endpoint_id) {
  endpoint_map::iterator iter = m_endpoint_map.find(endpoint_id);
  if (iter != m_endpoint_map.end()) {
    RunNotifications(endpoint_id, REMOVE);
    m_endpoint_map.erase(iter);
  }
}


/**
 * Lookup an endpoint by number.
 */
E133Endpoint* EndpointManager::GetEndpoint(uint16_t endpoint_id) const {
  endpoint_map::const_iterator iter = m_endpoint_map.find(endpoint_id);
  if (iter == m_endpoint_map.end()) {
    OLA_INFO << "Request to endpoint " << endpoint_id <<
      " but no Endpoint has been registered, this is a bug!";
    return NULL;
  }
  return iter->second;
}


/**
 * Fetch a list of all registered endpoints.
 * @param id_list pointer to a vector to be filled in with the endpoint ids.
 */
void EndpointManager::EndpointIDs(vector<uint16_t> *id_list) const {
  id_list->clear();
  id_list->reserve(m_endpoint_map.size());

  endpoint_map::const_iterator iter = m_endpoint_map.begin();
  for (; iter != m_endpoint_map. end(); ++iter)
    id_list->push_back(iter->first);
}


/**
 * Register a callback to run when endpoint are added or removed.
 * @param event_type the events to trigger this notification, ADD, REMOVE or
 * BOTH.
 * @param callback the Callback to run. Ownership is not transferred.
 */
void EndpointManager::RegisterNotification(
    EndpointNoticationEvent event_type,
    EndpointNotificationCallback *callback) {
  // if this callback already exists update it
  vector<EndpointNotification>::iterator iter = m_callbacks.begin();
  for (; iter != m_callbacks.end(); ++iter) {
    if (iter->callback == callback) {
      iter->event_type = event_type;
      return;
    }
  }
  EndpointNotification notification = {event_type, callback};
  m_callbacks.push_back(notification);
}


/*
 * Unregister a callback for notifications
 * @param callback the Callback to remove.
 * @return true if the notification was removed, false if it wasn't found.
 */
bool EndpointManager::UnRegisterNotification(
    EndpointNotificationCallback *callback) {
  vector<EndpointNotification>::iterator iter = m_callbacks.begin();
  for (; iter != m_callbacks.end(); ++iter) {
    if (iter->callback == callback) {
      m_callbacks.erase(iter);
      return true;
    }
  }
  return false;
}


/**
 * Run all notifications of a particular type
 * @param endpoint_id the id of the endpoint to pass to the callbacks.
 * @param event_type the type of notifications to trigger.
 */
void EndpointManager::RunNotifications(uint16_t endpoint_id,
                                       EndpointNoticationEvent event_type) {
  vector<EndpointNotification>::iterator iter = m_callbacks.begin();
  for (; iter != m_callbacks.end(); ++iter) {
    if (iter->event_type == event_type || event_type == BOTH)
      iter->callback->Run(endpoint_id);
  }
}
