#pragma once

#include "base/compiler_specific.h"
#include "content/public/renderer/content_renderer_client.h"

namespace meson {
    class MesonRendererClient : public content::ContentRendererClient {
        public:
        MesonRendererClient();
        virtual ~MesonRendererClient();

        DISALLOW_COPY_AND_ASSIGN(MesonRendererClient);
    };
}

