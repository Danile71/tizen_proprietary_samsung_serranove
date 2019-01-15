/*
 * Copyright (C) 2004, 2006, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ResourceHandle_h
#define ResourceHandle_h

#include "AuthenticationChallenge.h"
#include "AuthenticationClient.h"
#include "HTTPHeaderMap.h"
#include "NetworkingContext.h"
#include <wtf/OwnPtr.h>

#if ENABLE(TIZEN_COMPRESSION_PROXY)
#include <UCProxySDK/ProxyURLRequestDelegate.h>
#include <UCProxySDK/ProxyRequestContext.h>
#endif

#if USE(SOUP)
typedef struct _SoupSession SoupSession;
typedef struct _SoupRequest SoupRequest;
#endif

#if USE(CF)
typedef const struct __CFData * CFDataRef;
#endif

#if USE(WININET)
typedef unsigned long DWORD;
typedef unsigned long DWORD_PTR;
typedef void* LPVOID;
typedef LPVOID HINTERNET;
#endif

#if PLATFORM(MAC) || USE(CFURLSTORAGESESSIONS)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(MAC)
OBJC_CLASS NSData;
OBJC_CLASS NSError;
OBJC_CLASS NSURLConnection;
OBJC_CLASS WebCoreResourceHandleAsDelegate;
#ifndef __OBJC__
typedef struct objc_object *id;
#endif
#endif

#if USE(CFNETWORK)
typedef struct _CFURLConnection* CFURLConnectionRef;
typedef int CFHTTPCookieStorageAcceptPolicy;
typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
#endif

#if USE(CFURLSTORAGESESSIONS) && PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED <= 1060
typedef struct __CFURLStorageSession* CFURLStorageSessionRef;
#elif USE(CFURLSTORAGESESSIONS)
typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
#endif


#if ENABLE(TIZEN_COMPRESSION_PROXY)
using namespace proxy;
namespace proxy {
    class HTTPCachePushEntry;
}
#endif

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class Frame;
class KURL;
class ProtectionSpace;
class ResourceError;
class ResourceHandleClient;
class ResourceHandleInternal;
class ResourceRequest;
class ResourceResponse;
class SchedulePair;
class SharedBuffer;

enum StoredCredentials {
    AllowStoredCredentials,
    DoNotAllowStoredCredentials
};

template <typename T> class Timer;

class ResourceHandle : public RefCounted<ResourceHandle>
#if PLATFORM(MAC) || USE(CFNETWORK) || USE(CURL) || ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    , public AuthenticationClient
#endif
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    , public ProxyURLRequestDelegate
#endif
    {
public:
    static PassRefPtr<ResourceHandle> create(NetworkingContext*, const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff);
    static void loadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentials, ResourceError&, ResourceResponse&, Vector<char>& data);

    static bool willLoadFromCache(ResourceRequest&, Frame*);
    static void cacheMetadata(const ResourceResponse&, const Vector<char>&);

    virtual ~ResourceHandle();

#if PLATFORM(MAC) || USE(CFNETWORK)
    void willSendRequest(ResourceRequest&, const ResourceResponse& redirectResponse);
    bool shouldUseCredentialStorage();
#endif
#if PLATFORM(MAC) || USE(CFNETWORK) || USE(CURL) || ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
    virtual void receivedCredential(const AuthenticationChallenge&, const Credential&);
    virtual void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);
    virtual void receivedCancellation(const AuthenticationChallenge&);
#endif
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    bool startUC(NetworkingContext *netContext, ResourceRequest& request);
    void stopUC(bool cancelNetwork);
    bool startRedirectRequestFromUC(ResourceRequest& request);
    void cancelUC();
    static proxy::ProxyRequestContext* getProxyRequestContext();
    static void ensureProxyRequestContext();
    virtual void onResponseReceived(const proxy::ProxyURLResponse& response) ;
    virtual void onDataReceived(const char* data, int length) ;
    virtual void onFinishLoading();
    virtual void onWillSendRequest(proxy::ProxyURLRequest& request);
    virtual void onRedirectReceived(const proxy::ProxyURLResponse&);
    virtual void onRequestRejected(const proxy::ProxyURLRequest& request, proxy::RejectReasonCode reason);
    virtual void onDownloadResponseReceived(const proxy::ProxyURLResponse& response);
    virtual void onError(const std::string& errorDomain, int errorCode, const std::string& errorDescription, const std::string& errorURL);

    static void compressionProxyDataSizeGet (unsigned int& original, unsigned int& compressed);
    static void compressionProxyDataSizeSet (unsigned int original, unsigned int compressed);
    static void compressionProxyLocalDataSizeSet (unsigned int original, unsigned int compressed);
    static void compressionProxyDataSizeFilePathSet(const String& soupDataDirectory);
    static String compressionProxyDataSizeFilePathGet();
    static void compressionProxyDataSizeLoad();
    static void compressionProxyDataSizeDump();
    static void compressionProxyImageQualitySet(int quality);
    static bool is_uri_blacklisted_for_UCproxy (const char *url);
    static bool is_site_local (const char* host);
    static void attachSoupCacheToHTTPCacheQuerier();
    static void detachSoupCacheFromHTTPCacheQuerier();
#endif

#if USE(SOUP)
#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    bool shouldUseCredentialStorage();
#endif
#endif

#if PLATFORM(MAC)
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    bool canAuthenticateAgainstProtectionSpace(const ProtectionSpace&);
#endif
#if !USE(CFNETWORK)
    void didCancelAuthenticationChallenge(const AuthenticationChallenge&);
    NSURLConnection *connection() const;
    WebCoreResourceHandleAsDelegate *delegate();
    void releaseDelegate();
    id releaseProxy();
#endif

    void schedule(SchedulePair*);
    void unschedule(SchedulePair*);
#endif
#if USE(CFNETWORK)
    CFURLConnectionRef connection() const;
    CFURLConnectionRef releaseConnectionForDownload();
    static void setHostAllowsAnyHTTPSCertificate(const String&);
    static void setClientCertificate(const String& host, CFDataRef);
#endif

#if PLATFORM(WIN) && USE(CURL)
    static void setHostAllowsAnyHTTPSCertificate(const String&);
#endif
#if PLATFORM(WIN) && USE(CURL) && USE(CF)
    static void setClientCertificate(const String& host, CFDataRef);
#endif

    bool shouldContentSniff() const;
    static bool shouldContentSniffURL(const KURL&);

    static void forceContentSniffing();

#if USE(WININET)
    void setSynchronousInternetHandle(HINTERNET);
    void fileLoadTimer(Timer<ResourceHandle>*);
    void onRedirect();
    bool onRequestComplete();
    static void CALLBACK internetStatusCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);
#endif

#if PLATFORM(QT) || USE(CURL) || USE(SOUP) || PLATFORM(BLACKBERRY)
    ResourceHandleInternal* getInternal() { return d.get(); }
#endif

#if USE(SOUP)
    static SoupSession* defaultSession();
    static uint64_t getSoupRequestInitiaingPageID(SoupRequest*);
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    void cancelSoup();
#endif
#endif

    // Used to work around the fact that you don't get any more NSURLConnection callbacks until you return from the one you're in.
    static bool loadsBlocked();    

    bool hasAuthenticationChallenge() const;
    void clearAuthentication();
    virtual void cancel();

    // The client may be 0, in which case no callbacks will be made.
    ResourceHandleClient* client() const;
    void setClient(ResourceHandleClient*);

    void setDefersLoading(bool);

#if PLATFORM(BLACKBERRY)
    void pauseLoad(bool);
#endif
      
    ResourceRequest& firstRequest();
    const String& lastHTTPMethod() const;

    void fireFailure(Timer<ResourceHandle>*);

#if USE(SOUP) && ENABLE(TIZEN_PAUSE_NETWORK)
    bool suspendRequest();
    bool resumeRequest();
#endif

#if USE(CFURLSTORAGESESSIONS)
    static CFURLStorageSessionRef currentStorageSession();
    static void setDefaultStorageSession(CFURLStorageSessionRef);
    static CFURLStorageSessionRef defaultStorageSession();

    static void setPrivateBrowsingStorageSessionIdentifierBase(const String&);
    static RetainPtr<CFURLStorageSessionRef> createPrivateBrowsingStorageSession(CFStringRef identifier);
#endif // USE(CFURLSTORAGESESSIONS)

#if USE(CFURLSTORAGESESSIONS) || ENABLE(TIZEN_PRIVATE_BROWSING)
    static void setPrivateBrowsingEnabled(bool);
    static bool privateBrowsingEnabledGet();
#endif

#if ENABLE(TIZEN_SOUP_FEATURES)
    static void setSpdyEnabled(bool);
    static void setPerformanceFeaturesEnabled(bool);
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
    static bool compressionProxyEnabled() { return m_compressionProxyEnabled; }
    static void setCompressionProxyEnabled(bool enabled);
    static GHashTable* get_proxy_failure_table();
    static void  set_proxy_failure_table(GHashTable *table);
#endif

    using RefCounted<ResourceHandle>::ref;
    using RefCounted<ResourceHandle>::deref;

#if PLATFORM(MAC) || USE(CFNETWORK)
    static CFStringRef synchronousLoadRunLoopMode();
#endif

#if HAVE(NETWORK_CFDATA_ARRAY_CALLBACK)
    void handleDataArray(CFArrayRef dataArray);
#endif

    typedef PassRefPtr<ResourceHandle> (*BuiltinConstructor)(const ResourceRequest& request, ResourceHandleClient* client);
    static void registerBuiltinConstructor(const AtomicString& protocol, BuiltinConstructor);
#if ENABLE(TIZEN_INTERCEPT_REQUEST)
    void receivedInterceptResponse(String headers, const char* body, unsigned int length);
    void ignoreInterceptResponse();
#endif

protected:
    ResourceHandle(const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff);

private:
    enum FailureType {
        NoFailure,
        BlockedFailure,
        InvalidURLFailure
    };

    void platformSetDefersLoading(bool);

    void scheduleFailure(FailureType);

    bool start(NetworkingContext*);

    virtual void refAuthenticationClient() { ref(); }
    virtual void derefAuthenticationClient() { deref(); }

#if PLATFORM(MAC) && !USE(CFNETWORK)
    void createNSURLConnection(id delegate, bool shouldUseCredentialStorage, bool shouldContentSniff);
#elif USE(CF)
    void createCFURLConnection(bool shouldUseCredentialStorage, bool shouldContentSniff);
#endif

#if USE(CFURLSTORAGESESSIONS)
    static String privateBrowsingStorageSessionIdentifierDefaultBase();
    static CFURLStorageSessionRef privateBrowsingStorageSession();
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
    static ProxyRequestContext* m_proxyRequestContext;
    ProxyURLRequest* m_proxyRequest;
    proxy::HTTPCachePushEntry* m_cachePushEntry;
    static bool m_compressionProxyEnabled;
    static unsigned long int m_localOriginalSize;
    static unsigned long int m_localCompressedSize;
    static unsigned long int m_originalSize;
    static unsigned long int m_compressedSize;
    static String m_compressionDataSizeFilePath;
    static GHashTable* m_uc_proxy_failure_info;
#endif
#if USE(CFURLSTORAGESESSIONS) || ENABLE(TIZEN_PRIVATE_BROWSING)
    static bool m_privateBrowsingEnabled;
#endif
    friend class ResourceHandleInternal;
    OwnPtr<ResourceHandleInternal> d;
};

}

#endif // ResourceHandle_h
