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

#ifndef __UCProxy__NetTransactionImpl__H__
#define __UCProxy__NetTransactionImpl__H__

#include <UCProxySDK/NetTransaction.h>
#include "NetTransactionClient.h"
#include "NetTransactionSoup.h"

namespace proxy {

class HTTPURLRequest;
class HTTPURLResponse;
class NetTransactionDelegate;

class NetTransactionImpl : public NetTransaction, public NetTransactionClient
{
public:
    /*************************************************************************************************
     * @Destructor for NetTransactionImpl.
     *
     * Detailed description:
     Pure abstract class should have a virtual destructor.
     *************************************************************************************************/
    ~NetTransactionImpl();
    NetTransactionImpl(NetTransactionDelegate* delegate);

    /*************************************************************************************************
     * @Function name:  submit
     *
     * Detailed description:
     begin the real http request.
     handler should delete request after using it.
     * @param[in] request  : the request interface, used to get all request parameters
     * @return bool        : return true if start successfully, and vise verse.
     *************************************************************************************************/
    bool start(const HTTPURLRequest& request);

    /*************************************************************************************************
     * @Function name:  cancel
     *
     * Detailed description:
      cancel a specified request.
     *************************************************************************************************/
    void cancel();

    virtual void onResponseReceived(const HTTPURLResponse& response);

    virtual void onDataReceived(const char* data, int length);

    virtual void onFinishLoading();

    virtual void onError(const std::string& domain, int errorCode, const std::string& failingURL, const std::string& localizedDescription);

private:
    void releaseTransactionSoup();

    NetTransactionDelegate* m_delegate;
    NetTransactionSoup* m_transactionSoup;
};


// An interface to a class that can create HttpTransaction objects.
class NetTransactionFactoryImpl : public NetTransactionFactory
{
public:
    ~NetTransactionFactoryImpl() {}
    
    // Creates a HttpTransaction object. 
    NetTransaction* createTransaction(NetTransactionDelegate* delegate)
    {
        return new NetTransactionImpl(delegate);
    }
};

}


#endif /* defined(__UCProxy__NetTransactionImpl__H__) */
