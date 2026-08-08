#include "FurnitureInterfaceImpl.h"

#include "../InterfaceMapImpl.h"

#include "Furniture/FurnitureTable.h"

namespace Interface {
    uint32_t FurnitureInterfaceImpl::getVersion() {
        return InterfaceMapImpl::CORE_INTERFACE_VERSION;
    }


    OStim::FurnitureType* FurnitureInterfaceImpl::getType(const char* id) {
        return Furniture::FurnitureTable::getFurnitureType(std::string(id));
    }
}