
#ifndef __UCProxy__NetTransactionClient__H__
#define __UCProxy__NetTransactionClient__H__

#include <string>

namespace proxy {

class HTTPURLRequest;
class HTTPURLResponse;

class NetTransactionClient
{
public:
	virtual ~NetTransactionClient(){}

    virtual void onResponseReceived(const HTTPURLResponse& response) = 0;

    virtual void onDataReceived(const char* data, int length) = 0;

    virtual void onFinishLoading() = 0;

	virtual void onError(const std::string& domain, int errorCode, const std::string& failingURL, const std::string& localizedDescription) = 0;
};

} // namespace proxy

#endif // __UCProxy__NetTransactionClient__H__