/*
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 * Copyright (C) 2008, 2010 Collabora Ltd.
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2009, 2010, 2011 Igalia S.L.
 * Copyright (C) 2009 John Kjellberg <john.kjellberg@power.alstom.com>
 * Copyright (C) 2012 Intel Corporation
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
#include "ResourceHandle.h"

#include "CachedResourceLoader.h"
#include "ChromeClient.h"
#include "CookieJarSoup.h"
#include "FileSystem.h"
#include "Frame.h"
#include "GOwnPtrSoup.h"
#include "HTTPParsers.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Page.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "TextEncoding.h"
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-request-http.h>
#include <libsoup/soup-requester.h>
#include <libsoup/soup.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(BLOB)
#include "BlobData.h"
#include "BlobRegistryImpl.h"
#include "BlobStorageData.h"
#endif

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystemTizen.h"
#endif

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
#include "CredentialStorage.h"
#endif

#if ENABLE(TIZEN_PRIVATE_BROWSING)
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-cache.h>
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#include <sys/time.h>
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
#include "ResourceLoader.h"
#endif

namespace WebCore {
#if ENABLE(TIZEN_EXPONENTIAL_BUFFER_SIZE)
#define READ_BUFFER_SIZE_MIN (8*1024)
#define READ_BUFFER_SIZE_MAX (64*1024)
#else
#define READ_BUFFER_SIZE 8192
#endif

// Use the same value as in NSURLError.h
static const int gTimeoutError = -1001;

static bool loadingSynchronousRequest = false;

class WebCoreSynchronousLoader : public ResourceHandleClient {
    WTF_MAKE_NONCOPYABLE(WebCoreSynchronousLoader);
public:

    WebCoreSynchronousLoader(ResourceError& error, ResourceResponse& response, SoupSession* session, Vector<char>& data)
        : m_error(error)
        , m_response(response)
        , m_session(session)
        , m_data(data)
        , m_finished(false)
    {
        // We don't want any timers to fire while we are doing our synchronous load
        // so we replace the thread default main context. The main loop iterations
        // will only process GSources associated with this inner context.
        loadingSynchronousRequest = true;
        GRefPtr<GMainContext> innerMainContext = adoptGRef(g_main_context_new());
        g_main_context_push_thread_default(innerMainContext.get());
        m_mainLoop = adoptGRef(g_main_loop_new(innerMainContext.get(), false));

        adjustMaxConnections(1);
    }

    ~WebCoreSynchronousLoader()
    {
        adjustMaxConnections(-1);
        g_main_context_pop_thread_default(g_main_context_get_thread_default());
        loadingSynchronousRequest = false;
    }

    void adjustMaxConnections(int adjustment)
    {
        int maxConnections, maxConnectionsPerHost;
        g_object_get(m_session,
                     SOUP_SESSION_MAX_CONNS, &maxConnections,
                     SOUP_SESSION_MAX_CONNS_PER_HOST, &maxConnectionsPerHost,
                     NULL);
        maxConnections += adjustment;
        maxConnectionsPerHost += adjustment;
        g_object_set(m_session,
                     SOUP_SESSION_MAX_CONNS, maxConnections,
                     SOUP_SESSION_MAX_CONNS_PER_HOST, maxConnectionsPerHost,
                     NULL);

    }

    virtual bool isSynchronousClient()
    {
        return true;
    }

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
    {
        m_response = response;
    }

    virtual void didReceiveData(ResourceHandle*, const char* data, int length, int)
    {
        m_data.append(data, length);
    }

    virtual void didFinishLoading(ResourceHandle*, double)
    {
        if (g_main_loop_is_running(m_mainLoop.get()))
            g_main_loop_quit(m_mainLoop.get());
        m_finished = true;
    }

    virtual void didFail(ResourceHandle* handle, const ResourceError& error)
    {
        m_error = error;
        didFinishLoading(handle, 0);
    }

    void run()
    {
        if (!m_finished)
            g_main_loop_run(m_mainLoop.get());
    }

private:
    ResourceError& m_error;
    ResourceResponse& m_response;
    SoupSession* m_session;
    Vector<char>& m_data;
    bool m_finished;
    GRefPtr<GMainLoop> m_mainLoop;
};

static void cleanupSoupRequestOperation(ResourceHandle*, bool isDestroying);
static void sendRequestCallback(GObject*, GAsyncResult*, gpointer);
static void readCallback(GObject*, GAsyncResult*, gpointer);
static void closeCallback(GObject*, GAsyncResult*, gpointer);
static gboolean requestTimeoutCallback(void*);
static bool startNonHTTPRequest(ResourceHandle*, KURL);
#if ENABLE(TIZEN_COMPRESSION_PROXY)
static bool startHTTPRequest(ResourceHandle* handle, ResourceRequest& request);
#else
static bool startHTTPRequest(ResourceHandle* handle);
#endif
#if ENABLE(WEB_TIMING)
static int  milisecondsSinceRequest(double requestTime);
#endif
#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
static void applyAuthenticationToRequest(ResourceHandle* handle, ResourceRequest& request, bool redirect);
#endif

ResourceHandleInternal::~ResourceHandleInternal()
{
}

static SoupSession* sessionFromContext(NetworkingContext* context)
{
    return (context && context->isValid()) ? context->soupSession() : ResourceHandle::defaultSession();
}

SoupSession* ResourceHandleInternal::soupSession()
{
    return sessionFromContext(m_context.get());
}

uint64_t ResourceHandleInternal::initiatingPageID()
{
    return (m_context && m_context->isValid()) ? m_context->initiatingPageID() : 0;
}

ResourceHandle::~ResourceHandle()
{
    cleanupSoupRequestOperation(this, true);
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    stopUC(true);
#endif
}

#if ENABLE(TIZEN_LOG)
static void soupLoggerPrinter(SoupLogger* logger, SoupLoggerLogLevel, char direction, const char* data, gpointer)
{
    LOG(Network,"[Network-libsoup] %c %s\n", direction, data);
}
#endif

#if ENABLE(TIZEN_ALLOW_CERTIFICATE_TRUSTED_URI)
static const char* certificateTrustedUris[] = { "developer.tizen.org", "logs1252.xiti.com", "vimeo.com", "mobile.twitter.com", "ma.twimg.com" };
#endif

static void ensureSessionIsInitialized(SoupSession* session)
{
#if ENABLE(TIZEN_TIMEOUT_FIX)
    static const int defaultTimeout = 20; // seconds
#endif

    if (g_object_get_data(G_OBJECT(session), "webkit-init"))
        return;

    if (session == ResourceHandle::defaultSession()) {
        SoupCookieJar* jar = SOUP_COOKIE_JAR(soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR));
        if (!jar)
            soup_session_add_feature(session, SOUP_SESSION_FEATURE(soupCookieJar()));
        else
            setSoupCookieJar(jar);
    }

#if !LOG_DISABLED
    if (!soup_session_get_feature(session, SOUP_TYPE_LOGGER) && LogNetwork.state == WTFLogChannelOn) {
        SoupLogger* logger = soup_logger_new(static_cast<SoupLoggerLogLevel>(SOUP_LOGGER_LOG_BODY), -1);
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(logger));
#if ENABLE(TIZEN_LOG)
        soup_logger_set_printer(logger, soupLoggerPrinter, NULL, NULL);
#endif
        g_object_unref(logger);
    }
#endif // !LOG_DISABLED

    if (!soup_session_get_feature(session, SOUP_TYPE_REQUESTER)) {
        SoupRequester* requester = soup_requester_new();
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(requester));
        g_object_unref(requester);
    }

#if ENABLE(TIZEN_TIMEOUT_FIX)
    LOG(Network,"[Network] ensureSessionIsInitialized() defaultTimeout [%d]", defaultTimeout);
    g_object_set(session, SOUP_SESSION_TIMEOUT, defaultTimeout, NULL);
#endif
    g_object_set_data(G_OBJECT(session), "webkit-init", reinterpret_cast<void*>(0xdeadbeef));
}

static void gotHeadersCallback(SoupMessage* msg, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] gotHeadersCallback !handle");
#endif
        return;
    }
    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_cancelled) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] gotHeadersCallback d->m_cancelled");
#endif
        return;
    }

#if ENABLE(WEB_TIMING)
    if (d->m_response.resourceLoadTiming())
        d->m_response.resourceLoadTiming()->receiveHeadersEnd = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#if PLATFORM(EFL) && USE(SOUP)
    bool isTrusted = false;

    if (handle) {
        SoupURI* soupUri = soup_message_get_uri(msg);
        char* uri = soup_uri_to_string(soupUri, false);
        if (WebCore::protocolIs(String(uri), "https")) { // only for https sites
            if (soup_message_get_flags (msg) & SOUP_MESSAGE_CERTIFICATE_TRUSTED)
                isTrusted = true;

            GTlsCertificate* certificate = NULL;
            GTlsCertificateFlags errors = G_TLS_CERTIFICATE_VALIDATE_ALL;
            gchar* certificate_pem = NULL;
            g_object_get(msg, SOUP_MESSAGE_TLS_CERTIFICATE, &certificate, SOUP_MESSAGE_TLS_ERRORS, &errors, NULL); // get peer Certificate for error case
            if (certificate) {
                g_object_get(G_OBJECT (certificate), "certificate-pem", &certificate_pem, NULL); // get Certificate PEM Data
                g_object_unref (certificate);
            }
            // Add this log to find out the error cause if the error is occurred for certificate validation.
            if (static_cast<int>(errors)) {
                time_t localTime = time(0);
                tm localt;
                getLocalTime(&localTime, &localt);

                if (localt.tm_year >= 100)    // In case of over 2000 year, tm_year is displayed 1XX. so to view more friendly, add this condition.
                    localt.tm_year = (localt.tm_year + 2000) - 100;

                localt.tm_mon++;    // mon : 0~11

#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGI("[Network] gotHeadersCallback certificate date [%02d/%02d/%02d]", localt.tm_year, localt.tm_mon, localt.tm_mday);
                // Never print URI itself. Whole URI can import non-printable character in it.
                TIZEN_SECURE_LOGI("[Network] gotHeadersCallback certificate host [%s], isTrusted [%d], errors [%d]", soup_uri_get_host(soupUri), isTrusted, static_cast<int>(errors));
#endif
            }

            bool shouldContinue = true;
            ResourceHandleClient* client = handle->client();
            bool shouldTrustForUri = false;

#if ENABLE(TIZEN_ALLOW_CERTIFICATE_TRUSTED_URI)
            // FIXME: should find why the specific certificate does not be validated from the proper Root CA.
            const char* hostUri = soup_uri_get_host(soupUri);
            for (int i = 0; i < sizeof(certificateTrustedUris)/(sizeof(certificateTrustedUris[0])); i++) {
                if (equalIgnoringCase(String::fromUTF8(hostUri), certificateTrustedUris[i]))
                    shouldTrustForUri = true;
            }
#endif
            if (client && !shouldTrustForUri)
                shouldContinue = client->didReceiveCertificateError(handle, isTrusted, uri, certificate_pem, static_cast<int>(errors));
            g_free(certificate_pem);

            if (!shouldContinue) { // if user decided not to continue
                handle->cancel();  // cancel libsoup message
                if (client) {
                    ResourceError error(g_quark_to_string(SOUP_HTTP_ERROR),
                            SOUP_STATUS_SSL_FAILED,
                            uri,
                            isTrusted ? "Loading page cancelled."
                                        /* Generally we shouldn't get 'isTrusted == true' here because g_certificate_confirmation_callback
                                           should rather return false only when certificate is untrusted. */
                                      : "Loading page cancelled because of untrusted connection.");
                    client->didFail(handle, error);
                }
                g_free(uri);
#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGI("[Network] gotHeadersCallback certificate !shouldContinue");
#endif
                return;
            }
        }
        g_free(uri);
    }
#endif /* PLATFORM (EFL) && USE(SOUP) */
#endif /* TIZEN_CERTIFICATE_HANDLING */

    // The original response will be needed later to feed to willSendRequest in
    // restartedCallback() in case we are redirected. For this reason, so we store it
    // here.
    ResourceResponse response;
    response.updateFromSoupMessage(msg);

    d->m_response = response;
}

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
static void applyAuthenticationToRequest(ResourceHandle* handle, ResourceRequest& request, bool redirect)
{
    // m_user/m_pass are credentials given manually, for instance, by the arguments passed to XMLHttpRequest.open().
    ResourceHandleInternal* d = handle->getInternal();

    if (handle->shouldUseCredentialStorage()) {
        if (d->m_user.isEmpty() && d->m_pass.isEmpty())
            d->m_initialCredential = CredentialStorage::get(request.url());
        else if (!redirect) {
            // If there is already a protection space known for the URL, update stored credentials
            // before sending a request. This makes it possible to implement logout by sending an
            // XMLHttpRequest with known incorrect credentials, and aborting it immediately (so that
            // an authentication dialog doesn't pop up).
            CredentialStorage::set(Credential(d->m_user, d->m_pass, CredentialPersistenceNone), request.url());
        }
    }

    String user = d->m_user;
    String password = d->m_pass;
    if (!d->m_initialCredential.isEmpty()) {
        user = d->m_initialCredential.user();
        password = d->m_initialCredential.password();
    }

    if (user.isEmpty() && password.isEmpty())
        return;

    // We always put the credentials into the URL. In the CFNetwork-port HTTP family credentials are applied in
    // the didReceiveAuthenticationChallenge callback, but libsoup requires us to use this method to override
    // any previously remembered credentials. It has its own per-session credential storage.
    KURL urlWithCredentials(request.url());
    urlWithCredentials.setUser(user);
    urlWithCredentials.setPass(password);
    request.setURL(urlWithCredentials);
}
#endif

// Called each time the message is going to be sent again except the first time.
// It's used mostly to let webkit know about redirects.
static void restartedCallback(SoupMessage* msg, gpointer data)
{
#if ENABLE(TIZEN_RESTARTED_CALLBACK_BUG_FIX)
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
#else
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
#endif
    if (!handle) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] restartedCallback !handle");
#endif
        return;
    }
    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_cancelled) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] restartedCallback d->m_cancelled");
#endif
        return;
    }

    GOwnPtr<char> uri(soup_uri_to_string(soup_message_get_uri(msg), false));
    String location = String::fromUTF8(uri.get());
#if ENABLE(TIZEN_REDIRECTED_LOCATION_IS_NOT_UTF_8)
    if (location.isEmpty())
        location = String(uri.get());
#endif
    KURL newURL = KURL(handle->firstRequest().url(), location);

#if ENABLE(TIZEN_EXTERNAL_URL_CONTAINING_USERNAME_PASSWORD)
    SoupURI * soupUri = soup_message_get_uri(msg);
    if(soupUri->password)
        newURL.setPass(String(soupUri->password));
#endif

    ResourceRequest request = handle->firstRequest();
    request.setURL(newURL);
    request.setHTTPMethod(msg->method);
#if ENABLE(TIZEN_CLEAR_HTTPBODY_IN_POST_TO_GET_REDIRECTION)
    if (!(request.httpMethod() == "POST" || request.httpMethod() == "PUT"))
        request.setHTTPBody(0);
#endif
    // Should not set Referer after a redirect from a secure resource to non-secure one.
    if (!request.url().protocolIs("https") && protocolIs(request.httpReferrer(), "https")) {
        request.clearHTTPReferrer();
        soup_message_headers_remove(msg->request_headers, "Referer");
    }

    if (d->client())
#if ENABLE(TIZEN_RESTARTED_CALLBACK_BUG_FIX)
        d->client()->willSendRequest(handle.get(), request, d->m_response);
#else
        d->client()->willSendRequest(handle, request, d->m_response);
#endif

    if (d->m_cancelled) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] restartedCallback d->m_cancelled after willSendRequest");
#endif
        return;
    }

#if ENABLE(WEB_TIMING)
    d->m_response.setResourceLoadTiming(ResourceLoadTiming::create());
    d->m_response.resourceLoadTiming()->requestTime = monotonicallyIncreasingTime();
#endif

    // Update the first party in case the base URL changed with the redirect
    String firstPartyString = request.firstPartyForCookies().string();
    if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(d->m_soupMessage.get(), firstParty.get());
    }
}

static void wroteBodyDataCallback(SoupMessage*, SoupBuffer* buffer, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;

    ASSERT(buffer);
    ResourceHandleInternal* internal = handle->getInternal();
    internal->m_bodyDataSent += buffer->length;

    if (internal->m_cancelled)
        return;
    ResourceHandleClient* client = handle->client();
    if (!client)
        return;

    client->didSendData(handle.get(), internal->m_bodyDataSent, internal->m_bodySize);
}

static void cleanupSoupRequestOperation(ResourceHandle* handle, bool isDestroying = false)
{
    ResourceHandleInternal* d = handle->getInternal();

    if (d->m_soupRequest)
        d->m_soupRequest.clear();

    if (d->m_inputStream)
        d->m_inputStream.clear();

    d->m_cancellable.clear();

    if (d->m_soupMessage) {
        g_signal_handlers_disconnect_matched(d->m_soupMessage.get(), G_SIGNAL_MATCH_DATA,
                                             0, 0, 0, 0, handle);
        g_object_set_data(G_OBJECT(d->m_soupMessage.get()), "handle", 0);
        d->m_soupMessage.clear();
    }

    if (d->m_buffer) {
#if ENABLE(TIZEN_EXPONENTIAL_BUFFER_SIZE)
        g_slice_free1(d->m_bufferSize, d->m_buffer);
#else
        g_slice_free1(READ_BUFFER_SIZE, d->m_buffer);
#endif
        d->m_buffer = 0;
    }

    if (d->m_timeoutSource) {
        g_source_destroy(d->m_timeoutSource.get());
        d->m_timeoutSource.clear();
    }

    if (!isDestroying)
        handle->deref();
}

static ResourceError convertSoupErrorToResourceError(GError* error, SoupRequest* request, SoupMessage* message = 0)
{
    ASSERT(error);
    ASSERT(request);

    GOwnPtr<char> uri(soup_uri_to_string(soup_request_get_uri(request), FALSE));
    if (message && SOUP_STATUS_IS_TRANSPORT_ERROR(message->status_code)) {
        return ResourceError(g_quark_to_string(SOUP_HTTP_ERROR),
                             static_cast<gint>(message->status_code),
                             uri.get(),
                             String::fromUTF8(message->reason_phrase));
    }

    // Non-transport errors are handled differently.
    return ResourceError(g_quark_to_string(G_IO_ERROR),
                         error->code,
                         uri.get(),
                         String::fromUTF8(error->message));
}

static void sendRequestCallback(GObject* source, GAsyncResult* res, gpointer data)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup sendRequestCallback\n");
#endif
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    ResourceHandleInternal* d = handle->getInternal();
    ResourceHandleClient* client = handle->client();
    SoupMessage* soupMessage = d->m_soupMessage.get();

    if (d->m_cancelled || !client) {
        cleanupSoupRequestOperation(handle.get());
        TIZEN_LOGI("[Network] sendRequestCallback d->m_cancelled || !client");
        return;
    }

    if (d->m_defersLoading) {
        d->m_deferredResult = res;
        return;
    }

    GOwnPtr<GError> error;
    GInputStream* in = soup_request_send_finish(d->m_soupRequest.get(), res, &error.outPtr());
    if (error) {
#if ENABLE(TIZEN_SOUP_HANDLE_STATUS_CODE_5XX_AFTER_RECEIVING_HTTP_BODY)
        if (d->m_soupMessage && SOUP_STATUS_IS_SERVER_ERROR(soupMessage->status_code)) {
                if (!(soupMessage->response_body) || (soupMessage->response_body->length == 0)) {
                        SoupURI* uri = soup_request_get_uri(d->m_soupRequest.get());
                        GOwnPtr<char> uriStr(soup_uri_to_string(uri, false));
                        gint errorCode = static_cast<gint>(soupMessage->status_code);
                        const gchar* errorMsg = soupMessage->reason_phrase;
                        const gchar* quarkStr = g_quark_to_string(SOUP_HTTP_ERROR);
                        ResourceError resourceError(quarkStr, errorCode, uriStr.get(), String::fromUTF8(errorMsg));

                        cleanupSoupRequestOperation(handle.get());
                        client->didFail(handle.get(), resourceError);
                        TIZEN_LOGI("[Network] sendRequestCallback SOUP_STATUS_IS_SERVER_ERROR");
                        return;
                }
        }
#endif

        TIZEN_SECURE_LOGI("[Network] sendRequestCallback error [%s]", error->message);
#if ENABLE(TIZEN_COMPRESSION_PROXY)
        if(handle->firstRequest().isMainFrameRequest() && ((error->code == 4) || (error->code == 7)) ) {
                SoupURI *uri = soup_message_get_uri(soupMessage);
                GHashTable * proxy_failure = ResourceHandle::get_proxy_failure_table();
                if(proxy_failure) {
                        gboolean remove = FALSE;
                        guint32 value = (guint32)g_hash_table_lookup(proxy_failure, GUINT_TO_POINTER(g_str_hash(soup_uri_get_host (uri))) );
                        TIZEN_LOGI("value %d proxy_failure %p", value, proxy_failure);
                        remove = g_hash_table_remove(proxy_failure, GUINT_TO_POINTER(g_str_hash(soup_uri_get_host (uri))) );
                        TIZEN_LOGI("[Network]removed domain %s result %d ", soup_uri_get_host (uri), remove);
                        TIZEN_LOGI("HASH table size %d", g_hash_table_size (proxy_failure));
                        if(g_hash_table_size (proxy_failure) == 0) {
                                g_hash_table_destroy(proxy_failure);
                                ResourceHandle::set_proxy_failure_table(NULL);
                                TIZEN_LOGI("[Network] HASH table destroyed");
                        }
                }
        }
#endif
        client->didFail(handle.get(), convertSoupErrorToResourceError(error.get(), d->m_soupRequest.get(), soupMessage));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

#if ENABLE(TIZEN_SOUP_NOT_ALLOW_BROWSE_LOCAL_DIRECTORY)
    KURL url = handle->firstRequest().url();
    if (url.isLocalFile() && g_file_test(url.path().utf8().data(), G_FILE_TEST_IS_DIR)) {
        TIZEN_LOGI("[Network] sendRequestCallback local directory is not allowed to browse.");
        gint errorCode = SOUP_STATUS_IO_ERROR;
        const gchar* quarkStr = g_quark_to_string(SOUP_HTTP_ERROR);
        ResourceError resourceError(quarkStr, errorCode, url.string(), String::fromUTF8("Is a directory"));
        cleanupSoupRequestOperation(handle.get());
        client->didFail(handle.get(), resourceError);
        return;
    }
#endif

    d->m_inputStream = adoptGRef(in);
#if ENABLE(TIZEN_EXPONENTIAL_BUFFER_SIZE)
    d->m_buffer = static_cast<char*>(g_slice_alloc(READ_BUFFER_SIZE_MIN));
    d->m_bufferSize = READ_BUFFER_SIZE_MIN;
#else
    d->m_buffer = static_cast<char*>(g_slice_alloc(READ_BUFFER_SIZE));
#endif
    if (soupMessage) {
        if (handle->shouldContentSniff() && soupMessage->status_code != SOUP_STATUS_NOT_MODIFIED) {
            const char* sniffedType = soup_request_get_content_type(d->m_soupRequest.get());
            d->m_response.setSniffedContentType(sniffedType);
        }
        d->m_response.updateFromSoupMessage(soupMessage);
    } else {
        d->m_response.setURL(handle->firstRequest().url());
        const gchar* contentType = soup_request_get_content_type(d->m_soupRequest.get());
        d->m_response.setMimeType(extractMIMETypeFromMediaType(contentType));
        d->m_response.setTextEncodingName(extractCharsetFromMediaType(contentType));
        d->m_response.setExpectedContentLength(soup_request_get_content_length(d->m_soupRequest.get()));
    }

#if ENABLE(TIZEN_SET_AUTHORIZATION_TO_RESOURCE_REQUEST_FROM_SOUP_MESSAGE)
    if (soupMessage) {
        const char *auth = soup_message_headers_get_one(soupMessage->request_headers, "Authorization");
        if (auth) {
            if (!handle->firstRequest().httpHeaderField("Authorization").isEmpty())
                handle->firstRequest().clearHTTPAuthorization();
            handle->firstRequest().addHTTPHeaderField("Authorization", auth);
        }
    }
#endif

    client->didReceiveResponse(handle.get(), d->m_response);

    if (d->m_cancelled) {
        cleanupSoupRequestOperation(handle.get());
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] sendRequestCallback d->m_cancelled after didReceiveResponse");
#endif
        return;
    }
#if ENABLE(TIZEN_EXPONENTIAL_BUFFER_SIZE)
    g_input_stream_read_async(d->m_inputStream.get(), d->m_buffer, d->m_bufferSize,
                              G_PRIORITY_DEFAULT, d->m_cancellable.get(), readCallback, handle.get());
#else
    g_input_stream_read_async(d->m_inputStream.get(), d->m_buffer, READ_BUFFER_SIZE,
                               G_PRIORITY_DEFAULT, d->m_cancellable.get(), readCallback, handle.get());
#endif
}

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
static void authenticatedCallback(SoupMessage* message, SoupAuth* auth, gboolean, gpointer data)
{
    LOG(Network,"[Network] ResourceHandleSoup authenticatedCallback\n");

    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] authenticatedCallback !handle");
#endif
        return;
    }

    ResourceHandleInternal* d = handle->getInternal();
    if (!d) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] authenticatedCallback !d");
#endif
        return;
    }

    SoupSession* session = handle->defaultSession();

    const char* realm = soup_auth_get_realm(auth);
    SoupURI* uri = soup_message_get_uri(message);
    const char* host = soup_uri_get_host(uri);
    unsigned int port = soup_uri_get_port(uri);
    const char* protocol = soup_uri_get_scheme(uri);
    const char* scheme = soup_auth_get_scheme_name(auth);

    // FIXME : should consider ProtectionSpaceProxy type
    ProtectionSpaceServerType serverType;
    if (equalIgnoringCase(String::fromUTF8(protocol), "http"))
        serverType = ProtectionSpaceServerHTTP;
    else if (equalIgnoringCase(String::fromUTF8(protocol), "https"))
        serverType = ProtectionSpaceServerHTTPS;
    else {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Network] authenticatedCallback not supported protocol [%s]", protocol);
#endif
        notImplemented();
        return;
    }

    // FIXME : should consider other authenticationScheme
    ProtectionSpaceAuthenticationScheme authenticationScheme = ProtectionSpaceAuthenticationSchemeDefault;
    if (serverType == ProtectionSpaceServerHTTP || serverType == ProtectionSpaceServerHTTPS) {
        if (equalIgnoringCase(String::fromUTF8(scheme), "basic"))
            authenticationScheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
        else if (equalIgnoringCase(String::fromUTF8(scheme), "digest"))
            authenticationScheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
        else {
            TIZEN_LOGI("[Network] ResourceHandleSoup authenticatedCallback authenticationScheme [%s] is not handled.", scheme);
            notImplemented();
            return;
        }
    } else {
        TIZEN_LOGI("[Network] ResourceHandleSoup authenticatedCallback serverType [%d] is not handled.", serverType);
        notImplemented();
        return;
    }

    d->m_soupAuth = auth;
    soup_session_pause_message(session, message);

    ProtectionSpace protectionSpace(WTF::String::fromUTF8(host), port, serverType, WTF::String::fromUTF8(realm), authenticationScheme);
    Credential credential = d->m_initialCredential;
    handle->didReceiveAuthenticationChallenge(AuthenticationChallenge(protectionSpace, credential, 0, d->m_response, ResourceError(), handle));
}
#endif

static bool addFileToSoupMessageBody(SoupMessage* message, const String& fileNameString, size_t offset, size_t lengthToSend, unsigned long& totalBodySize)
{
    GOwnPtr<GError> error;
    CString fileName = fileSystemRepresentation(fileNameString);
    GMappedFile* fileMapping = g_mapped_file_new(fileName.data(), false, &error.outPtr());
    if (error)
        return false;

    gsize bufferLength = lengthToSend;
    if (!lengthToSend)
        bufferLength = g_mapped_file_get_length(fileMapping);
    totalBodySize += bufferLength;

    SoupBuffer* soupBuffer = soup_buffer_new_with_owner(g_mapped_file_get_contents(fileMapping) + offset,
                                                        bufferLength,
                                                        fileMapping,
                                                        reinterpret_cast<GDestroyNotify>(g_mapped_file_unref));
    soup_message_body_append_buffer(message->request_body, soupBuffer);
    soup_buffer_free(soupBuffer);
    return true;
}

#if ENABLE(BLOB)
static bool blobIsOutOfDate(const BlobDataItem& blobItem)
{
    ASSERT(blobItem.type == BlobDataItem::File);
    if (!isValidFileTime(blobItem.expectedModificationTime))
        return false;

    time_t fileModificationTime;
    if (!getFileModificationTime(blobItem.path, fileModificationTime))
        return true;

    return fileModificationTime != static_cast<time_t>(blobItem.expectedModificationTime);
}

static void addEncodedBlobItemToSoupMessageBody(SoupMessage* message, const BlobDataItem& blobItem, unsigned long& totalBodySize)
{
    if (blobItem.type == BlobDataItem::Data) {
        totalBodySize += blobItem.length;
        soup_message_body_append(message->request_body, SOUP_MEMORY_TEMPORARY,
                                 blobItem.data->data() + blobItem.offset, blobItem.length);
        return;
    }

    ASSERT(blobItem.type == BlobDataItem::File);
    if (blobIsOutOfDate(blobItem))
        return;

    addFileToSoupMessageBody(message,
                             blobItem.path,
                             blobItem.offset,
                             blobItem.length == BlobDataItem::toEndOfFile ? 0 : blobItem.length,
                             totalBodySize);
}

static void addEncodedBlobToSoupMessageBody(SoupMessage* message, const FormDataElement& element, unsigned long& totalBodySize)
{
    RefPtr<BlobStorageData> blobData = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(KURL(ParsedURLString, element.m_blobURL));
    if (!blobData)
        return;

    for (size_t i = 0; i < blobData->items().size(); ++i)
        addEncodedBlobItemToSoupMessageBody(message, blobData->items()[i], totalBodySize);
}
#endif // ENABLE(BLOB)

static bool addFormElementsToSoupMessage(SoupMessage* message, const char* contentType, FormData* httpBody, unsigned long& totalBodySize)
{
    soup_message_body_set_accumulate(message->request_body, FALSE);
    size_t numElements = httpBody->elements().size();
    for (size_t i = 0; i < numElements; i++) {
        const FormDataElement& element = httpBody->elements()[i];

        if (element.m_type == FormDataElement::data) {
            totalBodySize += element.m_data.size();
            soup_message_body_append(message->request_body, SOUP_MEMORY_TEMPORARY,
                                     element.m_data.data(), element.m_data.size());
            continue;
        }

        if (element.m_type == FormDataElement::encodedFile) {
            if (!addFileToSoupMessageBody(message ,
                                         element.m_filename,
                                         0 /* offset */,
                                         0 /* lengthToSend */,
                                         totalBodySize))
                return false;
            continue;
        }

#if ENABLE(BLOB)
        ASSERT(element.m_type == FormDataElement::encodedBlob);
        addEncodedBlobToSoupMessageBody(message, element, totalBodySize);
#endif
    }
    return true;
}

#if ENABLE(WEB_TIMING)
static int milisecondsSinceRequest(double requestTime)
{
    return static_cast<int>((monotonicallyIncreasingTime() - requestTime) * 1000.0);
}

static void wroteBodyCallback(SoupMessage*, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;

    ResourceHandleInternal* d = handle->getInternal();
    if (!d->m_response.resourceLoadTiming())
        return;

    d->m_response.resourceLoadTiming()->sendEnd = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
}

static void requestStartedCallback(SoupSession*, SoupMessage* soupMessage, SoupSocket*, gpointer data)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup requestStartedCallback\n");
#endif

    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(g_object_get_data(G_OBJECT(soupMessage), "handle"));
    if (!handle)
        return;

    ResourceHandleInternal* d = handle->getInternal();
    if (!d->m_response.resourceLoadTiming())
        return;

    d->m_response.resourceLoadTiming()->sendStart = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
    if (d->m_response.resourceLoadTiming()->sslStart != -1) {
        // WebCore/inspector/front-end/RequestTimingView.js assumes
        // that SSL time is included in connection time so must
        // substract here the SSL delta that will be added later (see
        // WebInspector.RequestTimingView.createTimingTable in the
        // file above for more details).
        d->m_response.resourceLoadTiming()->sendStart -=
            d->m_response.resourceLoadTiming()->sslEnd - d->m_response.resourceLoadTiming()->sslStart;
    }
}

static void networkEventCallback(SoupMessage*, GSocketClientEvent event, GIOStream*, gpointer data)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup networkEventCallback\n");
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;
    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_cancelled)
        return;

    int deltaTime = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
    switch (event) {
    case G_SOCKET_CLIENT_RESOLVING:
        d->m_response.resourceLoadTiming()->dnsStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_RESOLVED:
        d->m_response.resourceLoadTiming()->dnsEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_CONNECTING:
        d->m_response.resourceLoadTiming()->connectStart = deltaTime;
        if (d->m_response.resourceLoadTiming()->dnsStart != -1)
            // WebCore/inspector/front-end/RequestTimingView.js assumes
            // that DNS time is included in connection time so must
            // substract here the DNS delta that will be added later (see
            // WebInspector.RequestTimingView.createTimingTable in the
            // file above for more details).
            d->m_response.resourceLoadTiming()->connectStart -=
                d->m_response.resourceLoadTiming()->dnsEnd - d->m_response.resourceLoadTiming()->dnsStart;
        break;
    case G_SOCKET_CLIENT_CONNECTED:
        // Web Timing considers that connection time involves dns, proxy & TLS negotiation...
        // so we better pick G_SOCKET_CLIENT_COMPLETE for connectEnd
        break;
    case G_SOCKET_CLIENT_PROXY_NEGOTIATING:
        d->m_response.resourceLoadTiming()->proxyStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_PROXY_NEGOTIATED:
        d->m_response.resourceLoadTiming()->proxyEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_TLS_HANDSHAKING:
        d->m_response.resourceLoadTiming()->sslStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_TLS_HANDSHAKED:
        d->m_response.resourceLoadTiming()->sslEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_COMPLETE:
        d->m_response.resourceLoadTiming()->connectEnd = deltaTime;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}
#endif

static const char* gSoupRequestInitiaingPageIDKey = "wk-soup-request-initiaing-page-id";

static void setSoupRequestInitiaingPageID(SoupRequest* request, uint64_t initiatingPageID)
{
    if (!initiatingPageID)
        return;

    uint64_t* initiatingPageIDPtr = static_cast<uint64_t*>(fastMalloc(sizeof(uint64_t)));
    *initiatingPageIDPtr = initiatingPageID;
    g_object_set_data_full(G_OBJECT(request), g_intern_static_string(gSoupRequestInitiaingPageIDKey), initiatingPageIDPtr, fastFree);
}

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
static bool startInterceptRequest(ResourceHandle *handle)
{
    ASSERT(handle);
    ResourceHandleInternal* d = handle->getInternal();

    ResourceRequest request(handle->firstRequest());
    KURL url(request.url());
    url.removeFragmentIdentifier();
    request.setURL(url);

    ResourceHandleClient *client = handle->client();
    bool interceptResponse = false;
    if (client) {
        SoupMessage* soupMessage = soup_message_new(request.httpMethod().utf8().data(), url.string().utf8().data());
        if (!soupMessage)
            return false;

        FormData* httpBody = request.httpBody();
        unsigned long bodySize;
        const char* body = NULL;
        int bodyLength = 0;

        if (httpBody && !httpBody->isEmpty()
            && !addFormElementsToSoupMessage(soupMessage, NULL, httpBody, bodySize)) {
            //TODO : Delete soupMessage
            TIZEN_LOGI("ResourceHandleSoup startHTTPRequest fail to handle httpBody");
            return false;
        }

        String accept = d->m_firstRequest.httpHeaderField("Accept");
        if (accept.isEmpty())
            d->m_firstRequest.setHTTPHeaderField("Accept", "*/*");

#if ENABLE(TIZEN_HTTP_REQUEST_HEADER_APPEND)
        String acceptCharset = d->m_firstRequest.httpHeaderField("Accept-Charset");
        if (acceptCharset.isEmpty())
            d->m_firstRequest.setHTTPHeaderField("Accept-Charset", "iso-8859-1, utf-8, utf-16, *;q=0.1");

        String xWap = d->m_firstRequest.httpHeaderField("-Proxy-Cookie");
        if (xWap.isEmpty())
            d->m_firstRequest.setHTTPHeaderField("x-Wap-Proxy-Cookie", "none");
#endif

        String acceptEncoding = d->m_firstRequest.httpHeaderField("Accept-Encoding");
        if (acceptEncoding.isEmpty())
            d->m_firstRequest.setHTTPHeaderField("Accept-Encoding", "gzip, deflate");

        String connetion = d->m_firstRequest.httpHeaderField("Connection");
        if (connetion.isEmpty())
            d->m_firstRequest.setHTTPHeaderField("Connection", "Keep-Alive");

        soup_message_body_set_accumulate(soupMessage->request_body, TRUE);
        SoupBuffer* soupBuffer = soup_message_body_flatten(soupMessage->request_body);
        if (soupBuffer)
            soup_buffer_get_data(soupBuffer, (const guint8**)&body, (gsize*)&bodyLength);
        handle->ref();
        client->shouldInterceptRequest(handle, body);
        TIZEN_SECURE_LOGI ("[INTERCEPT] Intercept URL: %s, response: %d", url.string().utf8().data(), interceptResponse);
        soup_buffer_free(soupBuffer);
        g_object_unref(soupMessage);

        //TODO : Cleanup
        return true;
    }
    return false;
}
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
static bool startHTTPRequest(ResourceHandle* handle, ResourceRequest& request)
#else
static bool startHTTPRequest(ResourceHandle* handle)
#endif
{
    ASSERT(handle);

    ResourceHandleInternal* d = handle->getInternal();

    SoupSession* session = d->soupSession();
    ensureSessionIsInitialized(session);
    SoupRequester* requester = SOUP_REQUESTER(soup_session_get_feature(session, SOUP_TYPE_REQUESTER));
#if !ENABLE(TIZEN_COMPRESSION_PROXY)
    ResourceRequest request(handle->firstRequest());
#endif
    KURL url(request.url());
    url.removeFragmentIdentifier();
    request.setURL(url);

#if ENABLE(TIZEN_LOG)
    // Never print URI itself. It can import non-printable characters.
    LOG(Network,"[Network] ResourceHandleSoup startHTTPRequest host [%s] \n", url.host().utf8().data());
#endif

    GOwnPtr<GError> error;
    d->m_soupRequest = adoptGRef(soup_requester_request(requester, url.string().utf8().data(), &error.outPtr()));
    if (error) {
        TIZEN_SECURE_LOGI("[Network] startHTTPRequest error [%s] ", error->message);
        d->m_soupRequest = 0;
        return false;
    }

    setSoupRequestInitiaingPageID(d->m_soupRequest.get(), d->initiatingPageID());

    d->m_soupMessage = adoptGRef(soup_request_http_get_message(SOUP_REQUEST_HTTP(d->m_soupRequest.get())));
    if (!d->m_soupMessage) {
        TIZEN_LOGI("[Network] startHTTPRequest error from soup_request_http_get_message");
        return false;
    }

    SoupMessage* soupMessage = d->m_soupMessage.get();
    request.updateSoupMessage(soupMessage);

#if USE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup startHTTPRequest : Register callbacks\n");
#endif
    if (!handle->shouldContentSniff())
        soup_message_disable_feature(soupMessage, SOUP_TYPE_CONTENT_SNIFFER);

    g_signal_connect(soupMessage, "got-headers", G_CALLBACK(gotHeadersCallback), handle);
    g_signal_connect(soupMessage, "restarted", G_CALLBACK(restartedCallback), handle);
    g_signal_connect(soupMessage, "wrote-body-data", G_CALLBACK(wroteBodyDataCallback), handle);
#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    g_signal_connect(soupMessage, "authenticate", G_CALLBACK(authenticatedCallback), handle);
#endif

#if ENABLE(WEB_TIMING)
    g_signal_connect(soupMessage, "network-event", G_CALLBACK(networkEventCallback), handle);
    g_signal_connect(soupMessage, "wrote-body", G_CALLBACK(wroteBodyCallback), handle);
    g_object_set_data(G_OBJECT(soupMessage), "handle", handle);
#endif

    String firstPartyString = request.firstPartyForCookies().string();
    if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(soupMessage, firstParty.get());
    }

#if ENABLE(TIZEN_COMPRESSION_PROXY)
    FormData* httpBody = request.httpBody();
    CString contentType = request.httpContentType().utf8().data();
#else
    FormData* httpBody = d->m_firstRequest.httpBody();
    CString contentType = d->m_firstRequest.httpContentType().utf8().data();
#endif
    if (httpBody && !httpBody->isEmpty()
        && !addFormElementsToSoupMessage(soupMessage, contentType.data(), httpBody, d->m_bodySize)) {
        // We failed to prepare the body data, so just fail this load.
        g_signal_handlers_disconnect_matched(soupMessage, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, handle);
        d->m_soupMessage.clear();
        TIZEN_LOGI("[Network] startHTTPRequest fail to handle httpBody");
        return false;
    }

    // balanced by a deref() in cleanupSoupRequestOperation, which should always run
    handle->ref();

    setSoupRequestInitiaingPageID(d->m_soupRequest.get(), d->initiatingPageID());

#if ENABLE(WEB_TIMING)
    d->m_response.setResourceLoadTiming(ResourceLoadTiming::create());
#endif

    // Make sure we have an Accept header for subresources; some sites
    // want this to serve some of their subresources
    if (!soup_message_headers_get_one(soupMessage->request_headers, "Accept"))
        soup_message_headers_append(soupMessage->request_headers, "Accept", "*/*");

#if ENABLE(TIZEN_HTTP_REQUEST_HEADER_APPEND)
    if (!soup_message_headers_get_one(soupMessage->request_headers, "Accept-Charset"))
        soup_message_headers_append(soupMessage->request_headers, "Accept-Charset", "iso-8859-1, utf-8, utf-16, *;q=0.1");

    if (!soup_message_headers_get_one(soupMessage->request_headers, "x-Wap-Proxy-Cookie"))
        soup_message_headers_append(soupMessage->request_headers, "x-Wap-Proxy-Cookie", "none");
#endif

    // In the case of XHR .send() and .send("") explicitly tell libsoup
    // to send a zero content-lenght header for consistency
    // with other backends (e.g. Chromium's) and other UA implementations like FF.
    // It's done in the backend here instead of in XHR code since in XHR CORS checking
    // prevents us from this kind of late header manipulation.
    if ((request.httpMethod() == "POST" || request.httpMethod() == "PUT")
        && (!request.httpBody() || request.httpBody()->isEmpty()))
        soup_message_headers_set_content_length(soupMessage->request_headers, 0);

#if ENABLE(TIZEN_EXTERNAL_URL_CONTAINING_USERNAME_PASSWORD)
    if(!url.user().isEmpty() && !url.pass().isEmpty()) {
        d->m_soupAuth = soup_auth_new(SOUP_TYPE_AUTH_BASIC, soupMessage, "Basic");
        soup_auth_authenticate(d->m_soupAuth.get(), url.user().utf8().data(), url.pass().utf8().data());
    }
#endif

    // Send the request only if it's not been explicitly deferred.
    if (!d->m_defersLoading) {
#if ENABLE(WEB_TIMING)
        d->m_response.resourceLoadTiming()->requestTime = monotonicallyIncreasingTime();
#endif
#if ENABLE(TIZEN_COMPRESSION_PROXY)
        if (request.timeoutInterval() > 0) {
            // soup_add_timeout returns a GSource* whose only reference is owned by the context. We need to have our own reference to it, hence not using adoptRef.
            d->m_timeoutSource = soup_add_timeout(g_main_context_get_thread_default(), request.timeoutInterval() * 1000, requestTimeoutCallback, handle);
        }
#else
        if (d->m_firstRequest.timeoutInterval() > 0) {
            // soup_add_timeout returns a GSource* whose only reference is owned by the context. We need to have our own reference to it, hence not using adoptRef.
            d->m_timeoutSource = soup_add_timeout(g_main_context_get_thread_default(), d->m_firstRequest.timeoutInterval() * 1000, requestTimeoutCallback, handle);
        }
#endif
        d->m_cancellable = adoptGRef(g_cancellable_new());
#if ENABLE(TIZEN_LOG)
        LOG(Network,"[Network] ResourceHandleSoup startHTTPRequest : Invoke soup_request_send_async\n");
#endif

#if ENABLE(TIZEN_SET_SOUP_MESSAGE_USING_SYNC_CONTEXT)
        if (loadingSynchronousRequest)
#ifdef SOUP_MESSAGE_USE_SYNC_CONTEXT
            g_object_set(soupMessage, SOUP_MESSAGE_USE_SYNC_CONTEXT, TRUE, NULL);
#else
            LOG(Network,"SOUP_MESSAGE_USE_SYNC_CONTEXT is not defined");
#endif
#endif
        soup_request_send_async(d->m_soupRequest.get(), d->m_cancellable.get(), sendRequestCallback, handle);
    }

    return true;
}

bool ResourceHandle::start(NetworkingContext* context)
{
    ASSERT(!d->m_soupMessage);

    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an unload handler, so let's just block it.
    // If both the frame and the page are not null the context is valid.
    if (context && !context->isValid()) {
        TIZEN_LOGI("[Network] !context->isValid()");
        return false;
    }

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    applyAuthenticationToRequest(this, firstRequest(), false);
#else
    if (!(d->m_user.isEmpty() || d->m_pass.isEmpty())) {
        // If credentials were specified for this request, add them to the url,
        // so that they will be passed to NetworkRequest.
        KURL urlWithCredentials(firstRequest().url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        d->m_firstRequest.setURL(urlWithCredentials);
    }
#endif

    KURL url = firstRequest().url();
    String urlString = url.string();
    String protocol = url.protocol();
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    ResourceRequest& request = firstRequest();
#endif
    // Used to set the authentication dialog toplevel; may be NULL
    d->m_context = context;

#if ENABLE(TIZEN_WRT_APP_URI_SCHEME)
    // If the alternativeURL is not empty, it means url and protocol were changed from app:// to data:// or file://, so just replace url and protocol and go on next step,
    if ((!firstRequest().isAlternativeUrlEmpty()) && (equalIgnoringCase(firstRequest().alternativeURL().protocol(), "file") || equalIgnoringCase(firstRequest().alternativeURL().protocol(), "data"))) {
        url = firstRequest().alternativeURL();
        protocol = url.protocol();
    }
#endif

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
    if (context && context->interceptRequestEnabled() && client()->shouldUseInterceptRequest()) {
        if (startInterceptRequest(this))
            return true;
    }
#endif

    if (equalIgnoringCase(protocol, "http") || equalIgnoringCase(protocol, "https")) {
#if ENABLE(TIZEN_COMPRESSION_PROXY)
        if (startHTTPRequest(this, request))
#else
        if (startHTTPRequest(this))
#endif
            return true;
    }

    if (startNonHTTPRequest(this, url))
        return true;

    // Error must not be reported immediately
    this->scheduleFailure(InvalidURLFailure);

    return true;
}

#if ENABLE(TIZEN_COMPRESSION_PROXY)
void ResourceHandle::cancelSoup()
#else
void ResourceHandle::cancel()
#endif
{
#if USE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandle::cancel()\n");
#endif
    d->m_cancelled = true;
    if (d->m_soupMessage)
        soup_session_cancel_message(d->soupSession(), d->m_soupMessage.get(), SOUP_STATUS_CANCELLED);
    else if (d->m_cancellable)
        g_cancellable_cancel(d->m_cancellable.get());
}

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}
#endif

static bool hasBeenSent(ResourceHandle* handle)
{
    ResourceHandleInternal* d = handle->getInternal();

    return d->m_cancellable;
}

void ResourceHandle::platformSetDefersLoading(bool defersLoading)
{
    if (d->m_cancelled)
        return;

    // Except when canceling a possible timeout timer, we only need to take action here to UN-defer loading.
    if (defersLoading) {
        if (d->m_timeoutSource) {
            g_source_destroy(d->m_timeoutSource.get());
            d->m_timeoutSource.clear();
        }
        return;
    }

    // We need to check for d->m_soupRequest because the request may
    // have raised a failure (for example invalid URLs). We cannot
    // simply check for d->m_scheduledFailure because it's cleared as
    // soon as the failure event is fired.
    if (!hasBeenSent(this) && d->m_soupRequest) {
#if ENABLE(WEB_TIMING)
        if (d->m_response.resourceLoadTiming())
            d->m_response.resourceLoadTiming()->requestTime = monotonicallyIncreasingTime();
#endif
        d->m_cancellable = adoptGRef(g_cancellable_new());
        if (d->m_firstRequest.timeoutInterval() > 0) {
            // soup_add_timeout returns a GSource* whose only reference is owned by the context. We need to have our own reference to it, hence not using adoptRef.
            d->m_timeoutSource = soup_add_timeout(g_main_context_get_thread_default(), d->m_firstRequest.timeoutInterval() * 1000, requestTimeoutCallback, this);
        }
        soup_request_send_async(d->m_soupRequest.get(), d->m_cancellable.get(), sendRequestCallback, this);
        return;
    }

    if (d->m_deferredResult) {
        GRefPtr<GAsyncResult> asyncResult = adoptGRef(d->m_deferredResult.leakRef());

        if (d->m_inputStream)
            readCallback(G_OBJECT(d->m_inputStream.get()), asyncResult.get(), this);
        else
            sendRequestCallback(G_OBJECT(d->m_soupRequest.get()), asyncResult.get(), this);
    }
}

bool ResourceHandle::loadsBlocked()
{
    return false;
}

bool ResourceHandle::willLoadFromCache(ResourceRequest&, Frame*)
{
    // Not having this function means that we'll ask the user about re-posting a form
    // even when we go back to a page that's still in the cache.
    notImplemented();
    return false;
}

void ResourceHandle::loadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials /*storedCredentials*/, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
#if ENABLE(BLOB)
    if (request.url().protocolIs("blob")) {
        blobRegistry().loadResourceSynchronously(request, error, response, data);
        return;
    }
#endif

    ASSERT(!loadingSynchronousRequest);
    if (loadingSynchronousRequest) // In practice this cannot happen, but if for some reason it does,
        return;                    // we want to avoid accidentally going into an infinite loop of requests.

    WebCoreSynchronousLoader syncLoader(error, response, sessionFromContext(context), data);
    RefPtr<ResourceHandle> handle = create(context, request, &syncLoader, false /*defersLoading*/, false /*shouldContentSniff*/);
    if (!handle)
        return;

    // If the request has already failed, do not run the main loop, or else we'll block indefinitely.
    if (handle->d->m_scheduledFailureType != NoFailure)
        return;

    syncLoader.run();
}

static void closeCallback(GObject* source, GAsyncResult* res, gpointer data)
{
#if USE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandle closeCallback\n");
#endif
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = handle->getInternal();

    g_input_stream_close_finish(d->m_inputStream.get(), res, 0);

    ResourceHandleClient* client = handle->client();
    if (client && loadingSynchronousRequest)
        client->didFinishLoading(handle.get(), 0);

    cleanupSoupRequestOperation(handle.get());
}

static void readCallback(GObject* source, GAsyncResult* asyncResult, gpointer data)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup readCallback\n");
#endif
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    ResourceHandleInternal* d = handle->getInternal();
    ResourceHandleClient* client = handle->client();

    if (d->m_cancelled || !client) {
        TIZEN_LOGI("[Network] ResourceHandleSoup readCallback d->m_cancelled[%d], client[0x%x]", d->m_cancelled, client);
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (d->m_defersLoading) {
        d->m_deferredResult = asyncResult;
        return;
    }

    GOwnPtr<GError> error;
    gssize bytesRead = g_input_stream_read_finish(d->m_inputStream.get(), asyncResult, &error.outPtr());
    if (error) {
        TIZEN_SECURE_LOGI("[Network] ResourceHandleSoup readCallback error from g_input_stream_read_finish [%s]", error->message);
        client->didFail(handle.get(), convertSoupErrorToResourceError(error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (!bytesRead) {
#if ENABLE(TIZEN_LOG)
        LOG(Network,"[Network] ResourceHandleSoup readCallback finished. \n");
#endif
        // We inform WebCore of load completion now instead of waiting for the input
        // stream to close because the input stream is closed asynchronously. If this
        // is a synchronous request, we wait until the closeCallback, because we don't
        // want to halt the internal main loop before the input stream closes.
        if (client && !loadingSynchronousRequest) {
            client->didFinishLoading(handle.get(), 0);

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
            if (d->m_cancelled || !client) {
                TIZEN_LOGE("[Network] ResourceHandleSoup readCallback after client->didFinishLoading() d->m_cancelled[%d], client[0x%x]", d->m_cancelled, client);
                cleanupSoupRequestOperation(handle.get());
                return;
            }
#endif

           handle->setClient(0); // Unset the client so that we do not try to access the client in the closeCallback.
#if ENABLE(TIZEN_DESTROY_TIMEOUT_SOURCE)
           if (d->m_timeoutSource) {
               g_source_destroy(d->m_timeoutSource.get());
               d->m_timeoutSource.clear();
           }
#endif
        }
        g_input_stream_close_async(d->m_inputStream.get(), G_PRIORITY_DEFAULT, 0, closeCallback, handle.get());
        return;
    }

    // It's mandatory to have sent a response before sending data
    ASSERT(!d->m_response.isNull());

    client->didReceiveData(handle.get(), d->m_buffer, bytesRead, bytesRead);

    // didReceiveData may cancel the load, which may release the last reference.
    if (d->m_cancelled || !client) {
        TIZEN_LOGI("[Network] ResourceHandleSoup readCallback d->m_cancelled[%d], client[0x%x]", d->m_cancelled, client);
        cleanupSoupRequestOperation(handle.get());
        return;
    }

#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup readCallback : Invoke g_input_stream_read_async \n");
#endif

#if ENABLE(TIZEN_EXPONENTIAL_BUFFER_SIZE)
    if (equalIgnoringCase(d->m_response.mimeType(), "text/html")
        && d->m_bufferSize < READ_BUFFER_SIZE_MAX && bytesRead == static_cast<long>(d->m_bufferSize)) {

        g_slice_free1(d->m_bufferSize, d->m_buffer);
        d->m_buffer = 0;
        d->m_bufferSize *= 2;
        d->m_buffer = static_cast<char*>(g_slice_alloc(d->m_bufferSize));
    }
    g_input_stream_read_async(d->m_inputStream.get(), d->m_buffer, d->m_bufferSize, G_PRIORITY_DEFAULT,
                              d->m_cancellable.get(), readCallback, handle.get());
#else
    g_input_stream_read_async(d->m_inputStream.get(), d->m_buffer, READ_BUFFER_SIZE, G_PRIORITY_DEFAULT,
                               d->m_cancellable.get(), readCallback, handle.get());
#endif

}

static gboolean requestTimeoutCallback(gpointer data)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup requestTimeoutCallback \n");
#endif
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = handle->getInternal();
    ResourceHandleClient* client = handle->client();

    ResourceError timeoutError("WebKitNetworkError", gTimeoutError, d->m_firstRequest.url().string(), "Request timed out");
    timeoutError.setIsTimeout(true);
    client->didFail(handle.get(), timeoutError);
    handle->cancel();

    return FALSE;
}

static bool startNonHTTPRequest(ResourceHandle* handle, KURL url)
{
#if ENABLE(TIZEN_LOG)
    // Never print URI itself. It can import non-printable characters.
    LOG(Network,"[Network] ResourceHandleSoup startNonHTTPRequest host [%s] \n", url.host().utf8().data());
#endif
    ASSERT(handle);

    if (handle->firstRequest().httpMethod() != "GET" && handle->firstRequest().httpMethod() != "POST") {
        TIZEN_SECURE_LOGI("[Network] ResourceHandleSoup startNonHTTPRequest httpMethod [%s] [%s]", handle->firstRequest().httpMethod().utf8().data(), url.string().utf8().data());
        return false;
    }

    ResourceHandleInternal* d = handle->getInternal();

    SoupSession* session = d->soupSession();
    ensureSessionIsInitialized(session);
    SoupRequester* requester = SOUP_REQUESTER(soup_session_get_feature(session, SOUP_TYPE_REQUESTER));

#if ENABLE(FILE_SYSTEM)
    CString urlStr;

    if (url.protocolIs("filesystem")) {
        String filePath = "file://" + AsyncFileSystemTizen::virtualPathToFileSystemPath(url);
        urlStr = filePath.utf8();
    } else
        urlStr = url.string().utf8();
#else
    CString urlStr = url.string().utf8();
#endif

    GOwnPtr<GError> error;
    d->m_soupRequest = adoptGRef(soup_requester_request(requester, urlStr.data(), &error.outPtr()));
    if (error) {
        TIZEN_SECURE_LOGI("[Network] ResourceHandleSoup startNonHTTPRequest error from soup_requester_request [%s]", error->message);
        d->m_soupRequest = 0;
        return false;
    }

    // balanced by a deref() in cleanupSoupRequestOperation, which should always run
    handle->ref();

    // Send the request only if it's not been explicitly deferred.
    if (!d->m_defersLoading) {
        d->m_cancellable = adoptGRef(g_cancellable_new());
        if (d->m_firstRequest.timeoutInterval() > 0) {
            // soup_add_timeout returns a GSource* whose only reference is owned by the context. We need to have our own reference to it, hence not using adoptRef.
            d->m_timeoutSource = soup_add_timeout(g_main_context_get_thread_default(), d->m_firstRequest.timeoutInterval() * 1000, requestTimeoutCallback, handle);
        }
        soup_request_send_async(d->m_soupRequest.get(), d->m_cancellable.get(), sendRequestCallback, handle);
    }

    return true;
}

SoupSession* ResourceHandle::defaultSession()
{
    static SoupSession* session = 0;
    // Values taken from http://www.browserscope.org/  following
    // the rule "Do What Every Other Modern Browser Is Doing". They seem
    // to significantly improve page loading time compared to soup's
    // default values.
    //
    // Change MAX_CONNECTIONS_PER_HOST value 6 -> 12, and maxConnections is changed from 35 to 60.
    // Enhanced network loading speed apply tunning value.
    static const int maxConnections = 60;
    static const int maxConnectionsPerHost = 12;

    if (!session) {
        session = soup_session_async_new();
        g_object_set(session,
                     SOUP_SESSION_MAX_CONNS, maxConnections,
                     SOUP_SESSION_MAX_CONNS_PER_HOST, maxConnectionsPerHost,
                     SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
                     SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER,
                     SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                     SOUP_SESSION_USE_THREAD_CONTEXT, TRUE,
                     NULL);
#if ENABLE(WEB_TIMING)
        g_signal_connect(G_OBJECT(session), "request-started", G_CALLBACK(requestStartedCallback), 0);
#endif
    }

    return session;
}

uint64_t ResourceHandle::getSoupRequestInitiaingPageID(SoupRequest* request)
{
    uint64_t* initiatingPageIDPtr = static_cast<uint64_t*>(g_object_get_data(G_OBJECT(request), gSoupRequestInitiaingPageIDKey));
    return initiatingPageIDPtr ? *initiatingPageIDPtr : 0;
}

#if ENABLE(TIZEN_PAUSE_NETWORK)
bool ResourceHandle::suspendRequest()
{
    if (d->m_soupMessage.get()) {

/*      FIXME: checking if status-code != 0 is too much!
        There can be situation that connection is initialised (so we can pause message)
        but status code is 0.
        We should instead connect to "request-started" SoupSession callback and allow to
        pause only requests which had request-started called. Another choice is to check
        if SoupMessage's internal io_data is != NULL but there is no API for this.

        gint intval;
        g_object_get (d->m_soupMessage.get(), "status-code", &intval,
                                               NULL);
        g_message("ResourceHandle::suspendRequest status-code is: %d", intval);
        if (intval != 0)
*/

// FIXME: Workaround patch to resolve slow loading issue(P130910-02494) because of many resources having same host.
#if ENABLE(TIZEN_SOUP_MESSAGE_PAUSE_SET_FLAG)
        SoupMessage* soupMessage = d->m_soupMessage.get();
        SoupURI* soupUri = soup_message_get_uri(soupMessage);
        const char* host = soup_uri_get_host(soupUri);
        if (equalIgnoringCase(String::fromUTF8(host), "spwd.photocolle-docomo.com") || equalIgnoringCase(String::fromUTF8(host), "smt.photo.smt.docomo.ne.jp"))
            soup_message_set_flags(soupMessage, static_cast<SoupMessageFlags>(soup_message_get_flags(soupMessage) | (1 << 7)));
#endif
        soup_session_pause_message(defaultSession(), d->m_soupMessage.get());
    }
    return true;
}

bool ResourceHandle::resumeRequest()
{
    if (d->m_soupMessage.get()) {
/*
        FIXME: checking if status-code != 0 is too much!
        see suspendRequest comment for details
        gint intval;
        g_object_get (d->m_soupMessage.get(), "status-code", &intval,
                                               NULL);
        g_message("ResourceHandle::resumedRequest status-code is: %d", intval);
        if (intval != 0)
*/

// FIXME: Workaround patch to resolve slow loading issue(P130910-02494) because of many resources having same host.
#if ENABLE(TIZEN_SOUP_MESSAGE_PAUSE_SET_FLAG)
        SoupMessage* soupMessage = d->m_soupMessage.get();
        SoupURI* soupUri = soup_message_get_uri(soupMessage);
        const char* host = soup_uri_get_host(soupUri);
        if (equalIgnoringCase(String::fromUTF8(host), "spwd.photocolle-docomo.com") || equalIgnoringCase(String::fromUTF8(host), "smt.photo.smt.docomo.ne.jp"))
            soup_message_set_flags(soupMessage, static_cast<SoupMessageFlags>(soup_message_get_flags(soupMessage)  & ~(1 << 7)));
#endif
        soup_session_unpause_message(defaultSession(), d->m_soupMessage.get());
    }
    return true;
}
#endif

#if ENABLE(TIZEN_PRIVATE_BROWSING)
bool ResourceHandle::m_privateBrowsingEnabled = false;

bool ResourceHandle::privateBrowsingEnabledGet()
{
    return m_privateBrowsingEnabled;
}

void ResourceHandle::setPrivateBrowsingEnabled(bool enabled)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] set private browsing enable = %d", enabled);
#endif

    if (m_privateBrowsingEnabled == enabled)
        return;
    else
        m_privateBrowsingEnabled = enabled;

    // private cookie set
    static SoupCookieJar* cookieJarNormal = 0;
    static SoupCookieJar* cookieJarPrivate = 0;

    if (enabled) {
        cookieJarNormal = WebCore::soupCookieJar();
        if (!cookieJarPrivate)
            cookieJarPrivate = soup_cookie_jar_new();
        WebCore::setSoupCookieJar(cookieJarPrivate);
    } else if (cookieJarNormal)
        WebCore::setSoupCookieJar(cookieJarNormal);

    soup_session_remove_feature_by_type(defaultSession(), SOUP_TYPE_COOKIE_JAR);
    soup_session_add_feature(defaultSession(), SOUP_SESSION_FEATURE(WebCore::soupCookieJar()));

    // private cache set
    static String cacheDirPath;

    if (enabled) {
        SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(defaultSession(), SOUP_TYPE_CACHE));
        if (!cache) {
            m_privateBrowsingEnabled = enabled;
            return;
        }

        GOwnPtr<char> cacheDir;
        g_object_get(G_OBJECT(cache), "cache-dir", &cacheDir.outPtr(), NULL);
        if (!cacheDir) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGI("[Network] !cacheDir");
#endif
            return;
        }
        cacheDirPath = String::fromUTF8(cacheDir.get());
        soup_cache_dump(cache);
#if ENABLE(TIZEN_COMPRESSION_PROXY)
        ResourceHandle::detachSoupCacheFromHTTPCacheQuerier();
#endif
        soup_session_remove_feature(defaultSession(), SOUP_SESSION_FEATURE(cache));
    } else {
        SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(defaultSession(), SOUP_TYPE_CACHE));
        if (cache)
            return;

        if (!cacheDirPath.isEmpty()) {
            cache = soup_cache_new(cacheDirPath.utf8().data(), SOUP_CACHE_SINGLE_USER);
            if (!cache) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGI("[Network] !cache soup_cache_new fail");
#endif
                return;
            }
            soup_session_add_feature(defaultSession(), SOUP_SESSION_FEATURE(cache));
            soup_cache_load(cache);
#if ENABLE(TIZEN_COMPRESSION_PROXY)
            ResourceHandle::attachSoupCacheToHTTPCacheQuerier();
#endif
            g_object_unref(cache);
        }
    }
}
#endif

#if ENABLE(TIZEN_SOUP_FEATURES)
void ResourceHandle::setSpdyEnabled(bool enabled)
{
#ifdef SOUP_SESSION_ALLOW_SPDY
    static bool spdyEnabled = false;

    ASSERT(spdyEnabled != enabled);

    spdyEnabled = enabled;
    g_object_set(defaultSession(), SOUP_SESSION_ALLOW_SPDY, spdyEnabled, NULL);
#else
    LOG(Network,"SOUP_SESSION_ALLOW_SPDY is not defined");
#endif
}

void ResourceHandle::setPerformanceFeaturesEnabled(bool enabled)
{
    // FIXME: this method doesn't do anything after commit
    // 9d56721aa9567c21cac1c42dc66b6f4d59efb2ce
    // It's still used in Settings
}
#endif

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup didReceiveAuthenticationChallenge \n");
#endif
    ASSERT(d->m_currentWebChallenge.isNull());

    d->m_currentWebChallenge = challenge;

    if (client())
        client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup receivedCredential \n");
#endif
    if (challenge != d->m_currentWebChallenge) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Loader] challenge != d->m_currentWebChallenge return");
#endif
        return;
    }

    // FIXME: Support empty credentials. Currently, an empty credential cannot be stored in WebCore credential storage, as that's empty value for its map.
    if (credential.isEmpty()) {
#if ENABLE(TIZEN_LOG)
        LOG(Network,"[Network] ResourceHandleSoup receivedCredential credential.isEmpty()\n");
#endif
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    if (credential.persistence() == CredentialPersistenceForSession) {
       Credential webCredential(credential.user(), credential.password(), CredentialPersistenceNone);

        KURL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = firstRequest().url();

        CredentialStorage::set(webCredential, challenge.protectionSpace(), urlToStore);

        soup_auth_authenticate(d->m_soupAuth.get(), credential.user().utf8().data(), credential.password().utf8().data());
        soup_session_unpause_message(defaultSession(), d->m_soupMessage.get());
    } else {
        soup_auth_authenticate(d->m_soupAuth.get(), credential.user().utf8().data(), credential.password().utf8().data());
        soup_session_unpause_message(defaultSession(), d->m_soupMessage.get());
    }

    clearAuthentication();
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup receivedRequestToContinueWithoutCredential\n");
#endif
    ASSERT(!challenge.isNull());

    if (challenge != d->m_currentWebChallenge) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Loader] challenge != d->m_currentWebChallenge return");
#endif
        return;
    }

    soup_auth_authenticate(d->m_soupAuth.get(), String("").utf8().data(), String("").utf8().data());
    soup_session_unpause_message(defaultSession(), d->m_soupMessage.get());

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
#if ENABLE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandleSoup receivedCancellation\n");
#endif
    if (challenge != d->m_currentWebChallenge) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[Loader] challenge != d->m_currentWebChallenge return");
#endif
        return;
    }

    clearAuthentication();
    soup_session_unpause_message(defaultSession(), d->m_soupMessage.get());
}
#endif

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
void ResourceHandle::receivedInterceptResponse(String headers, const char* body, unsigned int length)
{
    TIZEN_LOGI("[INTERCEPT] length: [%d]", length);

    if (d->m_cancelled) {
        TIZEN_SECURE_LOGI("[INTERCEPT] Load is cancelled for url : [%s]", this->firstRequest().url().string().utf8().data());
        deref();
        return;
    }

    int status_code = SOUP_STATUS_NONE;
    SoupURI *uri = soup_uri_new(this->firstRequest().url().string().utf8().data());

    if (!headers || headers.isEmpty()) {
        ResourceError error =  ResourceError(g_quark_to_string(SOUP_HTTP_ERROR),
                                              static_cast<gint>(SOUP_STATUS_NOT_FOUND),
                                              soup_uri_to_string (uri, false),
                                              String::fromUTF8("Intercept request failed"));
        client()->didFail(this, error);
        deref();
        return;
    }

    SoupMessage* soupMessage = soup_message_new(firstRequest().httpMethod().utf8().data(), firstRequest().url().string().utf8().data());
    if (!soupMessage) {
        TIZEN_LOGI("[INTERCEPT] soupMessage creation fail");
        return;
    }
    soupMessage->response_headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_RESPONSE);
    soup_headers_parse_response(headers.utf8().data(), headers.length(), soupMessage->response_headers, NULL, (guint *)&status_code, NULL);
    soupMessage->status_code = status_code;

    ResourceResponse response;
    
    response.updateFromSoupMessage(soupMessage);
    client()->setReceivedInterceptResponse(true);

    client()->didReceiveResponse(this, response);

    ResourceHandleClient* handle_client = this->client();
    if (d->m_cancelled || !handle_client) {
        TIZEN_LOGI("[INTERCEPT] receivedInterceptResponse d->m_cancelled[%d], client[0x%x]", d->m_cancelled, handle_client);
        deref();
        g_object_unref(soupMessage);
        return;
    }
    client()->didReceiveData(this, body, length, length);
    client()->didFinishLoading(this, 0);
    deref();
    g_object_unref(soupMessage);
}

void ResourceHandle::ignoreInterceptResponse()
{
    bool cancelled = d->m_cancelled;

    if (cancelled) {
        deref();
        return;
    }

    KURL url = firstRequest().url();
    String urlString = url.string();
    String protocol = url.protocol();

    if (equalIgnoringCase(protocol, "http") || equalIgnoringCase(protocol, "https")) {
#if ENABLE(TIZEN_COMPRESSION_PROXY)
		if (startHTTPRequest(this, firstRequest())) {
#else
        if (startHTTPRequest(this)) {
#endif
            deref();
            return;
        }
    }

    if (startNonHTTPRequest(this, url)) {
        deref();
        return;
    }

    this->scheduleFailure(InvalidURLFailure);
}
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
bool ResourceHandle::startRedirectRequestFromUC(ResourceRequest& request)
{
    KURL url = request.url();
    String urlString = url.string();
    String protocol = url.protocol();

    if (equalIgnoringCase(protocol, "http") || equalIgnoringCase(protocol, "https")) {
        if (startHTTPRequest(this, request))
            return true;
    }

    if (startNonHTTPRequest(this, url))
        return true;
}
#endif
}
