#include "VRAPI/OstimVRpluginapi.h"

// Interface classes are stored statically
OstimVRPluginAPI::OstimVRInterface001 g_interface001;

// Constructs and returns an API of the revision number requested
void* OstimVRPluginAPI::GetApi(unsigned int revisionNumber) { 
    return &g_interface001;
}

// Handles skse mod messages requesting to fetch API functions from OstimVR
void OstimVRPluginAPI::ModMessageHandler(SKSE::MessagingInterface::Message* message) 
{    
    if (message->type == OstimVRMessage::kMessage_GetInterface) 
    {
        OstimVRMessage* ostimMessage = (OstimVRMessage*)message->data;
        ostimMessage->GetApiFunction = GetApi;
        logger::info("Provided OstimVR plugin interface to {}", message->sender);
    }
}

// Fetches the version number
unsigned int OstimVRPluginAPI::OstimVRInterface001::getBuildNumber() { 
    return OSTIMVR_BUILD_NUMBER; 
}

bool OstimVRPluginAPI::OstimVRInterface001::IsPlayerOstimScenePlaying() 
{ 
    Threading::ThreadManager* threadManager = Threading::ThreadManager::GetSingleton();

    return threadManager !=nullptr && threadManager->playerThreadRunning();
}

void GetOstimSceneMenuData(UI::Scene::MenuData& menuData) 
{
    UI::Scene::SceneMenu* sceneMenu = UI::Scene::SceneMenu::GetMenu();
    if (sceneMenu != nullptr) {
        if (sceneMenu->IsOptionsOpen()) {
            UI::Scene::SceneOptions* sceneOptions = UI::Scene::SceneOptions::GetSingleton();
            if (sceneOptions != nullptr) sceneOptions->BuildMenuData(menuData);
        } else {
            // Make sure SceneMenu::BuildMenuData is not private or this would give an error.
            sceneMenu->BuildMenuData(menuData);
        }
    }
}

void OstimVRPluginAPI::OstimVRInterface001::GetMenuData(char ** outStr, int & outStrLength) 
{ 
    UI::Scene::MenuData menuData;
    GetOstimSceneMenuData(menuData);        

    std::string options = "";

    // Converting menuData to string here (doing it this way instead of vector or std::string etc. so that older c++ versions can read it)
    for (int i = 0; i < menuData.options.size(); i++) 
    {
        //order is nodeId|description|imagePath|border
        std::string str= menuData.options[i].nodeId + "|" 
        + menuData.options[i].description + "|"
        + menuData.options[i].imagePath + "|"
        + menuData.options[i].border;

        options = options + str;
        if (i < menuData.options.size() - 1)
        {
            options = options + ";";
        }
    }

    outStrLength = std::min((int)options.length(), 65536);
    if (outStrLength > 0 && *outStr!=nullptr) {
        strcpy(*outStr, options.c_str());
    }
}

void OstimVRPluginAPI::OstimVRInterface001::SelectOption(const char* id) 
{
    auto sceneMenu = UI::Scene::SceneMenu::GetMenu();
    if (sceneMenu != nullptr) 
    {
        if (strcmp(id, "options") == 0)
        {
            sceneMenu->SetOptionsOpen(true);
            //sceneMenu->UpdateMenuData();
        } 
        else 
        {
            if (sceneMenu->IsOptionsOpen()) 
            {
                sceneMenu->HandleOption(id);
            } 
            else 
            {
                sceneMenu->ChangeAnimation(id);
            }
        }
    }
}

bool OstimVRPluginAPI::OstimVRInterface001::IsInOptionsMode() 
{
    auto sceneMenu = UI::Scene::SceneMenu::GetMenu();
    return (sceneMenu != nullptr && sceneMenu->IsOptionsOpen());
}


void OstimVRPluginAPI::OstimVRInterface001::ChangeSpeed(bool increaseSpeed) 
{ 
    auto sceneMenu = UI::Scene::SceneMenu::GetMenu(); 
    if (sceneMenu != nullptr)
    {
        sceneMenu->ChangeSpeed(increaseSpeed);
    }
}

void OstimVRPluginAPI::OstimVRInterface001::SwitchCamera(bool firstperson) 
{ 
    OStimVR::CameraSwitchFunc(firstperson); 
}

bool OstimVRPluginAPI::OstimVRInterface001::IsCameraFirstPerson() 
{ 
    return OStimVR::GetIsCameraFirstPerson(); 
}

void OstimVRPluginAPI::OstimVRInterface001::EndSceneEarly() 
{

    SKSE::GetTaskInterface()->AddTask([]() { 
        
        if (OStimVR::CurrentCameraFirstPerson==false && OStimVR::TooDistToRealBodyCheck()) {
            //We switch to first person first to prevent bugs
            OStimVR::CurrentCameraFirstPerson = true;
            OStimVR::SetOstimVRSettings(true);
        }
        
        Threading::ThreadManager* threadManager = Threading::ThreadManager::GetSingleton();

        if (threadManager != nullptr && threadManager->playerThreadRunning()) {
            auto playerThread = threadManager->getPlayerThread();
            if (playerThread != nullptr) {
                playerThread->setStopTimer(4000);
                playerThread->setAutoModeToMainStage();
            }
        }
        
        });    
}

bool OstimVRPluginAPI::OstimVRInterface001::InTransition() 
{
    auto state = UI::UIState::GetSingleton();
    auto currentNode = state->currentNode;

    return currentNode && currentNode->isTransition;

    //check if data exists
       /* UI::Scene::MenuData menuData;
        GetOstimSceneMenuData(menuData); 

        return (menuData.options.size() == 0);
    */
}

bool OstimVRPluginAPI::OstimVRInterface001::InSequence() {
    auto state = UI::UIState::GetSingleton();
        
    return (state && state->currentThread && (state->currentThread->isInSequence() ||
            ((state->currentThread && state->currentThread->getThreadFlags() & Threading::ThreadFlag::NO_PLAYER_CONTROL))));
        //}
}

void OstimVRPluginAPI::OstimVRInterface001::GetSceneOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset) 
{
    OStimVR::GetSceneOffsets(offsetX, offsetY, offsetZ, rotationOffset);
}

void OstimVRPluginAPI::OstimVRInterface001::GetGlobalOffsets(float& offsetX, float& offsetY, float& offsetZ, float& rotationOffset) 
{
    OStimVR::GetGlobalOffsets(offsetX, offsetY, offsetZ, rotationOffset);
}

void OstimVRPluginAPI::OstimVRInterface001::ModifyOffsets(float offsetX, float offsetY, float offsetZ, float rotationOffset, bool global) 
{
    OStimVR::ModifyOffsets(offsetX, offsetY, offsetZ, rotationOffset, global);
}

void OstimVRPluginAPI::OstimVRInterface001::SaveGlobalOffsets() 
{ 
    OStimVR::saveGlobalAlignmentConfig(); 
}

void OstimVRPluginAPI::OstimVRInterface001::SaveSceneOffsets() { OStimVR::saveSceneAlignmentsConfig(); }

void OstimVRPluginAPI::OstimVRInterface001::SaveSceneOffsetsForAllSet() 
{ 
    OStimVR::saveSceneAlignmentsForAllSetConfig();
}

void OstimVRPluginAPI::OstimVRInterface001::GetExcitements(float& domRatio, float& subRatio, float& thirdRatio,
                                                           bool& domEnabled, bool& subEnabled, bool& thirdEnabled) {
    Threading::ThreadManager* threadManager = Threading::ThreadManager::GetSingleton();

    if (threadManager != nullptr && threadManager->playerThreadRunning()) 
    {
        auto playerThread = threadManager->getPlayerThread();
        if (playerThread != nullptr) 
        {
            auto domActor = playerThread->GetActor(0);
            auto subActor = playerThread->GetActor(1);
            auto thirdActor = playerThread->GetActor(2);

            if (domActor != nullptr) {
                domEnabled = true;
                domRatio = clamp(domActor->getExcitement() / 100.0f, 0.0f, 1.0f);
            } 
            else {
                domEnabled = false;
                domRatio = 0.0f;
            }
            if (subActor != nullptr) {
                subEnabled = true;
                subRatio = clamp(subActor->getExcitement() / 100.0f, 0.0f, 1.0f);
            } 
            else {
                subEnabled = false;
                subRatio = 0.0f;
            }
            if (thirdActor != nullptr) {
                thirdEnabled = true;
                thirdRatio = clamp(thirdActor->getExcitement() / 100.0f, 0.0f, 1.0f);
            } 
            else {
                thirdEnabled = false;
                thirdRatio = 0.0f;
            }
        }
    }
}


bool OstimVRPluginAPI::OstimVRInterface001::IsPLANCKCollisionsEnabled() {
    return OStimVR::disablePLANCKduringScenes == false;
};

bool OstimVRPluginAPI::OstimVRInterface001::IsHandTrackingEnabled() { return OStimVR::trackHands; };

bool OstimVRPluginAPI::OstimVRInterface001::IsLockHeightToBodyEnabled() { return OStimVR::lockHeightToBody; };

void OstimVRPluginAPI::OstimVRInterface001::TogglePLANCKMode() 
{ 
    OStimVR::disablePLANCKduringScenes = OStimVR::disablePLANCKduringScenes == false;
    if (OStimVR::disablePLANCKduringScenes)
    {
        OStimVR::AddRagdollCollisionIgnoredActors();
    }
    else {
        OStimVR::RemoveRagdollCollisionIgnoredActors();
    }
}

void OstimVRPluginAPI::OstimVRInterface001::ToggleHandTrackingMode() 
{
    OStimVR::trackHands = OStimVR::trackHands == false;
    OStimVR::SetVRIKHandTracking();
    OStimVR::ShowHideControllersFunc(true);
}


void OstimVRPluginAPI::OstimVRInterface001::ToggleLockHeightToBodyMode() {
    OStimVR::lockHeightToBody = OStimVR::lockHeightToBody == false;
    OStimVR::SetVRIKLockHeightToBody();
}

void OstimVRPluginAPI::OstimVRInterface001::OStimWheelOpenCloseEvent(bool opened, bool leftWheel)
{
    OStimVR::ShowHideControllersFunc(opened);
}

bool OstimVRPluginAPI::OstimVRInterface001::IsInAutoMode() {
    Threading::ThreadManager* threadManager = Threading::ThreadManager::GetSingleton();

    if (threadManager != nullptr && threadManager->playerThreadRunning()) {
        auto playerThread = threadManager->getPlayerThread();
        if (playerThread != nullptr) {
            return playerThread->isInAutoMode();
        }
    }
    return false;
}

void OstimVRPluginAPI::OstimVRInterface001::StartStopAutoMode(bool start)
{
    Threading::ThreadManager* threadManager = Threading::ThreadManager::GetSingleton();

    if (threadManager != nullptr && threadManager->playerThreadRunning()) 
    {
        auto playerThread = threadManager->getPlayerThread();
        if (playerThread != nullptr) 
        {
            if (start)
            {
                if (playerThread->isInAutoMode() == false)
                    playerThread->startAutoMode();
            }
            else 
            {
                if (playerThread->isInAutoMode())
                    playerThread->stopAutoMode();
            }
        }
    }
}
