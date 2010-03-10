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
 * UID.h
 * Representation of an RDM UID
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_UID_H_
#define INCLUDE_OLA_RDM_UID_H_

#include <stdint.h>
#include <iomanip>
#include <string>

namespace ola {
namespace rdm {

using std::ostream;


/*
 * Represents a RDM UID.
 */
class UID {
  public:
    UID(uint16_t esta_id, uint32_t device_id) {
      m_uid.esta_id = esta_id;
      m_uid.device_id = device_id;
    }

    UID& operator=(const UID& other) {
      if (this != &other) {
        m_uid.esta_id = other.m_uid.esta_id;
        m_uid.device_id = other.m_uid.device_id;
      }
      return *this;
    }

    bool operator==(const UID &other) const {
      return 0 == cmp(*this, other);
    }

    bool operator!=(const UID &other) const {
      return 0 != cmp(*this, other);
    }

    bool operator>(const UID &other) const {
      return cmp(*this, other) > 0;
    }

    bool operator<(const UID &other) const {
      return cmp(*this, other) < 0;
    }

    std::string ToString() const {
      std::stringstream str;
      str << std::setfill('0') << std::setw(4) << std::hex << m_uid.esta_id
        << ":" << std::setw(8) << m_uid.device_id;
      return str.str();
    }

    friend ostream& operator<< (ostream &out, const UID &uid) {
      return out << uid.ToString();
    }

    struct rdm_uid {
      uint16_t esta_id;
      uint32_t device_id;
    };

    static UID AllDevices() {
      UID uid(0xffff, 0xffffffff);
      return uid;
    }

    static UID AllManufactureDevices(uint16_t esta_id) {
      UID uid(esta_id, 0xffffffff);
      return uid;
    }

  private:
    struct rdm_uid m_uid;

    int cmp(const UID &a, const UID &b) const {
      if (a.m_uid.esta_id == b.m_uid.esta_id)
        return cmp(a.m_uid.device_id, b.m_uid.device_id);
      return cmp(a.m_uid.esta_id, b.m_uid.esta_id);
    }

    int cmp(int a, int b) const {
      if (a == b)
        return 0;
      return a < b ? -1 : 1;
    }
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_UID_H_