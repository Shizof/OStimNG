#include "VRAPI/OstimVR.h"

namespace OStimVR 
{
    vrikPluginApi::IVrikInterface001* vrikInterface;
    PlanckPluginAPI::IPlanckInterface001* planckInterface;
    spellwheelPluginApi::ISpellWheelInterface001* spellWheelInterface;
    ControllerFixPluginApi::IControllerFixInterface001* controllerFixInterface;

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
    float lockHmdSpeed = 50.0f;

    // OStimVR Settings General
    int enableVRIKScaling = 1;
    float heightAdjustSpeed = 1.0f;
    int defaultThirdPerson = 0;

    int disablePLANCKduringScenes = 0;

    int showControllersInThirdPerson = 1;

    bool CurrentCameraFirstPerson = true;

    std::vector<RE::Actor*> ignoredActorsForAggressionList;

    std::unordered_map<std::string, OstimVRAlignment> sceneAlignmentMap;
    OstimVRAlignment globalAlignments;

    bool GetIsCameraFirstPerson()
    { 
        return CurrentCameraFirstPerson;
    }

    float ostimAlignmentX = 0.0f;
    float ostimAlignmentY = 0.0f;
    float ostimAlignmentZ = 0.0f;
    float ostimAlignmentR = 0.0f;

    double activeRagdollStartDistanceOrgValue = 50.0;
    double activeRagdollEndDistanceOrgValue = 60.0;

    void MovePlayerInThirdPersonStart(bool firstPerson) 
    {
        //SKSE::GetTaskInterface()->AddTask([firstPerson]() {
            OstimVRAlignment sceneAlignment;
            auto state = UI::UIState::GetSingleton();
            if (state) {
                if (state->currentThread) {
                    auto center = state->currentThread->getCenter();

                    if (firstPerson) {
                        auto player = RE::PlayerCharacter::GetSingleton();
                        if (player != nullptr && player->AsReference()) {
                            RE::NiPoint3 newPos = player->AsReference()->GetPosition();
                            newPos.x = center.x;
                            newPos.y = center.y;

                            player->SetRotationZ(center.r);
                            player->AsReference()->SetPosition(newPos);
                        }
                    } else {
                        float sin = std::sin(center.r);
                        float cos = std::cos(center.r);

                        auto player = RE::PlayerCharacter::GetSingleton();
                        if (player != nullptr && player->AsReference()) {
                            RE::NiPoint3 newPos = player->AsReference()->GetPosition();

                            // Forward displacement
                             newPos.x = newPos.x + (sin * 25.0f);
                             newPos.y = newPos.y + (cos * 25.0f);

                            // Side displacement
                            float sideDisplacement = 15.0f;
                            newPos.x += (-cos * sideDisplacement);
                            newPos.y += (sin * sideDisplacement);

                            player->SetRotationZ(center.r + 1.571f);
                            player->AsReference()->SetPosition(newPos);

                            // player->AsReference()->SetPosition(RE::NiPoint3(center.x, center.y, center.z));
                        }
                    }
                }
            }
        //});
    }

    void RotatePlayerInFirstPersonSwitch()
    { 
        //SKSE::GetTaskInterface()->AddTask([]() {
            OstimVRAlignment sceneAlignment;
            auto state = UI::UIState::GetSingleton();
            if (state) {
                if (state->currentThread) {
                    auto center = state->currentThread->getCenter();

                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (player != nullptr && player->AsReference()) {
                        const RE::NiPoint3 newPos = player->AsReference()->GetPosition();

                        player->SetRotationZ(center.r);
                        player->AsReference()->SetPosition(newPos);
                    }
                }
            }
        //});        
    }

    void ModifyAlignment()
    {
        OstimVRAlignment sceneAlignment;
        auto state = UI::UIState::GetSingleton();
        if (state) {
            auto currentNode = state->currentNode;
            if (currentNode && currentNode->isTransition == false) {
                if (sceneAlignmentMap.find(currentNode->scene_id) != sceneAlignmentMap.end()) {
                    sceneAlignment = sceneAlignmentMap[currentNode->scene_id];
                }
            }

            if (state->currentThread) {
                float sin = std::sin(state->currentThread->getCenter().r);
                float cos = std::cos(state->currentThread->getCenter().r);

                vrikInterface->setSettingDouble("lockRotationAngle", ostimAlignmentR * 57.295776f +
                                                                         globalAlignments.angleOffsetDegrees +
                                                                         sceneAlignment.angleOffsetDegrees);
                /*if (CurrentCameraFirstPerson) {
                    vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 2.0);
                }*/
                vrikInterface->setSettingDouble("lockRotation", 1);

                vrikInterface->setSettingDouble(
                    "lockPositionX", ostimAlignmentX +
                                         (cos * (globalAlignments.bodyOffsetX + sceneAlignment.bodyOffsetX)) +
                                         (sin * (globalAlignments.bodyOffsetY + sceneAlignment.bodyOffsetY)));
                vrikInterface->setSettingDouble(
                    "lockPositionY", ostimAlignmentY -
                                         (sin * (globalAlignments.bodyOffsetX + sceneAlignment.bodyOffsetX)) +
                                         (cos * (globalAlignments.bodyOffsetY + sceneAlignment.bodyOffsetY)));
                vrikInterface->setSettingDouble(
                    "lockPositionZ", ostimAlignmentZ + globalAlignments.bodyOffsetZ + sceneAlignment.bodyOffsetZ);
                vrikInterface->setSettingDouble("lockPosition", 2.0);  // Yes, this needs to be 2.0, which means specific X,Y,Z coordinates. 1.0 would be offsets.
                /*if (CurrentCameraFirstPerson) {
                    vrikInterface->setSettingDouble("lockHmdToBody", 1);
                }*/
                if (enableVRIKScaling) {
                    auto gameActors = state->currentThread->getGameActors();
                    for (int i = 0; i < gameActors.size(); i++) {
                        if (gameActors[i].isPlayer()) {
                            const float playerScale = gameActors[i].getScale();
                            vrikInterface->setSettingDouble("bodySize", playerScale);
                            vrikInterface->setSettingDouble("armSize", playerScale);
                            vrikInterface->setSettingDouble("armLength", 1.0f);

                            break;
                        }
                    }
                }
            }
        }
    }

    void VRIKLockPositionAndRotation(float rotSin, float rotCos, float x, float y, float z, float r, float playerScale)
    {
        ostimAlignmentX = x;
        ostimAlignmentY = y;
        ostimAlignmentZ = z;
        ostimAlignmentR = r;
        
        OstimVRAlignment sceneAlignment;
        auto state = UI::UIState::GetSingleton();
        if (state) {
            auto currentNode = state->currentNode;
            if (currentNode && currentNode->isTransition == false) {
                if (sceneAlignmentMap.find(currentNode->scene_id) != sceneAlignmentMap.end()) {
                    sceneAlignment = sceneAlignmentMap[currentNode->scene_id];
                }
            }
        }

        vrikInterface->setSettingDouble("lockRotationAngle", r * 57.295776f + globalAlignments.angleOffsetDegrees + sceneAlignment.angleOffsetDegrees);        
        /*if (CurrentCameraFirstPerson)
        {
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 2.0); 
        }*/
        vrikInterface->setSettingDouble("lockRotation", 1);

        vrikInterface->setSettingDouble("lockPositionX", x + (rotCos * (globalAlignments.bodyOffsetX + sceneAlignment.bodyOffsetX)) + (rotSin * (globalAlignments.bodyOffsetY + sceneAlignment.bodyOffsetY)));
        vrikInterface->setSettingDouble("lockPositionY", y - (rotSin * (globalAlignments.bodyOffsetX + sceneAlignment.bodyOffsetX)) + (rotCos * (globalAlignments.bodyOffsetY + sceneAlignment.bodyOffsetY)));
        vrikInterface->setSettingDouble("lockPositionZ", z + globalAlignments.bodyOffsetZ + sceneAlignment.bodyOffsetZ);
        vrikInterface->setSettingDouble("lockPosition", 2.0); //Yes, this needs to be 2.0, which means specific X,Y,Z coordinates. 1.0 would be offsets.
        /*if (CurrentCameraFirstPerson)
        {
            vrikInterface->setSettingDouble("lockHmdToBody", 1);        
        } */  
        if (enableVRIKScaling) 
        {
            vrikInterface->setSettingDouble("bodySize", playerScale);
            vrikInterface->setSettingDouble("armSize", playerScale);
            vrikInterface->setSettingDouble("armLength", 1.0f);
        }
    }

    void FixStandingBug()
    {
        Sleep(2000);
        // Fix for stand animation bug
        OStim::ThreadManager* threadManager = OStim::ThreadManager::GetSingleton();

        if (threadManager != nullptr && threadManager->playerThreadRunning()) {
            auto playerThread = threadManager->getPlayerThread();
            if (playerThread != nullptr) {
                playerThread->SetSpeed(playerThread->getSpeed());
            }
        }
    }

    void SetOstimVRSettings(bool firstPerson)
    {
        if (firstPerson) {
            MovePlayerInThirdPersonStart(firstPerson);
            //RotatePlayerInFirstPersonSwitch();

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
        } else {
            MovePlayerInThirdPersonStart(firstPerson);
        }

        if (!firstPerson) {
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
                
        ModifyAlignment();

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


            //?
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", firstPerson ? 2.0 : 0);
            vrikInterface->setSettingDouble("lockHmdToBody", firstPerson ? 1 : 0);
        }

        if (showControllersInThirdPerson) 
        {
            if (controllerFixInterface != nullptr)
            {
                controllerFixInterface->ForceShowControllers(!firstPerson);
            } 
            /*else 
            {
                RE::BSOpenVR* openVR = RE::BSOpenVR::GetSingleton();
                if (openVR && openVR->unk388[0].get() && openVR->unk388[1].get()) {
                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (player != nullptr) {
                        auto nodeData = player->GetVRNodeData();
                        if (nodeData) {
                            if (firstPerson) {
                                if (nodeData->LeftWandNode)
                                    nodeData->LeftWandNode->DetachChild(openVR->unk388[0].get());
                                if (nodeData->RightWandNode)
                                    nodeData->RightWandNode->DetachChild(openVR->unk388[1].get());
                            } else {
                                if (nodeData->LeftWandNode)
                                    nodeData->LeftWandNode->AttachChild(openVR->unk388[0].get());
                                if (nodeData->RightWandNode)
                                    nodeData->RightWandNode->AttachChild(openVR->unk388[1].get());
                            }
                        }
                    }
                }
            }*/
        }

        if (!firstPerson) {
            std::thread t1(FixStandingBug);
            t1.detach(); 
        }       
    }

    void SetVRIKHandTracking()
    {
        if (vrikInterface != nullptr) {
            if (CurrentCameraFirstPerson && trackHands) {
                vrikInterface->setSettingDouble("enableLeftArm", 1);
                vrikInterface->setSettingDouble("enableRightArm", 1);
                vrikInterface->setSettingDouble("enableInteractiveHands", 0);
            } else {
                vrikInterface->setSettingDouble("enableLeftArm", 0);
                vrikInterface->setSettingDouble("enableRightArm", 0);
            }
        }
    }

    void SetVRIKLockHeightToBody() 
    {
        if (vrikInterface != nullptr) {
            vrikInterface->setSettingDouble("lockHeightToBody", CurrentCameraFirstPerson ? 1.0 : lockHeightToBody);
        }
    }

    void CameraSwitchFunc(bool firstPerson)
    {
        if (firstPerson)
        {
            auto state = UI::UIState::GetSingleton();
            if (state) {
                if (state->currentThread) {
                    auto center = state->currentThread->getCenter();

                    if (firstPerson) {
                        auto player = RE::PlayerCharacter::GetSingleton();
                        if (player != nullptr && player->AsReference()) {
                            RE::NiPoint3 newPos = player->AsReference()->GetPosition();
                            float distSqr = newPos.GetSquaredDistance(RE::NiPoint3(center.x, center.y, center.z));
                            //logger::info("Dist: {}", distSqr);
                            if (distSqr > 25000.0f)
                            {
                                RE::DebugNotification("You need to be closer to your real body.");
                                return;
                            }
                        }
                    }
                }
            }
        }
        //logger::info("Applying {} settings", firstPerson ? "First Person" : "Third Person");

        CurrentCameraFirstPerson = firstPerson;

        SKSE::GetTaskInterface()->AddTask([firstPerson]() { SetOstimVRSettings(firstPerson); });
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
            
            vrikInterface->setSettingDouble("selfieModeEnabled", 0);
            vrikInterface->setSettingDouble("cameraOffsetting", 0);
            vrikInterface->setSettingDouble("enablePosture", 0);
            vrikInterface->setSettingDouble("enableBody", 0);
            vrikInterface->setSettingDouble("enableJumping", 0);
            vrikInterface->setSettingDouble("displayHolsters", 0);
            vrikInterface->setSettingDouble("nearClipDistance", nearDistance);
        }
        
        CameraSwitchFunc(!defaultThirdPerson);

        if (planckInterface != nullptr) {
            ignoredActorsForAggressionList.clear();
            auto state = UI::UIState::GetSingleton();
            if (state) {
                if (state->currentThread) {
                    auto gameActors = state->currentThread->getGameActors();
                    for (int i = 0; i < gameActors.size(); i++) {
                        if (gameActors[i].form != nullptr  && gameActors[i].form->formID != 0x14) {
                            planckInterface->AddAggressionIgnoredActor(gameActors[i].form);
                            ignoredActorsForAggressionList.emplace_back(gameActors[i].form);
                        }
                    }
                }
            }  
            if (disablePLANCKduringScenes)
            {
                planckInterface->GetSettingDouble("activeRagdollStartDistance", activeRagdollStartDistanceOrgValue);
                planckInterface->GetSettingDouble("activeRagdollEndDistance", activeRagdollEndDistanceOrgValue);

                planckInterface->SetSettingDouble("activeRagdollStartDistance", abs(activeRagdollStartDistanceOrgValue) * -1);
                planckInterface->SetSettingDouble("activeRagdollEndDistance", abs(activeRagdollEndDistanceOrgValue) * -1);
            }
        }
    }

    void AddRagdollCollisionIgnoredActors()
    {        
        if (disablePLANCKduringScenes) {
            planckInterface->GetSettingDouble("activeRagdollStartDistance", activeRagdollStartDistanceOrgValue);
            planckInterface->GetSettingDouble("activeRagdollEndDistance", activeRagdollEndDistanceOrgValue);

            planckInterface->SetSettingDouble("activeRagdollStartDistance", abs(activeRagdollStartDistanceOrgValue) * -1);
            planckInterface->SetSettingDouble("activeRagdollEndDistance", abs(activeRagdollEndDistanceOrgValue) * -1);
        }
    }

    void RemoveRagdollCollisionIgnoredActors()
    {
        if (planckInterface != nullptr) {
            planckInterface->GetSettingDouble("activeRagdollStartDistance", activeRagdollStartDistanceOrgValue);
            planckInterface->GetSettingDouble("activeRagdollEndDistance", activeRagdollEndDistanceOrgValue);

            planckInterface->SetSettingDouble("activeRagdollStartDistance", abs(activeRagdollStartDistanceOrgValue));
            planckInterface->SetSettingDouble("activeRagdollEndDistance", abs(activeRagdollEndDistanceOrgValue));
        }
    }

    void PlayerSceneEnd() 
    {
        // Set VRIK settings back
        if (vrikInterface != nullptr) {
            vrikInterface->restoreSettings();
        }

        // Set PLANCK setting back
        if (planckInterface != nullptr) {
            for (int i = 0; i < ignoredActorsForAggressionList.size(); i++) {
                if (ignoredActorsForAggressionList[i] != nullptr) {
                    planckInterface->RemoveAggressionIgnoredActor(ignoredActorsForAggressionList[i]);
                }
            }
            ignoredActorsForAggressionList.clear();
            RemoveRagdollCollisionIgnoredActors();
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

        if (showControllersInThirdPerson) {
            if (controllerFixInterface != nullptr) {
                controllerFixInterface->ForceShowControllers(false);
            } 
            /*else 
            {
                RE::BSOpenVR* openVR = RE::BSOpenVR::GetSingleton();
                if (openVR && openVR->unk388[0].get() && openVR->unk388[1].get()) {
                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (player != nullptr) {
                        auto nodeData = player->GetVRNodeData();
                        if (nodeData) {                            
                            if (nodeData->LeftWandNode)
                                nodeData->LeftWandNode->DetachChild(openVR->unk388[0].get());
                            if (nodeData->RightWandNode)
                                nodeData->RightWandNode->DetachChild(openVR->unk388[1].get());                            
                        }
                    }
                }
            }*/
        }

        CurrentCameraFirstPerson = !defaultThirdPerson;
    }

    void saveNewConfig() 
    {
        std::string filepath = "Data\\SKSE\\Plugins\\OStimVR.ini";

        std::ofstream output(filepath, std::fstream::out);
        if (!output.is_open()) {
            logger::error("...Failure while saving OStimVR.ini file.");
            return;
        }

        output << std::fixed;
        output << "################################################################################################\n";
        output << "# This is the config file for OStim VR mod VR Specific settings.\n";
        output << "#\n";
        output << "#\n";
        output << "# # ->This is the comment character.\n";
        output << "#\n";
        output << "################################################################################################\n";
        output << "[Settings]\n";
        output << "# OStimVR Settings 3rd person\n";
        output << "LockHeightToBody = 1          #When enabled, in 3rd person the hmd height will be the same as animation head height. This is always 1 for 1st person.\n";
        output << "\n";
        output << "# OStimVR Settings 1st person\n";
        output << "TrackHands = 1                #When enabled, your VRIK body hands will be attached to your controllers.\n";
        output << "\n";
        output << "NearDistance = 3.0            #Sets VRIK Neardistance value to this value automatically in Ostim VR scenes. Default is 3.0.\n";
        output << "\n";
        output << "LockHmdMaxThreshold = 20.0    #Lock HMD in place max threshold. May cause nausea at 0. You can "
                  "tighten it to prevent going off place. Default is 20.0.\n";
        output << "LockHmdMinThreshold = 2.0     #Lock HMD in place min threshold. May cause nausea at 0. Default is 2.0.\n";
        output << "LockHmdSpeed = 50.0           #Lock HMD in place speed. May cause nausea at high values if used with low min-max threshold values. You can increase it to make it faster to snap in place. Default is 50.0.\n";
        output << "\n";
        output << "# OStimVR Settings General\n";
        output << "EnableVRIKScaling = 1         #Apply Ostim scaling settings to VRIK body.\n";
        output << "\n";
        output << "DisablePLANCKduringScenes = 0   #Disable PLANCK collision during scenes.\n";
        output << "\n";
        output << "HeightAdjustSpeed = 1.0       #Snapback speed for viewpoint. Higher speeds may cause nausea. "
                  "Default is 1.0.\n";
        output << "\n";
        output << "ShowControllersInThirdPerson = 1 #Shows openvr controllers while in third person mode.\n ";
        output << "\n";
        output << "DefaultThirdPerson = 0        #If set to 1, scenes will start in third person camera. You can always switch during scenes using Ostim Wheel.\n";
        output << "\n";

        output.close();
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
                            else if (variableName == "EnableVRIKScaling") {
                                enableVRIKScaling = std::stoi(variableValue);
                            } 
                            else if (variableName == "ShowControllersInThirdPerson") {
                                showControllersInThirdPerson = std::stoi(variableValue);
                            }  
                            else if (variableName == "DefaultThirdPerson") {
                                defaultThirdPerson = std::stoi(variableValue);

                                CurrentCameraFirstPerson = !defaultThirdPerson;
                            } else if (variableName == "DisablePLANCKduringScenes") {
                                disablePLANCKduringScenes = std::stoi(variableValue);
                            } 
                        }
                    }
                }
            }
        } 
        else  // Regenerate new file
        {
            saveNewConfig();
        }
    }

    void GetGlobalOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset)
    {
        offsetX = globalAlignments.bodyOffsetX;
        offsetY = globalAlignments.bodyOffsetY;
        offsetZ = globalAlignments.bodyOffsetZ;
        rotationOffset = globalAlignments.angleOffsetDegrees;
    }

    void GetSceneOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset) 
    {
        auto state = UI::UIState::GetSingleton();
        if (state)
        {
            auto currentNode = state->currentNode;
            if (currentNode && currentNode->isTransition == false) 
            {
                if (sceneAlignmentMap.find(currentNode->scene_id) != sceneAlignmentMap.end()) 
                {
                    offsetX = sceneAlignmentMap[currentNode->scene_id].bodyOffsetX;
                    offsetY = sceneAlignmentMap[currentNode->scene_id].bodyOffsetY;
                    offsetZ = sceneAlignmentMap[currentNode->scene_id].bodyOffsetZ;
                    rotationOffset = sceneAlignmentMap[currentNode->scene_id].angleOffsetDegrees;
                }
            }
        }
    }

    void ModifyOffsetsOnNode(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset, Graph::Node* node)
    {
        if (node && node->isTransition == false) {
            if (sceneAlignmentMap.find(node->scene_id) != sceneAlignmentMap.end()) {
                sceneAlignmentMap[node->scene_id].bodyOffsetX = offsetX;
                sceneAlignmentMap[node->scene_id].bodyOffsetY = offsetY;
                sceneAlignmentMap[node->scene_id].bodyOffsetZ = offsetZ;
                sceneAlignmentMap[node->scene_id].angleOffsetDegrees = rotationOffset;
            } else {
                OstimVRAlignment sceneAlignment;
                sceneAlignment.bodyOffsetX = offsetX;
                sceneAlignment.bodyOffsetY = offsetY;
                sceneAlignment.bodyOffsetZ = offsetZ;
                sceneAlignment.angleOffsetDegrees = rotationOffset;
                sceneAlignmentMap[node->scene_id] = sceneAlignment;
            }
        }
    }

    void ModifyOffsets(float offsetX, float offsetY, float offsetZ, float rotationOffset, bool global) 
    {
        if (global)
        {
            globalAlignments.bodyOffsetX = offsetX;
            globalAlignments.bodyOffsetY = offsetY;
            globalAlignments.bodyOffsetZ = offsetZ;
            globalAlignments.angleOffsetDegrees = rotationOffset;
        }
        else
        {
            auto state = UI::UIState::GetSingleton();
            if (state) 
            {
                auto currentNode = state->currentNode;
                ModifyOffsetsOnNode(offsetX, offsetY, offsetZ, rotationOffset, currentNode);
            }
        }

        ModifyAlignment();
    }

    void loadGlobalAlignmentConfig() 
    {
        std::string filepath = "Data\\SKSE\\Plugins\\OStimVR_globalalignment.ini";

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
                        
                        std::string variableName;
                        std::string variableValue = GetConfigSettingsValue(line, variableName);

                        if (variableName == "AngleOffsetDegrees") {
                            globalAlignments.angleOffsetDegrees = std::strtof(variableValue.c_str(), 0);
                        } else if (variableName == "BodyOffsetX") {
                            globalAlignments.bodyOffsetX = std::strtof(variableValue.c_str(), 0);
                        } else if (variableName == "BodyOffsetY") {
                            globalAlignments.bodyOffsetY = std::strtof(variableValue.c_str(), 0);
                        } else if (variableName == "BodyOffsetZ") {
                            globalAlignments.bodyOffsetZ = std::strtof(variableValue.c_str(), 0);
                        }
                    }
                }
            }
        }
        else //Regenerate new file
        {
            saveGlobalAlignmentConfig();
        }
    }

    void saveGlobalAlignmentConfig() 
    {
        std::string filepath = "Data\\SKSE\\Plugins\\OStimVR_globalalignment.ini";

        std::ofstream output(filepath, std::fstream::out);
        if (!output.is_open()) 
        {
            logger::error("...Failure while saving global alignments to file.");
            return;
        }

        output << std::fixed;
        output << "################################################################################################\n";
        output << "# This is the config file for saving OStim VR global alignments.\n";
        output << "#\n";
        output << "# This file is generated automatically and will be written automatically.\n";
        output << "# DO NOT edit this file unless you know what you are doing.\n";
        output << "#\n";
        output << "################################################################################################\n";
        output << "\n";
        output << "AngleOffsetDegrees = " << floatToStr(globalAlignments.angleOffsetDegrees, 2) << " #Angle offset for player in scenes in degrees. Positive values are clockwise.\n";
        output << "\n";
        output << "BodyOffsetX = " << floatToStr(globalAlignments.bodyOffsetX, 2) << " #Sideways offset for player in scenes. Positive values are rightwards.\n";
        output << "\n";
        output << "BodyOffsetY = " << floatToStr(globalAlignments.bodyOffsetY, 2) << " #Forward offset for player in scenes. Positive values are forwards.\n";
        output << "\n";
        output << "BodyOffsetZ = " << floatToStr(globalAlignments.bodyOffsetZ, 2) << " #Vertical offset for player in scenes. Positive values are upwards.\n";
        output << "\n";        

        output.close();

        RE::DebugNotification("Global alignments saved.");
    }

    void loadSceneAlignmentsConfig() 
    {
        std::string filepath = "Data\\SKSE\\Plugins\\OStimVR_scenealignments.ini";

        std::ifstream file(filepath);

        sceneAlignmentMap.clear();

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
                        std::string variableName;
                        std::string variableValue = GetConfigSettingsValue(line, variableName);

						std::vector<std::string> splitted = splitTrimmed(variableValue, '|');

                        OstimVRAlignment sceneAlignment;
                        sceneAlignment.bodyOffsetX = splitted.size() >= 1 ? std::strtof(splitted[0].c_str(), 0) : 0.0f;
                        sceneAlignment.bodyOffsetY = splitted.size() >= 2 ? std::strtof(splitted[1].c_str(), 0) : 0.0f;
                        sceneAlignment.bodyOffsetZ = splitted.size() >= 3 ? std::strtof(splitted[2].c_str(), 0) : 0.0f;
                        sceneAlignment.angleOffsetDegrees = splitted.size() >= 4 ? std::strtof(splitted[3].c_str(), 0) : 0.0f;
                        sceneAlignmentMap[variableName] = sceneAlignment;
                    }
                }
            }
        } else  // Regenerate new file
        {
            saveSceneAlignmentsConfig();
        }
    }

    void saveSceneAlignmentsConfig() 
    {
        std::string filepath = "Data\\SKSE\\Plugins\\OStimVR_scenealignments.ini";

        std::ofstream output(filepath, std::fstream::out);
        if (!output.is_open()) {
            logger::error("...Failure while saving save alignments to file.");
            return;
        }

        output << std::fixed;
        output << "################################################################################################\n";
        output << "# This is the config file for saving OStim VR scene alignments.\n";
        output << "#\n";
        output << "# This file is generated automatically and will be written automatically.\n";
        output << "# DO NOT edit this file unless you know what you are doing.\n";
        output << "#\n";
        output << "# Format is: SceneId = OffsetX|OffsetY|OffsetZ|AngleOffset\n";
        output << "#\n";
        output << "################################################################################################\n";
        output << "\n";
        for (auto& alignment : sceneAlignmentMap)
        {
            output << alignment.first.c_str() << " = " 
                   << floatToStr(alignment.second.bodyOffsetX, 2) << "|"
                   << floatToStr(alignment.second.bodyOffsetY, 2) << "|"
                   << floatToStr(alignment.second.bodyOffsetZ, 2) << "|"
                   << floatToStr(alignment.second.angleOffsetDegrees, 2) << "\n";
            output << "\n";
        }

        output.close();

        RE::DebugNotification("Scene alignments saved.");
    }

    void PrintNodesTree(int depth, std::vector<Graph::Node*>& visitedList, Graph::Node* node) 
    { 
        if (node) 
        {
            auto text = std::string(depth, '.');
            logger::info("{}{} - {} - {} - transition:{} - navs:{}", text, node->scene_id, node->scene_name, node->modpack, node->isTransition, node->navigations.size());
            for (Graph::Navigation nav : node->navigations) {
                std::vector<std::string> nodeNames;
                for (Graph::Node* navNode : nav.nodes) {
                    nodeNames.emplace_back(navNode->scene_id);
                }
                logger::info("Navigation:{} - transition:{} - nodesCount:{} - nodes:{}", nav.description,
                             nav.isTransition, nav.nodes.size(), nav.nodes.size() > 0 ? join(nodeNames, ",") : "null");                
            }
            for (Graph::Navigation nav : node->navigations) 
            {
                for (Graph::Node* navNode : nav.nodes) 
                {
                    if (navNode && std::find(visitedList.begin(), visitedList.end(), navNode) == visitedList.end() && navNode->modpack == node->modpack) 
                    {
                        visitedList.emplace_back(navNode);
                        PrintNodesTree(depth + 1, visitedList, navNode);
                    }
                }
            }
        }
    }

    void GetSameSetNodes(std::vector<Graph::Node*>& visitedList, std::vector<Graph::Node*>& nodesList, Graph::Node* node, std::string nodeId) {
        if (node) 
        {
            if (node->isTransition == false && removeDigits(node->scene_id) == nodeId) {
                nodesList.emplace_back(node);
            }
            for (Graph::Navigation nav : node->navigations) 
            {
                for (Graph::Node* navNode : nav.nodes) {
                    if (navNode &&
                        std::find(visitedList.begin(), visitedList.end(), navNode) == visitedList.end() &&
                        navNode->modpack == node->modpack) {
                        visitedList.emplace_back(navNode);
                        GetSameSetNodes(visitedList, nodesList, navNode, nodeId);
                    }
                }
            }
        }
    }

    void saveSceneAlignmentsForAllSetConfig() 
    { 
        /*
        //Code to print nodes
        auto state = UI::UIState::GetSingleton();
        if (state) {
            auto currentNode = state->currentNode;
            if (currentNode && currentNode->isTransition == false) 
            {                
                logger::info("..NodesTree..");
                /// find all nodes from the same set and print to log
                std::vector<Graph::Node*> visitedNodesList;
                visitedNodesList.emplace_back(currentNode);
                PrintNodesTree(0, visitedNodesList, currentNode);
                logger::info("..........");
            }
        }*/

        //Copy the same scene alignment settings for others in the set
        auto state = UI::UIState::GetSingleton();
        if (state) {
            auto currentNode = state->currentNode;
            if (currentNode && currentNode->isTransition == false) 
            {
                if (sceneAlignmentMap.find(currentNode->scene_id) != sceneAlignmentMap.end()) 
                {
                    float offsetX = sceneAlignmentMap[currentNode->scene_id].bodyOffsetX;
                    float offsetY = sceneAlignmentMap[currentNode->scene_id].bodyOffsetY;
                    float offsetZ = sceneAlignmentMap[currentNode->scene_id].bodyOffsetZ;
                    float rotationOffset = sceneAlignmentMap[currentNode->scene_id].angleOffsetDegrees;

                    ///find all nodes from the same set and call
                    std::vector<Graph::Node*> visitedNodesList;
                    visitedNodesList.emplace_back(currentNode);
                    std::vector<Graph::Node*> nodesList;
                    GetSameSetNodes(visitedNodesList, nodesList, currentNode, removeDigits(currentNode->scene_id));
                    for (auto node : nodesList)
                    {
                        ModifyOffsetsOnNode(offsetX, offsetY, offsetZ, rotationOffset, node);
                    }
                }
            }
        }
        
        saveSceneAlignmentsConfig();
    }

}  // namespace OStimVR