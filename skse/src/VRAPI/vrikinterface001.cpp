#include "vrikinterface001.h"

// A message used to fetch VRIK's interface
struct VrikMessage {
	enum { kMessage_GetInterface = 0xF2AFAEE6 }; // Randomly chosen by cat
	void* (*getApiFunction)(unsigned int revisionNumber) = nullptr;
};

// Stores the API after it has already been fetched
vrikPluginApi::IVrikInterface001* g_vrikInterface = nullptr;

// Fetches the interface to use from VRIK
vrikPluginApi::IVrikInterface001* vrikPluginApi::getVrikInterface001()
{
	// If the interface has already been fetched, rturn the same object
	if (g_vrikInterface) {
		return g_vrikInterface;
	}

	// Dispatch a message to get the plugin interface from VRIK
	VrikMessage vrikMessage;
    const auto skseMessaging = SKSE::GetMessagingInterface();
    skseMessaging->Dispatch(VrikMessage::kMessage_GetInterface, (void*)&vrikMessage, sizeof(VrikMessage*), "VRIK");	
	if (!vrikMessage.getApiFunction) {
		return nullptr;
	}

	// Fetch the API for this version of the VRIK interface
	g_vrikInterface = static_cast<IVrikInterface001*>(vrikMessage.getApiFunction(1));
	return g_vrikInterface;
}



int GetVRIKCameraOffsetting()
{
	if (g_vrikInterface)
	{
		if (g_vrikInterface->getBuildNumber() >= 80200)
		{
			return g_vrikInterface->getSettingDouble("cameraOffsetting");
		}
		else
		{
			g_vrikInterface->getSettingDouble("headBobbingHeight");
		}
	}
	return 1;
}

void SetVRIKCameraOffsetting(int value)
{
	if (g_vrikInterface)
	{
		if (g_vrikInterface->getBuildNumber() >= 80200)
		{
			g_vrikInterface->setSettingDouble("cameraOffsetting", value);
		}
		else
		{
			g_vrikInterface->setSettingDouble("headBobbingHeight", value);
		}
	}
}