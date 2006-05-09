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
 * shownetport.cpp
 * The ShowNet plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "shownetport.h"
#include "shownetdevice.h"
#include "common.h"

#include <lla/universe.h>
#include <lla/logger.h>

#include <string.h>

#define min(a,b) a<b?a:b

ShowNetPort::ShowNetPort(Device *parent, int id) : Port(parent, id) {

	// if this is an in port, setup the buffer
	if(can_read()) {
		m_len = DMX_LENGTH ;
		m_buf = (uint8_t*) malloc(m_len) ;

		// we should handle this better
		if(m_buf == NULL) 
			Logger::instance()->log(Logger::CRIT, "ShownetPlugin: malloc failed") ;
		else
			memset(m_buf, 0x00, m_len) ;
	} else {
		m_buf  = NULL ;
		m_len = 0;
	}

}

ShowNetPort::~ShowNetPort() {

	if(can_read())
		free(m_buf) ;
}

int ShowNetPort::can_read() {
	// ports 0 to 7 are input
	return ( get_id()>=0 && get_id() < PORTS_PER_DEVICE);
}

int ShowNetPort::can_write() {
	// ports 8 to 13 are output
	return ( get_id()>= PORTS_PER_DEVICE && get_id() <2*PORTS_PER_DEVICE);
}

/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int ShowNetPort::write(uint8_t *data, int length) {
	ShowNetDevice *dev = (ShowNetDevice*) get_device() ;
	int ret ;

	if( !can_write())
		return -1 ;
	
	if(shownet_send_dmx(dev->get_node() , get_id()%8 , length, data)) {
		Logger::instance()->log(Logger::WARN, "ShownetPlugin: shownet_send_dmx failed %s", shownet_strerror() ) ;
		return -1 ;
	}
	return 0;
}

/*
 * Read operation
 *
 * @param 	data	buffer to read data into
 * @param 	length	length of data to read
 *
 * @return	the amount of data read
 */
int ShowNetPort::read(uint8_t *data, int length) {
	uint8_t *dmx = NULL;
	int len ;
	ShowNetDevice *dev = (ShowNetDevice*) get_device() ;
	
	if( !can_read()) 
		return -1 ;
	
	len = min(m_len, length) ;
	memcpy(data, m_buf, len ) ;
	return len;
}

/*
 * Update the data buffer for this port
 *
 */
int ShowNetPort::update_buffer(uint8_t *data, int length) {
	int len = min(DMX_LENGTH, length) ;

	// we can't update if this isn't a input port
	if(! can_read())
		return -1 ;
	
	Logger::instance()->log(Logger::DEBUG, "ShowNet: Updating dmx buffer for port %d", length);
	memcpy(m_buf, data, len);

	dmx_changed() ;
}