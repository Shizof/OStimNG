#pragma once

#include "PluginInterface/InterfaceMap.h"
#include "Util/Singleton.h"

namespace Interface {
    class InterfaceMapImpl : public OStim::InterfaceMap, public Util::Singleton<InterfaceMapImpl> {
    public:
        inline static const uint32_t CORE_INTERFACE_VERSION = 3;

        virtual OStim::PluginInterface* queryInterface(const char* name) override;
        virtual bool addInterface(const char* name, OStim::PluginInterface* pluginInterface) override;
        virtual OStim::PluginInterface* removeInterface(const char* name) override;

        void setupInterfaces();

    private:
        std::unordered_map<const char*, OStim::PluginInterface*> interfaces;
    };
}