#pragma once

#include "GameTable.h"

#include "Core/ThreadManager.h"
#include <array>
#include <atomic>
#include <cstring>

namespace GameLogic {
    void installHooks();
    void installHooksPostPost();

    struct IsThirdPerson {
    public:
        static bool thunk(RE::PlayerCamera* pthis) {
            // causes the SpeakSound console command to always think we're in third person
            return true;
        }

        static inline REL::Relocation<decltype(thunk)> func;

        static inline void Install() {
            REL::Relocation<std::uintptr_t> target{RELOCATION_ID(36541, 37542), REL::VariantOffset(0x132, 0x135, 0x132)};
            auto& trampoline = SKSE::GetTrampoline();
            SKSE::AllocTrampoline(14);

            func = trampoline.write_call<5>(target.address(), thunk);
        }
    };

    struct GetHeading {
        static float thunk(RE::Actor* pthis, void* a2) {
            // causes the SpeakSound console command to always think we're looking at the players face
            return -1.0f + RE::PlayerCamera::GetSingleton()->GetRuntimeData2().yaw; //for vr version
        }

        static inline REL::Relocation<decltype(thunk)> func;

        static inline void Install() {
            REL::Relocation<std::uintptr_t> target{RELOCATION_ID(36541, 37542), REL::VariantOffset(0x152, 0x155, 0x152)};
            auto& trampoline = SKSE::GetTrampoline();
            SKSE::AllocTrampoline(14);

            func = trampoline.write_call<5>(target.address(), thunk);
        }
    };

	//for VR version
    struct VRPlayerClipUpdate {
    public:
        static void thunk(RE::hkbClipGenerator* pthis, const RE::hkbContext& context, float timestep) {
            if (!REL::Module::IsVR() || !playerSceneActive.load() || !IsPlayerContext(context)) {
                func(pthis, context, timestep);
                return;
            }

            RE::hkaAnimationBinding* bindingBefore = pthis->binding;
            RE::hkaAnimation* animationBefore = bindingBefore && bindingBefore->animation ? bindingBefore->animation.get() : nullptr;
            const float oldTime = pthis->localTime;
            func(pthis, context, timestep);
            if (!animationBefore || pthis->binding != bindingBefore || !pthis->binding->animation || pthis->binding->animation.get() != animationBefore) return;

            const float newTime = pthis->localTime;
            if (newTime <= oldTime) return;

            for (const auto& track : animationBefore->annotationTracks) {
                for (const auto& annotation : track.annotations) {
                    const char* text = annotation.text.c_str();
                    if (annotation.time <= oldTime || annotation.time > newTime || !text || std::strcmp(text, "OStimClimax") != 0) continue;

                    auto thread = Threading::ThreadManager::GetSingleton()->getPlayerThread();
                    auto threadActor = thread ? thread->GetActor(GameAPI::GameActor::getPlayer()) : nullptr;
                    auto currentNode = thread ? thread->getCurrentNodeInternal() : nullptr;
                    auto graphActor = threadActor ? threadActor->getGraphActor() : nullptr;
                    if (!threadActor || !currentNode || !graphActor || threadActor->index < 0 || threadActor->index >= currentNode->actors.size() || !currentNode->hasActorTag(threadActor->index, "climaxing")) return;

                    int speed = graphActor->singleSpeed ? 0 : thread->getCurrentSpeed();
                    if (speed < 0 || speed >= currentNode->speeds.size()) return;

                    std::string expectedAnimation = currentNode->speeds[speed].animation + "_" + std::to_string(graphActor->animationIndex);
                    std::string currentAnimation = pthis->animationName.c_str() ? pthis->animationName.c_str() : "";
                    std::size_t slash = currentAnimation.find_last_of("\\/");
                    if (slash != std::string::npos) currentAnimation.erase(0, slash + 1);
                    if (currentAnimation.size() > 4 && _stricmp(currentAnimation.c_str() + currentAnimation.size() - 4, ".hkx") == 0) currentAnimation.resize(currentAnimation.size() - 4);
                    if (_stricmp(currentAnimation.c_str(), expectedAnimation.c_str()) != 0) return;

                    const std::uint64_t now = GetTickCount64();
                    const std::uint64_t last = lastClimaxTick.load();
                    if (now - last < 100) return;
                    lastClimaxTick.store(now);
                    threadActor->climax();
                    return;
                }
            }
        }

        static inline REL::Relocation<decltype(thunk)> func;
        static inline constexpr std::size_t idx = 0x05;

        static inline void Install() {
            if (REL::Module::IsVR()) stld::write_vfunc<RE::hkbClipGenerator, VRPlayerClipUpdate>();
        }

        static void SetPlayerSceneActive(bool active) {
            if (!REL::Module::IsVR()) return;
            if (active) {
                lastClimaxTick.store(0);
                RefreshPlayerContexts();
            }
            playerSceneActive.store(active);
        }

    private:
        static void RefreshPlayerContexts() {
            for (auto& character : playerCharacters) character.store(nullptr);

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return;

            RE::BSAnimationGraphManagerPtr graphManager;
            player->GetAnimationGraphManager(graphManager);
            if (!graphManager) return;

            const std::size_t count = (std::min)(playerCharacters.size(), static_cast<std::size_t>(graphManager->graphs.size()));
            for (std::size_t i = 0; i < count; i++) {
                if (graphManager->graphs[i]) playerCharacters[i].store(&graphManager->graphs[i]->characterInstance);
            }
        }

        static bool IsPlayerContext(const RE::hkbContext& context) {
            if (!context.character) 
				return false;

            for (auto& character : playerCharacters) {
                if (context.character == character.load()) return true;
            }
            return false;
        }

        static inline std::array<std::atomic<RE::hkbCharacter*>, 4> playerCharacters{};
        static inline std::atomic<bool> playerSceneActive{ false };
        static inline std::atomic<std::uint64_t> lastClimaxTick{ 0 };
    };

	struct PackageStart {
    public:
        static RE::TESPackage* thunk(RE::ExtraDataList* pthis, RE::Actor* actor) {
            if (actor && !actor->IsPlayerRef() && Threading::ThreadManager::GetSingleton()->findActor(actor)) {
                // I don't know if anything else of importance happens in the orig function, so we just call it to make sure
                PackageStartOrig(pthis, actor);
                return Util::LookupTable::OStimScenePackage;
            }

            return func(pthis, actor);
        }

        static inline REL::Relocation<decltype(thunk)> func;

        static inline void Install() {
            REL::Relocation<std::uintptr_t> target{RELOCATION_ID(36404, 37398), REL::VariantOffset(0x47, 0x47, 0x47)};
            auto& trampoline = SKSE::GetTrampoline();
            SKSE::AllocTrampoline(14);

            func = trampoline.write_branch<5>(target.address(), thunk);
        }

    private:
        inline static RE::TESPackage* PackageStartOrig(RE::ExtraDataList* pthis, RE::Actor* actor) {
            using func_t = decltype(PackageStartOrig);
            REL::Relocation<func_t> func{RELOCATION_ID(11918, 12057)};
            return func(pthis, actor);
        }
    };
}