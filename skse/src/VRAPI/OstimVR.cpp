#include "VRAPI/OstimVR.h"

namespace OStimVR 
{
    vrikPluginApi::IVrikInterface001* vrikInterface;

    bool iniSettingsSetBefore = false;
    float prevGamepadLookAngleSnapAmount = 45.0f;
    float prevActivatePickLength = 180.0f;
    float prevActivatePickRadius = 16.0f;
        
	REL::Relocation<float*> g_fActivatePickLength(REL::VariantID(0, 0, 0x1E95188));
    REL::Relocation<float*> g_fActivatePickRadius(REL::VariantID(0, 0, 0x1E95170));
    REL::Relocation<float*> g_fGamepadLookAngleSnapAmount(REL::VariantID(0, 0, 0x1E71688));
    REL::Relocation<bool*> g_bDisablePlayerCollision(REL::VariantID(0, 0, 0x1EAF210));

    REL::Relocation<EnablePlayerControls> EnablePlayerControlsFunc(REL::VariantID(0, 0, 0x9AC260));
    REL::Relocation<DisablePlayerControls> DisablePlayerControlsFunc(REL::VariantID(0, 0, 0x9AC190));
    
    

    // OStimVR Settings 3rd person
    int lockHeightToBody = 1;

    // OStimVR Settings 1st person
    int trackHands = 1;
    int trackHead = 1;
    float hidePlayerHeadDistance = 16.0f;
    float nearDistance = 3.0f;
    int lockHmdToBody = 0;
    float lockHmdMinThreshold = 20.0f;
    float lockHmdMaxThreshold = 2.0f;
    float lockHmdSpeed = 20.0f;

    // OStimVR Settings General
    float heightAdjustSpeed = 1.0f;
    int defaultThirdPerson = 0;
    float offsetForward = 0.0f;
    float offsetSideways = 0.0f;

    bool CurrentCameraFirstPerson = true;

    bool GetIsCameraFirstPerson()
    { 
        return CurrentCameraFirstPerson;
    }

    //
    void SetOstimVRSettings(bool firstPerson)
    {
        if (firstPerson)
        {
            EnablePlayerControlsFunc(VM::GetSingleton(), 0, 0, true, false, false, true, false, true, false, true, 0);
            DisablePlayerControlsFunc(VM::GetSingleton(), 0, 0, false, true, true, false, true, false, true, false, 0);
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player != nullptr) player->SetAIDriven(true);
            //Game.EnablePlayerControls(true, false, false, true, false, true, false, true, 0)
            //Game.DisablePlayerControls(false, true, true, false, true, false, true, false, 0)
            //Game.SetPlayerAIDriven(true)
        }

        if (vrikInterface != nullptr) {
            vrikInterface->setSettingDouble("enablePosture", 0);
            vrikInterface->setSettingDouble("enableBody", 0);
            vrikInterface->setSettingDouble("enableJumping", 0);
            vrikInterface->setSettingDouble("displayHolsters", 0);
                        
            if (firstPerson && (offsetForward >= 0.0001f || offsetForward <= 0.0001f || offsetSideways >= 0.0001f || offsetSideways <= 0.0001f)) 
            {
                RE::NiPoint3 offset(0,0,0);

                auto player = RE::PlayerCharacter::GetSingleton();
                if (player != nullptr)
                {
                    RE::TESObjectREFR* playerRef = player->AsReference();
                    if (playerRef != nullptr)
                    {
                        RE::NiPoint3 playerForward = normalize(GetForwardVector(playerRef->GetAngleZ()));

                        offset += playerForward * offsetForward;

                        RE::NiPoint3 playerSideways = normalize(GetSidewaysVector(playerForward));

                        offset += playerSideways * offsetSideways;

                        vrikInterface->setSettingDouble("lockPositionX", offset.x);
                        vrikInterface->setSettingDouble("lockPositionY", offset.y);
                    }
                }
            }

            vrikInterface->setSettingDouble("lockPosition", firstPerson ? 1 : 0);
            vrikInterface->setSettingDouble("lockRotation", 1);
            vrikInterface->setSettingDouble("nearClipDistance", nearDistance);
            vrikInterface->setSettingDouble("hidePlayerHeadDistance", firstPerson ? hidePlayerHeadDistance : 2.0f);

            vrikInterface->setSettingDouble("lockHeightToBody", firstPerson ? 1.0 : lockHeightToBody);
            vrikInterface->setSettingDouble("heightAdjustSpeed", heightAdjustSpeed);

            if (firstPerson && trackHands) 
            {
                vrikInterface->setSettingDouble("enableLeftArm", 1);
                vrikInterface->setSettingDouble("enableRightArm", 1);
                vrikInterface->setSettingDouble("enableInteractiveHands", 0);
            } 
            else 
            {
                vrikInterface->setSettingDouble("enableLeftArm", 0);
                vrikInterface->setSettingDouble("enableRightArm", 0);
            }

            vrikInterface->setSettingDouble("enableHead", firstPerson ? trackHead : 0);
            vrikInterface->setSettingDouble("lockHmdToBody", firstPerson ? lockHmdToBody : 0);
            vrikInterface->setSettingDouble("lockHmdMinThreshold", firstPerson ? lockHmdMinThreshold : 500.0f);
            vrikInterface->setSettingDouble("lockHmdMaxThreshold", firstPerson ? lockHmdMaxThreshold : 500.0f);
            vrikInterface->setSettingDouble("lockHmdSpeed", firstPerson ? lockHmdSpeed : 20.0f);

            auto ini = RE::INISettingCollection::GetSingleton();
            if (ini) {
                RE::Setting* playerCollision = ini->GetSetting("bDisablePlayerCollision:Havok");
                if (playerCollision) {
                    playerCollision->data.b = true;
                    g_bDisablePlayerCollision = true;
                    logger::info("Set bDisablePlayerCollision to true");
                }
            }
        }

        if (!firstPerson) 
        {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player != nullptr) player->SetAIDriven(false);
            EnablePlayerControlsFunc(VM::GetSingleton(), 0, 0, true, false, false, true, false, true, false, true, 0);
            DisablePlayerControlsFunc(VM::GetSingleton(), 0, 0, false, true, true, false, true, false, true, false, 0);
            // Game.EnablePlayerControls(true, false, false, true, false, true, false, true, 0)
            // Game.SetPlayerAIDriven(false)
        } 

        //freezes game
        /*OStim::ThreadManager* threadManager = OStim::ThreadManager::GetSingleton();
        if (threadManager!=nullptr)
        {
            auto playerThread = threadManager->getPlayerThread();
            if (playerThread != nullptr)
            {
                playerThread->alignActors();
            }
        }*/        
    }


    void CameraSwitchFunc(bool firstPerson)
    {
        logger::info("Applying {} settings", firstPerson ? "First Person" : "Third Person");

        // implement first person / third person switch here.        
        SetOstimVRSettings(firstPerson);
        CurrentCameraFirstPerson = firstPerson;
    }
    
    void PlayerSceneStart() 
    { 
        auto ini = RE::INISettingCollection::GetSingleton();
        if (ini) 
        {
            RE::Setting* pickLength = ini->GetSetting("fActivatePickLength:Interface");
            if (pickLength) {
                prevActivatePickLength = pickLength->data.f;
                pickLength->data.f = 1.0f;
                *g_fActivatePickLength = 1.0f;
            }

            RE::Setting* pickRadius = ini->GetSetting("fActivatePickRadius:Interface");
            if (pickRadius) {
                prevActivatePickRadius = pickRadius->data.f;
                pickRadius->data.f = 1.0f;
                *g_fActivatePickRadius = 1.0f;
            }
        }

        RE::Setting* snapAmount = RE::GetINISetting("fGamepadLookAngleSnapAmount:VRInput");
        if (snapAmount) {
            prevGamepadLookAngleSnapAmount = snapAmount->data.f;
            snapAmount->data.f = 0.0f;
            *g_fGamepadLookAngleSnapAmount = 0.0f;
        } 

        RE::UI* ui = RE::UI::GetSingleton();
        if (ui!=nullptr && ui->IsMenuOpen("WSActivateRollover"))
        {
            auto msgQ = RE::UIMessageQueue::GetSingleton();
            if (msgQ != nullptr) 
            {
                msgQ->AddMessage("WSActivateRollover", RE::UI_MESSAGE_TYPE::kHide, nullptr);
            }
        }
        
        CameraSwitchFunc(!defaultThirdPerson);
    }

    void PlayerSceneEnd() 
    {
        // Set VRIK settings back
        if (vrikInterface != nullptr) {
            vrikInterface->restoreSettings();
        }

        RE::Setting* snapAmount = RE::GetINISetting("fGamepadLookAngleSnapAmount:VRInput");
        if (snapAmount) {
            snapAmount->data.f = prevGamepadLookAngleSnapAmount;
            *g_fGamepadLookAngleSnapAmount = prevGamepadLookAngleSnapAmount;
        }
   
        auto ini = RE::INISettingCollection::GetSingleton();    
        if (ini) 
        {
            RE::Setting* pickLength = ini->GetSetting("fActivatePickLength:Interface");
            if (pickLength) 
            {
                pickLength->data.f = prevActivatePickLength;
                *g_fActivatePickLength = prevActivatePickLength;
            }

            RE::Setting* pickRadius = ini->GetSetting("fActivatePickRadius:Interface");
            if (pickRadius) 
            {
                pickRadius->data.f = prevActivatePickRadius;
                *g_fActivatePickRadius = prevActivatePickRadius;
            }

            RE::Setting* playerCollision = ini->GetSetting("bDisablePlayerCollision:Havok");
            if (playerCollision) {
                playerCollision->data.b = false;
                g_bDisablePlayerCollision = false;
            }
        }

        EnablePlayerControlsFunc(VM::GetSingleton(), 0, 0, true, true, true, false, true, false, true, false, 0);
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player != nullptr) player->SetAIDriven(false);
        //Game.EnablePlayerControls(true, true, true, false, true, false, true, false, 0)
        //Game.SetPlayerAIDriven(false)
    }

    void loadConfig() 
    {
        std::string filepath = "Data\\SKSE\\Plugins\\OStimVR.ini";

        std::ifstream file(filepath);

        if (!file.is_open()) {
            transform(filepath.begin(), filepath.end(), filepath.begin(), ::tolower);
            file.open(filepath);
        }

        if (file.is_open()) {
            std::string line;
            std::string currentSetting;
            while (std::getline(file, line)) {
                // trim(line);
                skipComments(line);
                trim(line);
                if (line.length() > 0) {
                    if (line.substr(0, 1) == "[") {
                        // newsetting
                        currentSetting = line;
                    } else {
                        if (currentSetting == "[Settings]") {
                            std::string variableName;
                            std::string variableValue = GetConfigSettingsValue(line, variableName);

                            if (variableName == "LockHeightToBody") 
                            {
                                lockHeightToBody = std::stoi(variableValue);
                            } 
                            else if (variableName == "TrackHands") 
                            {
                                trackHands = std::stoi(variableValue);
                            } 
                            else if (variableName == "TrackHead") 
                            {
                                trackHead = std::stoi(variableValue);
                            } else if (variableName == "HidePlayerHeadDistance") {
                                hidePlayerHeadDistance = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "NearDistance") {
                                nearDistance = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "LockHmdToBody") {
                                lockHmdToBody = std::stoi(variableValue);
                            } else if (variableName == "LockHmdMaxThreshold") {
                                lockHmdMaxThreshold = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "LockHmdMinThreshold") {
                                lockHmdMinThreshold = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "LockHmdSpeed") {
                                lockHmdSpeed = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "HeightAdjustSpeed") {
                                heightAdjustSpeed = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "OffsetForward") {
                                offsetForward = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "OffsetSideways") {
                                offsetSideways = std::strtof(variableValue.c_str(), 0);
                            } else if (variableName == "DefaultThirdPerson") {
                                defaultThirdPerson = std::stoi(variableValue);
                            }
                        }
                    }
                }
            }
        }
    }
}  // namespace OStimVR