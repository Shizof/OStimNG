#pragma once
#include <SKSE/SKSE.h>

namespace spellwheelPluginApi {
	// Returns an ISpellWheelInterface001 object compatible with the API shown below
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin
	class ISpellWheelInterface001;
	ISpellWheelInterface001* getSpellWheelInterface001();

	// This object provides access to Spell Wheel VR's mod support API
	class ISpellWheelInterface001
	{
	public:
		// Gets the build number
		virtual unsigned int getBuildNumber() = 0;

		virtual bool IsMainWheelOpen() = 0;
		virtual bool IsSecondaryWheelOpen() = 0;
		virtual void SpawnConjurationCircle(RE::NiPoint3 pos) = 0;
        virtual void CloseOstimWheels() = 0;
	};
}

extern spellwheelPluginApi::ISpellWheelInterface001* g_spellwheelInterface;
