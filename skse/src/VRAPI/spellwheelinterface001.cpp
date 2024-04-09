#include "spellwheelinterface001.h"

// A message used to fetch durability VR's interface
struct SpellWheelMessage
{
	enum { kMessage_GetInterface = 0xFA27C15D }; // Randomly chosen by the universe
	void* (*GetApiFunction)(unsigned int revisionNumber) = nullptr;
};

// Stores the API after it has already been fetched
static spellwheelPluginApi::ISpellWheelInterface001* g_spellwheelInterface = nullptr;

// Fetches the interface to use from Durability
spellwheelPluginApi::ISpellWheelInterface001* spellwheelPluginApi::getSpellWheelInterface001()
{
	// If the interface has already been fetched, return the same object
	if (g_spellwheelInterface) {
		return g_spellwheelInterface;
	}

	// Dispatch a message to get the plugin interface from SpellWheel
	SpellWheelMessage swMessage;
	const auto skseMessaging = SKSE::GetMessagingInterface();
    skseMessaging->Dispatch(SpellWheelMessage::kMessage_GetInterface, (void*)&swMessage, sizeof(SpellWheelMessage*), "SpellWheelVR");
    if (!swMessage.GetApiFunction) {
        return nullptr;
    }

	if (!swMessage.GetApiFunction)
	{
		return nullptr;
	}

	// Fetch the API for this version of the Durability interface
	g_spellwheelInterface = static_cast<ISpellWheelInterface001*>(swMessage.GetApiFunction(1));
	return g_spellwheelInterface;
}
