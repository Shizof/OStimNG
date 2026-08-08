#pragma once

namespace OStim {
    class ActionTag {
    public:
        // --- ABI version 1 ---
        virtual const char* getTagID() = 0;
    };
}