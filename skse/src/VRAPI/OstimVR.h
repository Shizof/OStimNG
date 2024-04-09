#pragma once
#include <PCH.h>
#include "vrikinterface001.h"
#include "spellwheelinterface001.h"
#include "Utility.hpp"

#include <RE/Skyrim.h>
#include "Core/Core.h"
#include "Core/ThreadManager.h"
#include <UI/Scene/Datatypes.h>
#include <UI/Scene/Datatypes.h>
#include <UI/UIState.h>

namespace OStimVR 
{
    using VM = RE::BSScript::Internal::VirtualMachine;

    extern vrikPluginApi::IVrikInterface001* vrikInterface;
    extern spellwheelPluginApi::ISpellWheelInterface001* spellWheelInterface;

    void PlayerSceneStart();
    void PlayerSceneEnd();
    void CameraSwitchFunc(bool firstPerson);
    bool GetIsCameraFirstPerson();
    void loadConfig();
    void VRIKLockPositionAndRotation(float rotSin, float rotCos, float x, float y, float z, float r);


    
	using EnablePlayerControls = __int64 (*)(VM* registry, uint32_t stackId, __int64 a3, bool abMovement,
                                             bool abFighting, bool abCamSwitch, bool abLooking, bool abSneaking,
                                             bool abMenu, bool abActivate, bool abJournalTabs, int aiDisablePOVType);

    using DisablePlayerControls = __int64 (*)(VM* registry, uint32_t stackId, __int64 a3, bool abMovement,
                                              bool abFighting, bool abCamSwitch, bool abLooking,
                                              bool abSneaking, bool abMenu, bool abActivate,
                                              bool abJournalTabs, int aiDisablePOVType);

}