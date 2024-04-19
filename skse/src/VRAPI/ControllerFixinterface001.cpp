#include "ControllerFixinterface001.h"

// A message used to fetch ControllerFix VR's interface
struct ControllerFixMessage
{
	enum { kMessage_GetInterface = 0xFA17C15E }; // Randomly chosen by the universe
	void* (*GetApiFunction)(unsigned int revisionNumber) = nullptr;
};

// Stores the API after it has already been fetched
static ControllerFixPluginApi::IControllerFixInterface001* g_ControllerFixInterface = nullptr;

// Fetches the interface to use from ControllerFix
ControllerFixPluginApi::IControllerFixInterface001* ControllerFixPluginApi::getControllerFixInterface001()
{
	// If the interface has already been fetched, return the same object
	if (g_ControllerFixInterface) {
		return g_ControllerFixInterface;
	}

	// Dispatch a message to get the plugin interface from ControllerFix
	ControllerFixMessage cMessage;
    const auto skseMessaging = SKSE::GetMessagingInterface();
    skseMessaging->Dispatch(ControllerFixMessage::kMessage_GetInterface, (void*)&cMessage, sizeof(ControllerFixMessage*), "ControllerFixVR");
    if (!cMessage.GetApiFunction) {
        return nullptr;
    }

	// Fetch the API for this version of the ControllerFix interface
    g_ControllerFixInterface = static_cast<IControllerFixInterface001*>(cMessage.GetApiFunction(1));
	return g_ControllerFixInterface;
}
