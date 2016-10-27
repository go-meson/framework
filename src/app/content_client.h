#pragma once

#include <string>

#include "brightray/common/content_client.h"
#include "brightray/common/application_info.h"

namespace meson
{
class ContentClient : public brightray::ContentClient
{
public:
  ContentClient();
  virtual ~ContentClient();

protected:
  virtual std::string GetProduct() const override;
  virtual void AddAdditionalSchemes(
      std::vector<url::SchemeWithType> *standard_schemes,
      std::vector<url::SchemeWithType> *referrer_schemes,
      std::vector<std::string> *savable_schemes) override;

private:
  DISALLOW_COPY_AND_ASSIGN(ContentClient);
};
}