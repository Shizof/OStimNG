#pragma once
#include "VRAPI/OstimVRinterface001.h"
#include "VRAPI/OstimVR.h"




#define OSTIMVR_BUILD_NUMBER 100

namespace OstimVRPluginAPI
{
    void* GetApi(unsigned int revisionNumber);

    void ModMessageHandler(SKSE::MessagingInterface::Message* message);

	// This object provides access to OstimVR's mod support API version 1
    struct OstimVRInterface001 : IOstimVRInterface001 {
        // Gets the build number
        virtual unsigned int getBuildNumber();

        virtual bool IsPlayerOstimScenePlaying();

        virtual void GetMenuData(char** outStr, int& outStrLength);

        virtual void SelectOption(const char* id);

        virtual bool IsInOptionsMode();

        virtual void ChangeSpeed(bool increaseSpeed);

        virtual void SwitchCamera(bool firstperson);

        virtual void EndSceneEarly();

        virtual bool IsCameraFirstPerson();

        virtual bool InTransition();

        virtual bool InSequence();

        virtual void GetSceneOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset);

        virtual void GetGlobalOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset);

        virtual void ModifyOffsets(float offsetX, float offsetY, float offsetZ, float rotationOffset, bool global);

        virtual void SaveGlobalOffsets();

        virtual void SaveSceneOffsets();

        virtual void SaveSceneOffsetsForAllSet();

        virtual void GetExcitements(float& domRatio, float& subRatio, float& thirdRatio, bool& domEnabled, bool& subEnabled, bool& thirdEnabled);

    };
}  // namespace OstimVRPluginAPI
extern OstimVRPluginAPI::OstimVRInterface001 g_interface001;

