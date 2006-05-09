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
 * pluginadaptor.h
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */


#ifndef PLUGINADAPTOR_H
#define PLUGINADAPTOR_H

#include <lla/device.h>
#include <lla/fdlistener.h>

class DeviceManager ;
class Network ;

class PluginAdaptor {

	public :
		enum Direction{READ, WRITE};
			
		PluginAdaptor(DeviceManager *dm, Network *net) ;
		int register_fd(int fd, PluginAdaptor::Direction dir, FDListener *listener) ;
		int unregister_fd(int fd, PluginAdaptor::Direction dir) ;
		int register_device(Device *dev) ;
		int unregister_device(Device *dev) ;

	private :
		DeviceManager *dm ;
		Network *net;
		
};

#endif