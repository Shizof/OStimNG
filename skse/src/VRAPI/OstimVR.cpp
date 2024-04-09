#include "VRAPI/OstimVR.h"

namespace OStimVR 
{
    vrikPluginApi::IVrikInterface001* vrikInterface;
    spellwheelPluginApi::ISpellWheelInterface001* spellWheelInterface;

    bool iniSettingsSetBefore = false;
    float prevGamepadLookAngleSnapAmount = -1.0f;
    float prevActivatePickLength = 180.0f;
    float prevActivatePickRadius = 16.0f;

    float originalVRIKplayerHeight = -1.0f;
        
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
    float nearDistance = 3.0f;

    float lockHmdMinThreshold = 20.0f;
    float lockHmdMaxThreshold = 2.0f;
    float lockHmdSpeed = 20.0f;

    float angleOffsetDegrees = 0.0f;
    float bodyOffsetX = 0.0f;
    float bodyOffsetY = 0.0f;
    float bodyOffsetZ = 0.0f;

    // OStimVR Settings General
    float heightAdjustSpeed = 1.0f;
    int defaultThirdPerson = 0;

    bool CurrentCameraFirstPerson = true;

    bool GetIsCameraFirstPerson()
    { 
        return CurrentCameraFirstPerson;
    }

    void VRIKLockPositionAndRotation(float rotSin, float rotCos, float x, float y, float z, float r)
    {
        vrikInterface->setSettingDouble("lockRotationAngle", r * 57.295776f + angleOffsetDegrees);        
        if (CurrentCameraFirstPerson)
        {
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 2.0); 
        }
        vrikInterface->setSettingDouble("lockRotation", 1);

        vrikInterface->setSettingDouble("lockPositionX", x + (rotCos * bodyOffsetX) + (rotSin * bodyOffsetY));
        vrikInterface->setSettingDouble("lockPositionY", y - (rotSin * bodyOffsetX) + (rotCos * bodyOffsetY));
        vrikInterface->setSettingDouble("lockPositionZ", z + bodyOffsetZ);
        vrikInterface->setSettingDouble("lockPosition", 2.0); //Yes, this needs to be 2.0, which means specific X,Y,Z coordinates. 1.0 would be offsets.
        if (CurrentCameraFirstPerson)
        {
            vrikInterface->setSettingDouble("lockHmdToBody", 1);        
        }     
    }

    //
    void SetOstimVRSettings(bool firstPerson)
    {
        if (firstPerson) {
            EnablePlayerControlsFunc(VM::GetSingleton(), 0, 0, true, false, false, true, false, true, false, true, 0);
            DisablePlayerControlsFunc(VM::GetSingleton(), 0, 0, false, true, true, false, true, false, true, false, 0);
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player != nullptr) player->SetAIDriven(true);

            if (prevGamepadLookAngleSnapAmount >= 0) {
                RE::Setting* snapAmount = RE::GetINISetting("fGamepadLookAngleSnapAmount:VRInput");
                if (snapAmount) {
                    snapAmount->data.f = 0.0f;
                    *g_fGamepadLookAngleSnapAmount = 0.0f;
                }
            }

            auto ini = RE::INISettingCollection::GetSingleton();
            if (ini) {
                RE::Setting* playerCollision = ini->GetSetting("bDisablePlayerCollision:Havok");
                if (playerCollision) {
                    playerCollision->data.b = true;
                    *g_bDisablePlayerCollision = true;
                }
            }
        }

        if (vrikInterface != nullptr) {
            vrikInterface->setSettingDouble("lockHeightToBody", firstPerson ? 1.0 : lockHeightToBody);
            vrikInterface->setSettingDouble("heightAdjustSpeed", heightAdjustSpeed);

            if (firstPerson && trackHands) {
                vrikInterface->setSettingDouble("enableLeftArm", 1);
                vrikInterface->setSettingDouble("enableRightArm", 1);
                vrikInterface->setSettingDouble("enableInteractiveHands", 0);
            } else {
                vrikInterface->setSettingDouble("enableLeftArm", 0);
                vrikInterface->setSettingDouble("enableRightArm", 0);
            }

            // Shows head in third person mode and hides it in first person mode.
            vrikInterface->setSettingDouble("enableHead", firstPerson ? 0 : 1);
            vrikInterface->setSettingDouble("hidePlayerHeadDistance", firstPerson ? 0.0 : 12.0f);

            vrikInterface->setSettingDouble("lockHmdToBody", firstPerson ? 1 : 0);
            vrikInterface->setSettingDouble("lockHmdMinThreshold", firstPerson ? lockHmdMinThreshold : 500.0f);
            vrikInterface->setSettingDouble("lockHmdMaxThreshold", firstPerson ? lockHmdMaxThreshold : 500.0f);
            vrikInterface->setSettingDouble("lockHmdSpeed", firstPerson ? lockHmdSpeed : 20.0f);
        }

        if (!firstPerson) 
        {
            auto ini = RE::INISettingCollection::GetSingleton();
            if (ini) {
                RE::Setting* playerCollision = ini->GetSetting("bDisablePlayerCollision:Havok");
                if (playerCollision) {
                    playerCollision->data.b = true;
                    *g_bDisablePlayerCollision = true;
                }
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            if (player != nullptr) player->SetAIDriven(false);
            EnablePlayerControlsFunc(VM::GetSingleton(), 0, 0, true, false, false, true, false, true, false, true, 0);
            DisablePlayerControlsFunc(VM::GetSingleton(), 0, 0, false, true, true, false, true, false, true, false, 0);

            if (prevGamepadLookAngleSnapAmount >= 0) {
                RE::Setting* snapAmount = RE::GetINISetting("fGamepadLookAngleSnapAmount:VRInput");
                if (snapAmount) {
                    snapAmount->data.f = prevGamepadLookAngleSnapAmount;
                    *g_fGamepadLookAngleSnapAmount = prevGamepadLookAngleSnapAmount;
                }
            }
        }        
    }


    void CameraSwitchFunc(bool firstPerson)
    {
        logger::info("Applying {} settings", firstPerson ? "First Person" : "Third Person");

        CurrentCameraFirstPerson = firstPerson;
        SetOstimVRSettings(firstPerson);
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
        
        // Main VRIK Settings.
        if (vrikInterface != nullptr) {
            originalVRIKplayerHeight = vrikInterface->getSettingDouble("playerHeight");
            
            vrikInterface->setSettingDouble("cameraOffsetting", 0);
            vrikInterface->setSettingDouble("enablePosture", 0);
            vrikInterface->setSettingDouble("enableBody", 0);
            vrikInterface->setSettingDouble("enableJumping", 0);
            vrikInterface->setSettingDouble("displayHolsters", 0);
            vrikInterface->setSettingDouble("nearClipDistance", nearDistance);
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
                *g_bDisablePlayerCollision = false;
            }
        }

        EnablePlayerControlsFunc(VM::GetSingleton(), 0, 0, true, true, true, false, true, false, true, false, 0);
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player != nullptr) player->SetAIDriven(false);

        if (spellWheelInterface && spellWheelInterface->getBuildNumber() >= 10413)
            spellWheelInterface->CloseOstimWheels();
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

                            if (variableName == "LockHeightToBody") {
                                lockHeightToBody = std::stoi(variableValue);
                            } 
                            else if (variableName == "TrackHands") {
                                trackHands = std::stoi(variableValue);
                            } 
                            else if (variableName == "NearDistance") {
                                nearDistance = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "LockHmdMaxThreshold") {
                                lockHmdMaxThreshold = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "LockHmdMinThreshold") {
                                lockHmdMinThreshold = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "LockHmdSpeed") {
                                lockHmdSpeed = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "HeightAdjustSpeed") {
                                heightAdjustSpeed = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "DefaultThirdPerson") {
                                defaultThirdPerson = std::stoi(variableValue);
                            } 
                            else if (variableName == "AngleOffsetDegrees") {
                                angleOffsetDegrees = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "BodyOffsetX") {
                                bodyOffsetX = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "BodyOffsetY") {
                                bodyOffsetY = std::strtof(variableValue.c_str(), 0);
                            }
                            else if (variableName == "BodyOffsetZ") {
                                bodyOffsetZ = std::strtof(variableValue.c_str(), 0);
                            }
                        }
                    }
                }
            }
        }
    }
}  // namespace OStimVR