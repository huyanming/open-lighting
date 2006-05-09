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
 *
 * dummyport_h
 * Dummy port
 * Copyright (C) 2005  Simon Newton
 */

#ifndef DUMMYPORT_H
#define DUMMYPORT_H

#include <lla/port.h>

class DummyPort : public Port  {

	public:
		DummyPort(Device *parent, int id) ;
		
		int write(uint8_t *data, int length) ;
		int read(uint8_t *data, int length) ;
			
	private:
		uint8_t m_dmx[512] ;				// pointer to our dmx buffer
		int m_length;					// length of dmx buffer
};

#endif