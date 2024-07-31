#include <stddef.h>

#include "Core.h"

#include "Core/ThreadInterface.h"
#include "Events/EventListener.h"
#include "GameLogic/GameHooks.h"
#include "GameLogic/GameTable.h"
#include "InterfaceSpec/IPluginInterface.h"
#include "InterfaceSpec/PluginInterface.h"
#include "Messaging/IMessages.h"
#include "Papyrus/Papyrus.h"
#include "PluginInterface/InterfaceExchangeMessage.h"
#include "PluginInterfaceImplementation/InterfaceMapImpl.h"
#include "Serial/Manager.h"
#include "UI/UIState.h"
#include "Util/RNGUtil.h"
#include "VRAPI/OstimVRpluginapi.h"
#include "VRAPI/OstimVR.h"

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    void UnspecificedSenderMessageHandler(SKSE::MessagingInterface::Message* a_msg) {        
        switch (a_msg->type) {            
            case OSAInterfaceExchangeMessage::kMessage_ExchangeInterface: {
                OSAInterfaceExchangeMessage* exchangeMessage = (OSAInterfaceExchangeMessage*)a_msg->data;
                exchangeMessage->interfaceMap = InterfaceMap::GetSingleton();
            } break;
            case OStim::InterfaceExchangeMessage::MESSAGE_TYPE: {
                OStim::InterfaceExchangeMessage* message = (OStim::InterfaceExchangeMessage*)a_msg->data;
                message->interfaceMap = Interface::InterfaceMapImpl::getSingleton();
            } break;
        }
    }

    void OstimVRMessageHandler(SKSE::MessagingInterface::Message* a_msg) {
        if (a_msg->type == OstimVRPluginAPI::OstimVRMessage::kMessage_GetInterface) 
        {    
            OstimVRPluginAPI::OstimVRMessage* ostimMessage = (OstimVRPluginAPI::OstimVRMessage*)a_msg->data;
            ostimMessage->GetApiFunction = OstimVRPluginAPI::GetApi;
            logger::info("Provided OstimVR plugin interface to {}", a_msg->sender);
        }
    }

    void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {        
        switch (a_msg->type) {
            case SKSE::MessagingInterface::kPostLoad: {
                auto message = SKSE::GetMessagingInterface();
                if (message) {
                    message->RegisterListener(nullptr, OstimVRMessageHandler);
                    logger::info("Registered OstimVR message handler");
                }

                SKSE::GetNiNodeUpdateEventSource()->AddEventSink(Events::EventListener::GetSingleton());
                SKSE::GetCrosshairRefEventSource()->AddEventSink(Events::EventListener::GetSingleton());

                Core::postLoad();                
            } break;
            case SKSE::MessagingInterface::kPostPostLoad: {
                OStimVR::vrikInterface = vrikPluginApi::getVrikInterface001();
                if (OStimVR::vrikInterface) {
                    logger::info("Got VRIK interface");
                } else {
                    logger::info("Did not get VRIK interface");
                }
                OStimVR::planckInterface = PlanckPluginAPI::GetPlanckInterface001();
                if (OStimVR::planckInterface) {
                    logger::info("Got PLANCK interface");
                } else {
                    logger::info("Did not get PLANCK interface");
                }
                OStimVR::spellWheelInterface = spellwheelPluginApi::getSpellWheelInterface001();
                if (OStimVR::spellWheelInterface) {
                    logger::info("Got Spell Wheel VR interface");
                } else {
                    logger::info("Did not get Spell Wheel VR interface");
                }
                OStimVR::controllerFixInterface = ControllerFixPluginApi::getControllerFixInterface001();
                if (OStimVR::controllerFixInterface) {
                    logger::info("Got Controller Fix VR interface");
                } else {
                    logger::info("Did not get Controller Fix VR interface");
                }

                Core::postpostLoad();

                // we are installing this hook so late because we need it to overwrite the PapyrusUtil hook
                GameLogic::installHooksPostPost();
            } break;
            case SKSE::MessagingInterface::kInputLoaded: {
                RE::BSInputDeviceManager::GetSingleton()->AddEventSink(Events::EventListener::GetSingleton());
            } break;
            case SKSE::MessagingInterface::kDataLoaded: {
                GameLogic::GameTable::setup();

                UI::PostRegisterMenus();

                OStimVR::loadConfig();
                OStimVR::loadSceneAlignmentsConfig();
                OStimVR::loadGlobalAlignmentConfig();

                Core::dataLoaded();
                SKSE::GetTaskInterface()->AddTask([]() { Core::postDataLoaded(); });
            } break;
            case SKSE::MessagingInterface::kPostLoadGame: {
                Events::EventListener::handleGameLoad();
                Core::newSession();
                Core::sessionStarted();

                if ((bool)(a_msg->data) == true) {
                    if (OStimVR::playerInScene)
                    {
                        OStimVR::PlayerSceneEnd();
                    }
                }
            } break;
            case SKSE::MessagingInterface::kNewGame: {
                Events::EventListener::handleGameLoad();
                Core::sessionLoaded();
                Core::sessionStarted();
            } break;

        }
    }
}  // namespace

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;
    v.PluginVersion(REL::Version("7.3.1.1"sv));
    v.PluginName("OStim");
    v.AuthorName("VersuchDrei");
    v.UsesAddressLibrary(true);
    v.HasNoStructUse(true);

    return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "OStim";
    a_info->version = 0x07030011;

    return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);

    Init(skse);

    InterfaceMap::GetSingleton()->AddInterface("Messaging", Messaging::MessagingRegistry::GetSingleton());
    InterfaceMap::GetSingleton()->AddInterface("Threads", Interfaces::ThreadInterface::GetSingleton());

    auto message = SKSE::GetMessagingInterface();
    if (!message->RegisterListener(MessageHandler)) {
        return false;
    }
    if (!message->RegisterListener(nullptr, UnspecificedSenderMessageHandler)) {
        logger::warn("Plugin Interface wasn't initialized.");
    }

    Papyrus::Bind();
    UI::Settings::LoadSettings();

    GameLogic::installHooks();

    const auto serial = SKSE::GetSerializationInterface();
    serial->SetUniqueID(_byteswap_ulong('OST'));
    serial->SetSaveCallback(Serialization::Save);
    serial->SetLoadCallback(Serialization::Load);
    serial->SetRevertCallback(Serialization::Revert);

    UI::RegisterMenus();

    Core::init();
    
    log::info("{} has finished loading.", plugin->GetName());
    return true;
}