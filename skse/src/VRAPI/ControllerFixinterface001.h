#pragma once
#include <SKSE/SKSE.h>

namespace ControllerFixPluginApi {
	// Returns an IControllerFixInterface001 object compatible with the API shown below
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin
	class IControllerFixInterface001;
	IControllerFixInterface001* getControllerFixInterface001();

	// This object provides access to ControllerFix VR's mod support API
	class IControllerFixInterface001
	{
	public:
		// Gets the build number
		virtual unsigned int getBuildNumber() = 0;

		virtual void ForceShowControllers(bool enable) = 0;
	};
}

extern ControllerFixPluginApi::IControllerFixInterface001* g_ControllerFixInterface;
