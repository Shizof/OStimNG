#pragma once

#include "../ActionAttributes.h"

#include "GameAPI/GameFaction.h"

using actionIndex = uint8_t;

namespace Graph {
    namespace Action {
        struct Action {
        public:
            actionIndex index = 0;
            std::string type = "";
            ActionAttributes* attributes = nullptr;
            RoleMap<int> roles = {-1, -1, -1};
            bool muted = false;
            bool doPeaks = true;
            bool peaksAnnotated = false;

            bool doFullStrip(int position);
            uint32_t getStrippingMask(int position);
            bool isType(std::string type);
            bool isType(std::vector<std::string> types);
        };
    }
}