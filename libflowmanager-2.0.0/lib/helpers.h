/*
 * This file is part of libflowmanager
 *
 * Copyright (c) 2009 The University of Waikato, Hamilton, New Zealand.
 * Author: Shane Alcock
 *
 * All rights reserved.
 *
 * This code has been developed by the University of Waikato WAND
 * research group. For further information please see http://www.wand.net.nz/
 *
 * libflowmanager is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libflowmanager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libflowmanager; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: tcp_reorder.h 30 2010-02-21 22:57:29Z salcock $
 *
 */


#ifndef HELPERS_H_
#define HELPERS_H_



#include <libtrace.h>

#ifdef __cplusplus
extern "C" {
#endif


        bool rfc1918_ip_addr ( uint32_t ip_addr );
        bool rfc1918_ip ( libtrace_ip_t *ip );
        uint16_t extract_vlan_id ( libtrace_packet_t *packet );


#ifdef __cplusplus
    }
#endif

#endif
