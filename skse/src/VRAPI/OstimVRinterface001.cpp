#include "VRAPI/OstimVRinterface001.h"

// Stores the API after it has already been fetched
OstimVRPluginAPI::IOstimVRInterface001* g_OstimVRInterface = nullptr;

// Fetches the interface to use from OstimVR
OstimVRPluginAPI::IOstimVRInterface001* OstimVRPluginAPI::GetOstimVRInterface001() {
    // If the interface has already been fetched, return the same object
    if (g_OstimVRInterface) {
        return g_OstimVRInterface;
    }

    // Dispatch a message to get the plugin interface from OstimVR
    OstimVRMessage ostimMessage;
    const auto skseMessaging = SKSE::GetMessagingInterface();
    skseMessaging->Dispatch(OstimVRMessage::kMessage_GetInterface, (void*)&ostimMessage, sizeof(OstimVRMessage*), OstimVRPluginName);
    if (!ostimMessage.GetApiFunction) {
        return nullptr;
    }

    // Fetch the API for this version of the OstimVR interface
    g_OstimVRInterface = static_cast<IOstimVRInterface001*>(ostimMessage.GetApiFunction(1));
    return g_OstimVRInterface;
}
