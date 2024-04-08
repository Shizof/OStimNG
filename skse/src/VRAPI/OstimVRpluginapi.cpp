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
    OStim::ThreadManager* threadManager = OStim::ThreadManager::GetSingleton();

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
        //order is nodeId|title|description|imagePath|border
        std::string str= menuData.options[i].nodeId + "|" 
            + menuData.options[i].title + "|"
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
            sceneMenu->UpdateMenuData();
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
    OStim::ThreadManager* threadManager = OStim::ThreadManager::GetSingleton();

    if (threadManager != nullptr && threadManager->playerThreadRunning()) {
        auto playerThread = threadManager->getPlayerThread();
        if (playerThread != nullptr) {
            playerThread->setStopTimer(4000);
            playerThread->setAutoModeToMainStage();
        }
    }
}

bool OstimVRPluginAPI::OstimVRInterface001::InTransition() 
{
    auto state = UI::UIState::GetSingleton();
    auto currentNode = state->currentNode;

    bool isInTransition = !currentNode || (currentNode->isTransition || state->currentThread->isInSequence() ||
            (state->currentThread->getThreadFlags() & OStim::ThreadFlag::NO_PLAYER_CONTROL));

    if (!isInTransition)
    {
        UI::Scene::MenuData menuData;
        GetOstimSceneMenuData(menuData); 

        return (menuData.options.size() == 0);
    } else {
        return true;
    }
}
