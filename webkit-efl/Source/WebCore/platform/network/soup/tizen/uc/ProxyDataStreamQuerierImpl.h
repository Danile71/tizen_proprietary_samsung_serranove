/*
 * Copyright (C) 2014 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __UCProxy_ProxyDataStreamQuerierImpl__H__
#define __UCProxy_ProxyDataStreamQuerierImpl__H__

#include <UCProxySDK/ProxyDataStreamQuerier.h>

namespace proxy
{
    class ProxyDataStreamQuerierImpl : public ProxyDataStreamQuerier
    {
        
    public:
        /*************************************************************************************************
        * @Destructor for compressed data query.
        * 
        * Detailed description: 
            Pure abstract class should have a virtual destructor.
        *************************************************************************************************/
        ProxyDataStreamQuerierImpl(); 
        ~ProxyDataStreamQuerierImpl();

        /*************************************************************************************************
        * @Function name:  onProxyDataStreamReceived 
        * 
        * Detailed description: return original and delta size .
        * Need to cumulate to calculate saved ratio.
        *
        * Saved ratio = Cumulated Size Delta/Cumulated Original Size
        * Saved ratio means how many packets are saved.
        *
        * @param[out] originalData : The size which original web server gives to UC proxy server.
        * @param[out] deltaBetweenOriginalNTransmissionSize : equals Original Size - Transmission Size
        * Transmission size: The size which UC proxy server gives to UC SDK.
        *************************************************************************************************/
        virtual void onProxyDataStreamReceived(int originalData, int deltaBetweenOriginalNTransmissionSize);
    };

}// namespace Proxy

#endif //__UCProxy__HTTPCacheQuerierImpl__H__
