#include "content_client.h"
#include "./common/chrome_version.h"

namespace meson
{
ContentClient::ContentClient()
{
}
ContentClient::~ContentClient()
{
}

std::string ContentClient::GetProduct() const
{
    return "chrome/" CHROME_VERSION_STRING;
}

void ContentClient::AddAdditionalSchemes(
    std::vector<url::SchemeWithType> *standard_schemes,
    std::vector<url::SchemeWithType> *referrer_schemes,
    std::vector<std::string> *savable_schemes)
{
    url::SchemeWithType t = {"chrome-extension", url::SCHEME_WITHOUT_PORT};
    standard_schemes->push_back(t);
}
}