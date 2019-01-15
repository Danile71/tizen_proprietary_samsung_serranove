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

#include "config.h"
#include "NetTransactionImpl.h"

#include "Logging.h"
#include <UCProxySDK/NetTransactionDelegate.h>

namespace proxy {

NetTransactionImpl::NetTransactionImpl(NetTransactionDelegate* delegate)
:   m_delegate(delegate)
{
}

NetTransactionImpl::~NetTransactionImpl()
{
    releaseTransactionSoup();
}

bool NetTransactionImpl::start(const HTTPURLRequest& request)
{
    m_transactionSoup = NetTransactionSoup::create(request, this);
    if (m_transactionSoup) {
        m_transactionSoup->start(request);
        return true;
    } else {
        TIZEN_LOGI("[HTTPWrapper] fail to create NetTransactionSoup");
        return false;
    }
}

void NetTransactionImpl::cancel()
{
    if (m_transactionSoup) {
        m_transactionSoup->cancel();
        releaseTransactionSoup();
    }
}

void NetTransactionImpl::onResponseReceived(const HTTPURLResponse& response)
{
    m_delegate->onResponseReceived(response);
}

void NetTransactionImpl::onDataReceived(const char* data, int length)
{
    m_delegate->onDataReceived(data, length);
}

void NetTransactionImpl::onFinishLoading()
{
    releaseTransactionSoup();

    m_delegate->onFinishLoading();
}

void NetTransactionImpl::onError(const std::string& domain, int errorCode, const std::string& failingURL, const std::string& localizedDescription)
{
    TIZEN_SECURE_LOGI ("[HTTPWrapper] this[%p] m_transactionSoup[%p] domain[%s] errorCode[%d] failingURL[%s] localizedDescription[%s]",
        this, m_transactionSoup, domain.c_str(), errorCode, failingURL.c_str(), localizedDescription.c_str());
    releaseTransactionSoup();

    // Actually this NetTransactionImpl::onError() should not be called
    // because NetTransactionImpl::cancel() already called releaseTransactionSoup().
    // But, this check is necessary because NetTransactionSoup calls to m_client->onError() before releaseTransactionSoup() called.
    // That is libsoup's original flow for canceling.
    // So, cancel flow is like this.
    //
    // 1. NetTransactionImpl::cancel() START
    // 2. NetTransactionSoup::cancel() START
    // 3. soup_session_cancel_message() START
    // 4. finishedCallback() on NetTransactionSoup START
    // 5. NetTransactionSoup::gotFinished() START
    // 6. NetTransactionImpl::onError() START => because NetTransactionSoup.m_client is still alive.
    // 7. m_delegate->onError() called, and this cause crash
    // 8. NetTransactionImpl::onError() END
    // 9. NetTransactionSoup::gotFinished() END
    // 10. inishedCallback() on NetTransactionSoup END
    // 11. soup_session_cancel_message() END
    // 12. NetTransactionSoup::cancel() END
    // 13. NetTransactionImpl::cancel() calls releaseTransactionSoup() => now NetTransactionSoup.m_client is cleared.
    // 14. NetTransactionImpl::cancel() END
    if (errorCode != SOUP_STATUS_CANCELLED)
        m_delegate->onError(domain, errorCode, failingURL, localizedDescription);
    else
        TIZEN_LOGI("[HTTPWrapper] this[%p] not to call m_delegate->onError() because it is canceled.", this);
}

void NetTransactionImpl::releaseTransactionSoup()
{
    if(m_transactionSoup) {
        if (m_transactionSoup->client() == this)
            m_transactionSoup->setClient(0);
        m_transactionSoup->deref();
        m_transactionSoup = NULL;
    }
}

} // namespace proxy
