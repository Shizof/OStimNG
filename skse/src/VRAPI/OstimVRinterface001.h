#pragma once
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace OstimVRPluginAPI 
{
    // Returns an IOstimVRInterface001 object compatible with the API shown below
    // This should only be called after SKSE sends kMessage_PostLoad to your plugin
    constexpr const auto OstimVRPluginName = "OStim";
    
    // A message used to fetch OstimVR's interface
    struct OstimVRMessage {
        enum : uint32_t { kMessage_GetInterface = 0x33ea7ba5 };  
        void* (*GetApiFunction)(unsigned int revisionNumber) = nullptr;
    };

    struct IOstimVRInterface001;
    IOstimVRInterface001* GetOstimVRInterface001();

    // This object provides access to OstimVR's mod support API
    struct IOstimVRInterface001 
    {
        // Gets the build number
        virtual unsigned int getBuildNumber() = 0;

        virtual bool IsPlayerOstimScenePlaying() = 0;

        virtual void GetMenuData(char** outStr, int& outStrLength) = 0;

        virtual void SelectOption(const char* id) = 0;

        virtual bool IsInOptionsMode() = 0;

        virtual void ChangeSpeed(bool increaseSpeed) = 0;	

        virtual void SwitchCamera(bool firstperson) = 0;

        virtual void EndSceneEarly() = 0;

        virtual bool IsCameraFirstPerson() = 0;

        virtual bool InTransition() = 0;

        virtual bool InSequence() = 0;

        virtual void GetSceneOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset) = 0;

        virtual void GetGlobalOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset) = 0;

        virtual void ModifyOffsets(float offsetX, float offsetY, float offsetZ, float rotationOffset, bool global) = 0;

        virtual void SaveGlobalOffsets() = 0;

        virtual void SaveSceneOffsets() = 0;

        virtual void SaveSceneOffsetsForAllSet() = 0;

        virtual void GetExcitements(float& domRatio, float& subRatio, float& thirdRatio, bool& domEnabled, bool& subEnabled, bool& thirdEnabled) = 0;
    };
}  // namespace OstimVRPluginAPI

extern OstimVRPluginAPI::IOstimVRInterface001* g_OstimVRInterface;
