#include "VRAPI/OstimVR.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace OStimVR 
{
    vrikPluginApi::IVrikInterface001* vrikInterface;
    PlanckPluginAPI::IPlanckInterface001* planckInterface;
    HiggsPluginAPI::IHiggsInterface001* higgsInterface;
    spellwheelPluginApi::ISpellWheelInterface001* spellWheelInterface;
    ControllerFixPluginApi::IControllerFixInterface001* controllerFixInterface;

    bool iniSettingsSetBefore = false;
    float prevGamepadLookAngleSnapAmount = -1.0f;
    float prevActivatePickLength = 180.0f;
    float prevActivatePickRadius = 16.0f;

    float originalVRIKplayerHeight = -1.0f;

    namespace fs = std::filesystem;

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

    float lockHmdMinThreshold = 2.0f;
    float lockHmdMaxThreshold = 20.0f;
    float lockHmdSpeed = 50.0f;

    // OStimVR Settings General
    int enableVRIKScaling = 1;
    float heightAdjustSpeed = 1.0f;
    int defaultThirdPerson = 0;

    int disablePLANCKduringScenes = 0;

    int ChangeHeadForwardDistance = 0;
    float HeadForwardDistance = 0.0f;
    float orgHeadForwardDistance = 10.0f;

    int ChangeHeadAboveDistance = 0;
    float HeadAboveDistance = 16.0f;
    float orgHeadAboveDistance = 13.0f;

    int showControllersInFirstPerson = 1;
    int showControllersInThirdPerson = 1;

    bool CurrentCameraFirstPerson = true;

    int hideVRIKCompass = 1;

    std::vector<RE::Actor*> ignoredActorsForAggressionList;

    std::unordered_map<std::string, OstimVRAlignment> sceneAlignmentMap;
    OstimVRAlignment globalAlignments;

    bool playerInScene = false;

    bool GetIsCameraFirstPerson() { return CurrentCameraFirstPerson; }

    float ostimAlignmentX = 0.0f;
    float ostimAlignmentY = 0.0f;
    float ostimAlignmentZ = 0.0f;
    float ostimAlignmentR = 0.0f;

    double activeRagdollStartDistanceOrgValue = 50.0;
    double activeRagdollEndDistanceOrgValue = 60.0;

    bool originalbDirectMovementWithWands = true;

    double higgsFarCastDistanceOrgValue = 5.0;

    int disableGravityGloves = 0;

    bool FBTExists = false;

    bool firstPersonBaseRotationValid = false;
    float firstPersonBaseRotation = 0.0f;

    bool sceneSnapWalkBaselineValid = false;
    RE::NiTransform sceneSnapWalkBaseline;
    bool sceneHeightOffsetBaselineValid = false;
    RE::NiMatrix3 sceneHeightOffsetBaselineRotation;
    std::uint32_t cameraTransitionGeneration = 0;
    std::uint32_t hmdPositionTransitionGeneration = 0;

    struct PlayerAnimObjectAttachment
    {
        RE::NiPointer<RE::NiNode> parent;
        RE::NiPointer<RE::NiAVObject> object;
        RE::NiPointer<RE::NiAVObject> transformSource;
        RE::NiTransform transformSourceLocal;
        bool useTransformSource = false;
    };

    std::unordered_map<std::string, std::vector<std::string>> playerAnimObjectEventMap;
    std::vector<PlayerAnimObjectAttachment> playerAnimObjectAttachments;
    std::atomic<std::uint32_t> playerAnimObjectGeneration{0};
    std::atomic<bool> playerAnimObjectUpdateQueued{false};

    std::string NormalizePlayerAnimObjectString(std::string value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r\n");
        value = value.substr(first, last - first + 1);
        std::ranges::transform(value, value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    bool IsPlayerAnimObjectList(const fs::path& path)
    {
        if (!path.has_filename() || NormalizePlayerAnimObjectString(path.extension().string()) != ".txt") {
            return false;
        }
        const auto name = NormalizePlayerAnimObjectString(path.filename().string());
        return (name.starts_with("fnis_") && name.ends_with("_list.txt")) ||
               (name.starts_with("att_") && name.ends_with("_animlist.txt"));
    }

    bool EndsWithHKX(std::string value)
    {
        std::ranges::transform(value, value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value.ends_with(".hkx");
    }

    void ParsePlayerAnimObjectList(const fs::path& path)
    {
        std::ifstream input(path);
        if (!input.is_open()) {
            return;
        }

        std::string line;
        while (std::getline(input, line)) {
            const auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '\'') {
                continue;
            }

            std::istringstream stream(line);
            std::vector<std::string> tokens;
            for (std::string token; stream >> token;) {
                if (!token.empty() && token.front() == '\'') {
                    break;
                }
                tokens.push_back(std::move(token));
            }

            const auto hkx = std::ranges::find_if(tokens, EndsWithHKX);
            if (hkx == tokens.end() || hkx == tokens.begin() || std::next(hkx) == tokens.end()) {
                continue;
            }

            const auto eventName = NormalizePlayerAnimObjectString(*std::prev(hkx));
            auto& objects = playerAnimObjectEventMap[eventName];
            for (auto it = std::next(hkx); it != tokens.end(); ++it) {
                std::string editorID = *it;
                if (const auto slash = editorID.find('/'); slash != std::string::npos) {
                    editorID.resize(slash);
                }
                if (editorID.empty()) {
                    continue;
                }
                if (std::ranges::find(objects, editorID) == objects.end()) {
                    objects.push_back(std::move(editorID));
                }
            }
        }
    }

    void BuildPlayerAnimObjectEventMap()
    {
        playerAnimObjectEventMap.clear();

        std::error_code error;
        const auto root = fs::current_path() / "Data" / "meshes" / "actors" / "character" / "animations";
        if (!fs::exists(root, error)) {
            return;
        }

        const auto options = fs::directory_options::skip_permission_denied;
        for (fs::recursive_directory_iterator it(root, options, error), end; it != end; it.increment(error)) {
            if (error) {
                error.clear();
                continue;
            }

            if (it->is_regular_file(error) && IsPlayerAnimObjectList(it->path())) {
                ParsePlayerAnimObjectList(it->path());
            }
        }
    }

    bool GetPlayerWeaponBindTransform(RE::PlayerCharacter* player, RE::NiAVObject* actor3D, RE::NiPointer<RE::NiAVObject>& transformSource, RE::NiTransform& weaponLocal)
    {
        if (!player || !actor3D) {
            return false;
        }

        auto actorBase = player->GetActorBase();
        auto race = actorBase ? actorBase->GetRace() : nullptr;
        if (!actorBase || !race) {
            return false;
        }

        const char* skeletonPath = race->skeletonModels[actorBase->GetSex()].GetModel();
        if (!skeletonPath || skeletonPath[0] == '\0') {
            return false;
        }

        RE::NiPointer<RE::NiNode> skeletonModel;
        const RE::BSModelDB::DBTraits::ArgsType args{};
        if (RE::BSModelDB::Demand(skeletonPath, skeletonModel, args) != RE::BSResource::ErrorCode::kNone || !skeletonModel) {
            return false;
        }

        auto bindWeapon = skeletonModel->GetObjectByName(RE::BSFixedString("WEAPON"));
        if (!bindWeapon || !bindWeapon->parent || bindWeapon->parent->name.c_str()[0] == '\0') {
            return false;
        }

        auto liveParent = actor3D->GetObjectByName(RE::BSFixedString(bindWeapon->parent->name.c_str()));
        if (!liveParent) {
            return false;
        }

        transformSource = RE::NiPointer<RE::NiAVObject>(liveParent);
        weaponLocal = bindWeapon->local;
        return true;
    }

    void RefreshPlayerAnimObject(RE::NiAVObject* object, RE::NiNode* parent)
    {
        if (!object) {
            return;
        }

        object->CullNode(false);

        auto& flags = object->GetFlags();
        flags.reset(RE::NiAVObject::Flag::kSelectiveUpdate);
        flags.reset(RE::NiAVObject::Flag::kSelectiveUpdateTransforms);
        flags.reset(RE::NiAVObject::Flag::kSelectiveUpdateController);
        flags.reset(RE::NiAVObject::Flag::kSelectiveUpdateRigid);
        flags.reset(RE::NiAVObject::Flag::kSelectiveUpdateTransformsOverride);
        flags.set(RE::NiAVObject::Flag::kForceUpdate);

        RE::NiUpdateData updateData{0.0F, RE::NiUpdateData::Flag::kDirty};
        object->UpdateWorldData(&updateData);
        object->UpdateDownwardPass(updateData, 0);
        object->UpdateWorldBound();

        if (parent) {
            parent->UpdateWorldBound();
        }
    }

    void SyncPlayerAnimObject(PlayerAnimObjectAttachment& attachment)
    {
        auto object = attachment.object.get();
        auto parent = attachment.parent.get();
        if (!object || !parent || object->parent != parent) {
            return;
        }

        object->previousWorld = object->world;
        if (attachment.useTransformSource && attachment.transformSource) {
            const auto objectWorld = attachment.transformSource->world * attachment.transformSourceLocal;
            object->local = parent->world.Invert() * objectWorld;
            object->world = objectWorld;
        } 
		else {
            object->world = parent->world * object->local;
        }

        object->CullNode(false);

        RE::NiUpdateData updateData{0.0F, RE::NiUpdateData::Flag::kDirty};
        if (auto node = object->AsNode()) {
            for (auto& child : node->GetChildren()) {
                if (child) {
                    child->UpdateDownwardPass(updateData, 0);
                }
            }
        }

        object->UpdateWorldBound();
        parent->UpdateWorldBound();
    }

    void QueuePlayerAnimObjectUpdate()
    {
        bool expected = false;
        if (!playerAnimObjectUpdateQueued.compare_exchange_strong(expected, true)) {
            return;
        }

        SKSE::GetTaskInterface()->AddTask([]() {
            playerAnimObjectUpdateQueued.store(false);
            if (!playerInScene) {
                return;
            }

            for (auto& attachment : playerAnimObjectAttachments) {
                if (attachment.object && attachment.parent) {
                    SyncPlayerAnimObject(attachment);
                }
            }
        });
    }

    void ClearPlayerAnimObjectsNow()
    {
        for (auto& attachment : playerAnimObjectAttachments) {
            if (attachment.parent && attachment.object && attachment.object->parent == attachment.parent.get()) {
                attachment.parent->DetachChild(attachment.object.get());
                attachment.parent->UpdateWorldBound();
            }
        }

        playerAnimObjectAttachments.clear();
    }

    void AttachPlayerAnimObject(const std::string& editorID)
    {
        auto form = RE::TESForm::LookupByEditorID<RE::TESObjectANIO>(editorID);
        if (!form) {
            return;
        }

        const char* modelPath = form->GetModel();
        if (!modelPath || modelPath[0] == '\0') {
            return;
        }

        RE::NiPointer<RE::NiNode> loadedModel;
        const RE::BSModelDB::DBTraits::ArgsType args{};
        const auto error = RE::BSModelDB::Demand(modelPath, loadedModel, args);
        if (error != RE::BSResource::ErrorCode::kNone || !loadedModel) {
            return;
        }

        RE::NiPointer<RE::NiAVObject> modelClone{netimmerse_cast<RE::NiAVObject*>(loadedModel->Clone())};
        auto modelNode = modelClone ? modelClone->AsNode() : nullptr;
        if (!modelNode) {
            return;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto actor3D = player->Get3D(false);
        if (!actor3D) {
            actor3D = player->Get3D();
        }
        if (!actor3D) {
            return;
        }

        const RE::NiStringExtraData* prn = nullptr;
        const RE::NiStringExtraData* frn = nullptr;
        for (std::uint16_t i = 0; i < modelNode->GetExtraDataSize(); ++i) {
            auto extra = netimmerse_cast<RE::NiStringExtraData*>(modelNode->GetExtraDataAt(i));
            if (!extra || !extra->value || extra->value[0] == '\0') {
                continue;
            }

            if (_stricmp(extra->GetName().c_str(), "Prn") == 0 && !prn) {
                prn = extra;
            } 
            else if (_stricmp(extra->GetName().c_str(), "Frn") == 0 && !frn) {
                frn = extra;
            }
        }

        const char* nodeName = "NPC Root [Root]";
        if (prn) {
            nodeName = prn->value;
        } 
        else if (frn) {
            nodeName = frn->value;
        }

        if (frn && !prn && _stricmp(nodeName, "Scene Root") == 0) {
            return;
        }

        const bool sceneFixedFrn =
            frn && !prn &&
            (_stricmp(nodeName, "AnimObjectA") == 0 || _stricmp(nodeName, "AnimObjectB") == 0);
        const bool sceneFixedDefault = !prn && !frn;
        const bool animationWeaponPrn = prn && _stricmp(nodeName, "WEAPON") == 0;

        RE::NiPointer<RE::NiAVObject> transformSource;
        RE::NiTransform transformSourceLocal;
        bool useTransformSource = false;

        RE::NiNode* parent = nullptr;
        if (animationWeaponPrn) {
            RE::NiTransform weaponLocal;
            if (GetPlayerWeaponBindTransform(player, actor3D, transformSource, weaponLocal)) {
                parent = actor3D->parent ? actor3D->parent->AsNode() : nullptr;
                if (parent) {
                    transformSourceLocal = weaponLocal * modelNode->local;
                    useTransformSource = true;

                    const auto objectWorld = transformSource->world * transformSourceLocal;
                    modelNode->local = parent->world.Invert() * objectWorld;
                }
            }
        } 
		else if (sceneFixedDefault) {
            parent = actor3D->parent ? actor3D->parent->AsNode() : nullptr;
            if (parent) {
                const auto nifLocal = modelNode->local;

                RE::NiTransform slotWorld;
                slotWorld.translate = {ostimAlignmentX, ostimAlignmentY, ostimAlignmentZ};
                slotWorld.rotate.SetEulerAnglesXYZ(0.0f, 0.0f, ostimAlignmentR);
                slotWorld.scale = player->GetScale();

                const auto objectWorld = slotWorld * nifLocal;
                modelNode->local = parent->world.Invert() * objectWorld;
            }
        } 
        else if (sceneFixedFrn) {
            parent = actor3D->parent ? actor3D->parent->AsNode() : nullptr;
            auto locatorObject = actor3D->GetObjectByName(RE::BSFixedString(nodeName));
            if (parent && locatorObject) {
                const auto nifLocal = modelNode->local;
                auto locatorLocal = locatorObject->local;
                locatorLocal.translate = {0.0f, 0.0f, 0.0f};

                RE::NiTransform slotWorld;
                slotWorld.translate = {ostimAlignmentX, ostimAlignmentY, ostimAlignmentZ};
                slotWorld.rotate.SetEulerAnglesXYZ(0.0f, 0.0f, ostimAlignmentR);
                slotWorld.scale = player->GetScale();

                const auto objectWorld = slotWorld * locatorLocal * nifLocal;
                modelNode->local = parent->world.Invert() * objectWorld;
            }
        }

        if (!parent) 
        {
            if (auto object = actor3D->GetObjectByName(RE::BSFixedString(nodeName)); object) {
                parent = object->AsNode();
            }

            if (!parent) {
                if (auto object = actor3D->GetObjectByName(RE::BSFixedString("NPC Root [Root]")); object) {
                    parent = object->AsNode();
                }
            }

            if (!parent) {
                parent = actor3D->AsNode();
            }
        }

        if (!parent) {
            return;
        }

        if (!sceneFixedFrn && (_stricmp(nodeName, "AnimObjectA") == 0 || _stricmp(nodeName, "AnimObjectB") == 0)) {
            parent->SetAppCulled(false);
        }

        parent->AttachChild(modelNode, true);
        RefreshPlayerAnimObject(modelNode, parent);

        PlayerAnimObjectAttachment attachment;
        attachment.parent = RE::NiPointer<RE::NiNode>(parent);
        attachment.object = modelClone;
        attachment.transformSource = transformSource;
        attachment.transformSourceLocal = transformSourceLocal;
        attachment.useTransformSource = useTransformSource;
        SyncPlayerAnimObject(attachment);
        playerAnimObjectAttachments.push_back(std::move(attachment));

    }

    void ApplyPlayerAnimObjectsForEvent(std::string_view animationEvent)
    {
        if (!playerInScene || animationEvent.empty()) {
            return;
        }

        const auto eventName = NormalizePlayerAnimObjectString(std::string(animationEvent));
        const auto generation = ++playerAnimObjectGeneration;
        std::vector<std::string> editorIDs;

        if (const auto it = playerAnimObjectEventMap.find(eventName); it != playerAnimObjectEventMap.end()) {
            editorIDs = it->second;
        }

        SKSE::GetTaskInterface()->AddTask([editorIDs = std::move(editorIDs), generation]() {
            if (!playerInScene || generation != playerAnimObjectGeneration.load()) {
                return;
            }

            ClearPlayerAnimObjectsNow();
            for (const auto& editorID : editorIDs) {
                AttachPlayerAnimObject(editorID);
            }
        });
    }

    void ClearPlayerAnimObjects()
    {
        const auto generation = ++playerAnimObjectGeneration;
        SKSE::GetTaskInterface()->AddTask([generation]() {
            if (generation != playerAnimObjectGeneration.load()) {
                return;
            }

            ClearPlayerAnimObjectsNow();
        });
    }

    float NormalizeAngleRadians(float angle)
    {
        while (angle > 3.14159265359f) angle -= 6.28318530718f;
        while (angle < -3.14159265359f) angle += 6.28318530718f;
        return angle;
    }

    OstimVRAlignment GetCurrentSceneAlignment()
    {
        OstimVRAlignment sceneAlignment;
        auto state = UI::UIState::GetSingleton();
        if (state && state->currentThread) {
            auto currentNode = state->currentThread->getCurrentNodeInternal();
            if (currentNode && currentNode->isTransition == false) {
                auto it = sceneAlignmentMap.find(currentNode->scene_id);
                if (it != sceneAlignmentMap.end()) sceneAlignment = it->second;
            }
        }
        return sceneAlignment;
    }

    float GetFirstPersonViewRotation()
    {
        auto sceneAlignment = GetCurrentSceneAlignment();
        return ostimAlignmentR + (globalAlignments.angleOffsetDegrees + sceneAlignment.angleOffsetDegrees) / 57.2957795131f;
    }

    float GetDesiredViewRotation(bool firstPerson)
    {
        auto state = UI::UIState::GetSingleton();
        if (!state || !state->currentThread) return GetFirstPersonViewRotation();
        return firstPerson ? GetFirstPersonViewRotation() : state->currentThread->getCenter().r + 1.571f;
    }

    void CaptureSceneVRRotationBaseline()
    {
        sceneSnapWalkBaselineValid = false;
        sceneHeightOffsetBaselineValid = false;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto vrNodes = player->GetVRNodeData();
        if (!vrNodes) return;

        if (vrNodes->SnapWalkOffsetNode) {
            sceneSnapWalkBaseline = vrNodes->SnapWalkOffsetNode->local;
            sceneSnapWalkBaselineValid = true;
        }
        if (vrNodes->HeightOffsetNode) {
            sceneHeightOffsetBaselineRotation = vrNodes->HeightOffsetNode->local.rotate;
            sceneHeightOffsetBaselineValid = true;
        }
    }

    void ClearVRIKAnimationRotation()
    {
        if (vrikInterface != nullptr) {
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 0.0);
            vrikInterface->setSettingDouble("rotateHmdToBody", 0.0);
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto vrNodes = player->GetVRNodeData();
        if (vrNodes && vrNodes->HeightOffsetNode) {
            if (sceneHeightOffsetBaselineValid) {
                vrNodes->HeightOffsetNode->local.rotate = sceneHeightOffsetBaselineRotation;
            } else {
                vrNodes->HeightOffsetNode->local.rotate = RE::NiMatrix3{};
            }
        }
    }

    void ResetSceneVRRotationForCameraSwitch()
    {
        firstPersonBaseRotationValid = false;
        ClearVRIKAnimationRotation();

        if (!sceneSnapWalkBaselineValid) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto vrNodes = player->GetVRNodeData();
        if (!vrNodes || !vrNodes->SnapWalkOffsetNode) return;

        vrNodes->SnapWalkOffsetNode->local.rotate = sceneSnapWalkBaseline.rotate;
        vrNodes->SnapWalkOffsetNode->local.translate = sceneSnapWalkBaseline.translate;
    }

    void RestoreSceneVRRotationBaseline()
    {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            auto vrNodes = player->GetVRNodeData();
            if (vrNodes) {
                if (sceneSnapWalkBaselineValid && vrNodes->SnapWalkOffsetNode) {
                    vrNodes->SnapWalkOffsetNode->local.rotate = sceneSnapWalkBaseline.rotate;
                    vrNodes->SnapWalkOffsetNode->local.translate = sceneSnapWalkBaseline.translate;
                }
                if (sceneHeightOffsetBaselineValid && vrNodes->HeightOffsetNode) {
                    vrNodes->HeightOffsetNode->local.rotate = sceneHeightOffsetBaselineRotation;
                }
            }
        }

        sceneSnapWalkBaselineValid = false;
        sceneHeightOffsetBaselineValid = false;
        firstPersonBaseRotationValid = false;
    }

    void RotateVRViewBy(float rotationDelta)
    {
        rotationDelta = NormalizeAngleRadians(rotationDelta);
        if (std::abs(rotationDelta) < 0.0001f) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto vrNodes = player->GetVRNodeData();
        if (!vrNodes || !vrNodes->SnapWalkOffsetNode) return;

        RE::NiAVObject* hmd = vrNodes->UprightHmdNode.get();
        if (!hmd) hmd = vrNodes->GamepadNode.get();
        if (!hmd) return;

        auto snapNode = vrNodes->SnapWalkOffsetNode.get();
        RE::NiMatrix3 rotation;
        rotation.MakeZRotation(rotationDelta);

        RE::NiPoint3 hmdWorldPos = hmd->world.translate;
        RE::NiPoint3 snapWorldPos = snapNode->world.translate;
        RE::NiPoint3 rotatedHmdWorldPos = snapWorldPos + rotation * (hmdWorldPos - snapWorldPos);
        RE::NiPoint3 worldPositionCorrection = hmdWorldPos - rotatedHmdWorldPos;

        if (snapNode->parent) {
            auto parentInverseRotation = snapNode->parent->world.rotate.Transpose();
            snapNode->local.rotate = parentInverseRotation * (rotation * snapNode->world.rotate);

            RE::NiPoint3 localPositionCorrection = parentInverseRotation * worldPositionCorrection;
            if (std::abs(snapNode->parent->world.scale) > 0.0001f) {
                localPositionCorrection /= snapNode->parent->world.scale;
            }
            snapNode->local.translate += localPositionCorrection;
        } else {
            snapNode->local.rotate = rotation * snapNode->local.rotate;
            snapNode->local.translate += worldPositionCorrection;
        }
    }

    void RotateVRViewTo(float targetRotation)
    {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto vrNodes = player->GetVRNodeData();
        if (!vrNodes) return;

        RE::NiAVObject* hmd = vrNodes->UprightHmdNode.get();
        if (!hmd) hmd = vrNodes->GamepadNode.get();
        if (!hmd) return;

        RE::NiPoint3 hmdForward = hmd->world.rotate * RE::NiPoint3{0.0f, 1.0f, 0.0f};
        float currentRotation = std::atan2(hmdForward.x, hmdForward.y);
        float rotationDelta = NormalizeAngleRadians(targetRotation - currentRotation);

        RotateVRViewBy(rotationDelta);
    }

    void FollowFirstPersonBaseRotation(float targetRotation)
    {
        if (!playerInScene || !CurrentCameraFirstPerson) return;

        if (firstPersonBaseRotationValid) {
            float rotationDelta = NormalizeAngleRadians(targetRotation - firstPersonBaseRotation);
            if (std::abs(rotationDelta) >= 0.0001f) {
                RotateVRViewBy(rotationDelta);
            }
        }

        firstPersonBaseRotation = targetRotation;
        firstPersonBaseRotationValid = true;

        if (vrikInterface != nullptr) {
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 2.0);
        }
    }

    void SetFirstPersonHmdLockSettings(float maxThreshold, float speed)
    {
        if (vrikInterface == nullptr) return;

        vrikInterface->setSettingDouble("lockHmdMinThreshold", lockHmdMinThreshold);
        vrikInterface->setSettingDouble("lockHmdMaxThreshold", maxThreshold);
        vrikInterface->setSettingDouble("lockHmdSpeed", speed);
        vrikInterface->setSettingDouble("lockHmdToBody", 1.0);
    }

    void ScheduleFirstPersonHmdPositionRecentering(bool resetCurrentOffset)
    {
        if (vrikInterface == nullptr || !playerInScene || !CurrentCameraFirstPerson) return;

        const std::uint32_t generation = ++hmdPositionTransitionGeneration;

        if (resetCurrentOffset) {
            vrikInterface->setSettingDouble("lockHmdToBody", 0.0);
        } else {
            SetFirstPersonHmdLockSettings(lockHmdMaxThreshold, lockHmdSpeed);
        }

        std::thread([generation, resetCurrentOffset]() {
            auto runIfCurrent = [generation](const std::function<void()>& action) {
                SKSE::GetTaskInterface()->AddTask([generation, action]() {
                    if (!playerInScene || !CurrentCameraFirstPerson || hmdPositionTransitionGeneration != generation) return;
                    action();
                });
            };

            if (resetCurrentOffset) {
                Sleep(100);
                runIfCurrent([]() { SetFirstPersonHmdLockSettings(lockHmdMaxThreshold, lockHmdSpeed); });
                Sleep(200);
            } else {
                Sleep(300);
            }

            auto recenterPulse = [runIfCurrent]() {
                runIfCurrent([]() { SetFirstPersonHmdLockSettings(lockHmdMinThreshold, 1000.0f); });
                Sleep(60);
                runIfCurrent([]() { SetFirstPersonHmdLockSettings(lockHmdMaxThreshold, lockHmdSpeed); });
            };

            recenterPulse();

            Sleep(440);
            recenterPulse();
        }).detach();
    }

    void ScheduleFirstPersonHmdPositionRecenteringAfterAnimation()
    {
        if (vrikInterface == nullptr || !playerInScene || !CurrentCameraFirstPerson) 
			return;

        const std::uint32_t generation = ++hmdPositionTransitionGeneration;
        SetFirstPersonHmdLockSettings(lockHmdMaxThreshold, lockHmdSpeed);

        SKSE::GetTaskInterface()->AddTask([generation]() {
            if (!playerInScene || !CurrentCameraFirstPerson || hmdPositionTransitionGeneration != generation) return;

            SKSE::GetTaskInterface()->AddTask([generation]() {
                if (!playerInScene || !CurrentCameraFirstPerson || hmdPositionTransitionGeneration != generation) return;

                SetFirstPersonHmdLockSettings(lockHmdMinThreshold, 1000.0f);

                std::thread([generation]() {
                    Sleep(60);
                    SKSE::GetTaskInterface()->AddTask([generation]() {
                        if (!playerInScene || !CurrentCameraFirstPerson || hmdPositionTransitionGeneration != generation) return;
                        SetFirstPersonHmdLockSettings(lockHmdMaxThreshold, lockHmdSpeed);
                    });

                    Sleep(340);
                    SKSE::GetTaskInterface()->AddTask([generation]() {
                        if (!playerInScene || !CurrentCameraFirstPerson || hmdPositionTransitionGeneration != generation) return;
                        SetFirstPersonHmdLockSettings(lockHmdMinThreshold, 1000.0f);
                    });

                    Sleep(60);
                    SKSE::GetTaskInterface()->AddTask([generation]() {
                        if (!playerInScene || !CurrentCameraFirstPerson || hmdPositionTransitionGeneration != generation) return;
                        SetFirstPersonHmdLockSettings(lockHmdMaxThreshold, lockHmdSpeed);
                    });
                }).detach();
            });
        });
    }
    void ApplyPlayerViewPosition(const RE::NiPoint3& position, float rotation)
    {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;
        GameAPI::GameActor(player).setPosition(GameAPI::GamePosition(position, rotation));
    }

    void MovePlayerInThirdPersonStart(bool firstPerson)
    {
        auto state = UI::UIState::GetSingleton();
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!state || !state->currentThread || !player) return;

        auto center = state->currentThread->getCenter();
        RE::NiPoint3 newPos = player->GetPosition();
        float playerRotation = center.r;

        if (firstPerson) 
        {
            newPos.x = center.x;
            newPos.y = center.y;
        } else {
            float sin = std::sin(center.r);
            float cos = std::cos(center.r);

            // Forward displacement
            newPos.x += sin * 25.0f;
            newPos.y += cos * 25.0f;

            // Side displacement
            newPos.x -= cos * 20.0f;
            newPos.y += sin * 20.0f;
            playerRotation = center.r + 1.571f;
        }

        ApplyPlayerViewPosition(newPos, playerRotation);
        RotateVRViewTo(GetDesiredViewRotation(firstPerson));

        if (firstPerson) {
            firstPersonBaseRotation = GetFirstPersonViewRotation();
            firstPersonBaseRotationValid = true;
        } else {
            firstPersonBaseRotationValid = false;
        }

        const std::uint32_t transitionGeneration = cameraTransitionGeneration;
        std::thread([firstPerson, newPos, playerRotation, transitionGeneration]() {
            auto reapply = [firstPerson, newPos, playerRotation, transitionGeneration](bool startAnimationRotationMonitor) {
                SKSE::GetTaskInterface()->AddTask([firstPerson, newPos, playerRotation, transitionGeneration, startAnimationRotationMonitor]() {
                    if (!playerInScene || CurrentCameraFirstPerson != firstPerson || cameraTransitionGeneration != transitionGeneration) return;
                    ApplyPlayerViewPosition(newPos, playerRotation);
                    RotateVRViewTo(GetDesiredViewRotation(firstPerson));

                    if (firstPerson) {
                        firstPersonBaseRotation = GetFirstPersonViewRotation();
                        firstPersonBaseRotationValid = true;
                        if (startAnimationRotationMonitor && vrikInterface != nullptr) {
                            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 2.0);
                        }
                    }
                });
            };

            Sleep(50);
            reapply(false);
            Sleep(150);
            reapply(true);
        }).detach();
    }

    /*void RotatePlayerInFirstPersonSwitch(float r)
    {
        SKSE::GetTaskInterface()->AddTask([r]() {
            OstimVRAlignment sceneAlignment;
            auto state = UI::UIState::GetSingleton();
            if (state) {
                if (state->currentThread) {
                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (player != nullptr && player->AsReference()) {
                        const RE::NiPoint3 newPos = player->AsReference()->GetPosition();

                        player->AsReference()->SetAngle(RE::NiPoint3{0.0f, 0.0f, r});
                        player->AsReference()->SetPosition(newPos);
                    }
                }
            }
        });
    }*/

    void ModifyAlignment() 
    {
        OstimVRAlignment sceneAlignment = GetCurrentSceneAlignment();
        auto state = UI::UIState::GetSingleton();
        if (state) {

            if (state->currentThread) {
                float sin = std::sin(state->currentThread->getCenter().r);
                float cos = std::cos(state->currentThread->getCenter().r);

                float rotAngle = ostimAlignmentR * 57.2957795131f + globalAlignments.angleOffsetDegrees + sceneAlignment.angleOffsetDegrees;
                while (rotAngle > 360.0f) {
                    rotAngle -= 360.0f;
                }
                while (rotAngle < 0.0f) {
                    rotAngle += 360.0f;
                }

                vrikInterface->setSettingDouble("lockRotationAngle", rotAngle);

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
                if (enableVRIKScaling) 
                {
                    auto gameActors = state->currentThread->getGameActors();
                    for (int i = 0; i < gameActors.size(); i++) 
                    {
                        if (gameActors[i].isPlayer()) 
                        {
                            const float playerScale = gameActors[i].getScale();
                            vrikInterface->setSettingDouble("bodySize", playerScale);
                            vrikInterface->setSettingDouble("armSize", playerScale);
                            //vrikInterface->setSettingDouble("armLength", 1.0f);

                            break;
                        }
                    }
                }
            }
        }
    }

    void VRIKLockPositionAndRotation(float rotSin, float rotCos, float x, float y, float z, float r, float playerScale) {
        ostimAlignmentX = x;
        ostimAlignmentY = y;
        ostimAlignmentZ = z;
        ostimAlignmentR = r;

        OstimVRAlignment sceneAlignment = GetCurrentSceneAlignment();

        float rotAngle = r * 57.2957795131f + globalAlignments.angleOffsetDegrees + sceneAlignment.angleOffsetDegrees;
        while (rotAngle > 360.0f) {
            rotAngle -= 360.0f;
        }
        while (rotAngle < 0.0f) {
            rotAngle += 360.0f;
        }

        vrikInterface->setSettingDouble("lockRotationAngle", rotAngle);
        vrikInterface->setSettingDouble("lockRotation", 1);

        if (CurrentCameraFirstPerson) {
            float targetRotation = rotAngle / 57.2957795131f;
            SKSE::GetTaskInterface()->AddTask([targetRotation]() { FollowFirstPersonBaseRotation(targetRotation); });
        }

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
            //logger::critical("Playerscale is: {}", playerScale);
            vrikInterface->setSettingDouble("bodySize", playerScale);
            vrikInterface->setSettingDouble("armSize", playerScale);
            //vrikInterface->setSettingDouble("armLength", 1.0f);
        }

        if (playerInScene && CurrentCameraFirstPerson) {
            ScheduleFirstPersonHmdPositionRecentering(false);
        }

        /*if (CurrentCameraFirstPerson) {
            SKSE::GetTaskInterface()->AddTask([r]() {
                auto player = RE::PlayerCharacter::GetSingleton();
                if (player != nullptr && player->AsReference()) {
                    auto vrnodes = player->GetVRNodeData();
                    if (vrnodes && vrnodes->GamepadNode && vrnodes->GamepadNode.get())
                    {
                        float x, y, z;
                        vrnodes->GamepadNode.get()->world.rotate.ToEulerAnglesXYZ(x,y,z);
                        logger::critical("player hmd rotation was {} - {} --- r:{} - {}", z, z * 57.2957795131f, r, r
        * 57.2957795131f); vrnodes->GamepadNode.get()->world.rotate.SetEulerAnglesXYZ(x, y, 0.0f);
                    }
                }
            });
        }*/

        /*player->SetRotationZ(0.0f);
        player->SetPosition(player->AsReference()->GetPosition(), true);*/
    }

    void FixStandingBug() {
        Sleep(2000);
        // Fix for stand animation bug
        Threading::ThreadManager* threadManager = Threading::ThreadManager::GetSingleton();

        if (threadManager != nullptr && threadManager->playerThreadRunning()) {
            auto playerThread = threadManager->getPlayerThread();
            if (playerThread != nullptr) {
                playerThread->SetSpeed(playerThread->getSpeed());
            }
        }
    }

    int controllersShown = -1;

    void ShowHideControllersFunc(bool ostimwheelknowntobeopen) {
        if (controllerFixInterface != nullptr) {
            if (showControllersInFirstPerson && CurrentCameraFirstPerson && !trackHands && playerInScene &&
                spellWheelInterface) {
                if (ostimwheelknowntobeopen || spellWheelInterface->IsSecondaryWheelOpen() ||
                    spellWheelInterface->IsMainWheelOpen()) {
                    if (controllersShown != 1) {
                        controllerFixInterface->ForceShowControllers(true);
                        controllersShown = 1;
                    }
                } else if (!spellWheelInterface->IsSecondaryWheelOpen() && !spellWheelInterface->IsMainWheelOpen()) {
                    if (controllersShown != 0) {
                        controllerFixInterface->ForceShowControllers(false);
                        controllersShown = 0;
                    }
                }
            } else if (showControllersInThirdPerson && !CurrentCameraFirstPerson && playerInScene) {
                if (controllersShown != 1) {
                    controllerFixInterface->ForceShowControllers(true);
                    controllersShown = 1;
                }
            } else if (controllersShown != 0) {
                controllerFixInterface->ForceShowControllers(false);
                controllersShown = 0;
            }
        }
    }

    void SetOstimVRSettings(bool firstPerson) {
        ResetSceneVRRotationForCameraSwitch();

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

            if (ChangeHeadAboveDistance) 
			{
                vrikInterface->setSettingDouble("headAboveDistance", firstPerson ? HeadAboveDistance : orgHeadAboveDistance);
            }
            if (ChangeHeadForwardDistance) {
                vrikInterface->setSettingDouble("headInFrontDistance", firstPerson ? HeadForwardDistance : orgHeadForwardDistance);
            }

            vrikInterface->setSettingDouble("enableHead", 1);

            if (FBTExists)
                vrikInterface->setSettingDouble("enableBody", firstPerson ? 0 : 1);

            vrikInterface->setSettingDouble("hidePlayerHeadDistance", 12.0f);

            vrikInterface->setSettingDouble("lockHmdMinThreshold", firstPerson ? lockHmdMinThreshold : 500.0f);
            vrikInterface->setSettingDouble("lockHmdMaxThreshold", firstPerson ? lockHmdMaxThreshold : 500.0f);
            vrikInterface->setSettingDouble("lockHmdSpeed", firstPerson ? lockHmdSpeed : 20.0f);
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 0.0);
            vrikInterface->setSettingDouble("rotateHmdToBody", 0.0);
            vrikInterface->setSettingDouble("lockHmdToBody", 0.0);
        }

        ShowHideControllersFunc(false);

        MovePlayerInThirdPersonStart(firstPerson);
        if (firstPerson) {
            ScheduleFirstPersonHmdPositionRecentering(true);
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

    bool TooDistToRealBodyCheck() 
    {
        auto state = UI::UIState::GetSingleton();
        if (state) {
            if (state->currentThread) {
                auto center = state->currentThread->getCenter();

                auto player = RE::PlayerCharacter::GetSingleton();
                if (player != nullptr && player->AsReference()) {
                    RE::NiPoint3 newPos = player->AsReference()->GetPosition();
                    float distSqr = distance2DNoSqrt(newPos, RE::NiPoint3(center.x, center.y, center.z));
                    if (distSqr > 15000.0f) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void CameraSwitchFunc(bool firstPerson) 
    {
        /*if (firstPerson)
        {
            if (TooDistToRealBodyCheck())
            {
            RE::SendHUDMessage::ShowHUDMessage("You need to be closer to your real body.");
                return;
            }
        }*/
        // logger::info("Applying {} settings", firstPerson ? "First Person" : "Third Person");

        const std::uint32_t transitionGeneration = ++cameraTransitionGeneration;
        ++hmdPositionTransitionGeneration;
        CurrentCameraFirstPerson = firstPerson;

        SKSE::GetTaskInterface()->AddTask([firstPerson, transitionGeneration]() {
            if (!playerInScene || CurrentCameraFirstPerson != firstPerson || cameraTransitionGeneration != transitionGeneration) return;
            SetOstimVRSettings(firstPerson);
        });
    }

    void PlayerSceneStart() 
    {
        playerInScene = true;
        ClearPlayerAnimObjects();
        ++cameraTransitionGeneration;
        ++hmdPositionTransitionGeneration;
        CaptureSceneVRRotationBaseline();

        controllersShown = -1;

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

        RE::Setting* bDirectMovementWithWandsSetting = RE::GetINISetting("bDirectMovementWithWands:VRInput");
        if (bDirectMovementWithWandsSetting) 
        {
            originalbDirectMovementWithWands = bDirectMovementWithWandsSetting->data.b;  
            if (originalbDirectMovementWithWands == false)
            {
                bDirectMovementWithWandsSetting->data.b = true;
            }
        }

        RE::UI* ui = RE::UI::GetSingleton();
        if (ui != nullptr && ui->IsMenuOpen("WSActivateRollover")) 
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

            orgHeadAboveDistance = vrikInterface->getSettingDouble("headAboveDistance"); 
            orgHeadForwardDistance = vrikInterface->getSettingDouble("headInFrontDistance");

            vrikInterface->setSettingDouble("selfieModeEnabled", 0);
            vrikInterface->setSettingDouble("cameraOffsetting", 0);
            vrikInterface->setSettingDouble("enablePosture", 0);
            vrikInterface->setSettingDouble("enableBody", 0);
            vrikInterface->setSettingDouble("enableJumping", 0);
            vrikInterface->setSettingDouble("displayHolsters", 0);
            vrikInterface->setSettingDouble("nearClipDistance", nearDistance);

            if (hideVRIKCompass)
                vrikInterface->setSettingDouble("hideCompass", 1);
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

        if (higgsInterface != nullptr) 
        {
            if (disableGravityGloves == 1) {
                higgsInterface->GetSettingDouble("FarCastDistance", higgsFarCastDistanceOrgValue);

                higgsInterface->SetSettingDouble("FarCastDistance", 0.0);
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
        ClearPlayerAnimObjects();
        playerInScene = false;
        ++cameraTransitionGeneration;
        ++hmdPositionTransitionGeneration;
        firstPersonBaseRotationValid = false;

        if (vrikInterface != nullptr) {
            vrikInterface->setSettingDouble("rotateHmdToBodySeconds", 0.0);
            vrikInterface->setSettingDouble("rotateHmdToBody", 0.0);
            vrikInterface->restoreSettings();
        }

        RestoreSceneVRRotationBaseline();

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

        if (higgsInterface != nullptr)
        {
            if (disableGravityGloves == 1) {
                higgsInterface->SetSettingDouble("FarCastDistance", higgsFarCastDistanceOrgValue);
            }
        }

        if (originalbDirectMovementWithWands == false) {
            RE::Setting* bDirectMovementWithWandsSetting = RE::GetINISetting("bDirectMovementWithWands:VRInput");
            if (bDirectMovementWithWandsSetting) {
                bDirectMovementWithWandsSetting->data.b = false;                
            }
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

        ShowHideControllersFunc(false);

        controllersShown = -1;

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
        output << "ChangeHeadForwardDistance = 0 #Enables/Disables modification of head forward distance during 1st person mode.\n";
        output << "HeadForwardDistance = 0.0   #Sets VRIK headInFrontDistance to this value during 1st person mode.\n";
        output << "\n";
        output << "ChangeHeadAboveDistance = 0 #Enables/Disables modification of head above distance during 1st person mode.\n";
        output << "HeadAboveDistance = 16.0   #Sets VRIK headAboveDistance to this value during 1st person mode.\n";
        output << "\n";
        output << "\n";
        output << "LockHmdMaxThreshold = 20.0    #Lock HMD in place max threshold. May cause nausea at 0. You can "
                  "tighten it to prevent going off place. Default is 20.0.\n";
        output << "LockHmdMinThreshold = 2.0     #Lock HMD in place min threshold. May cause nausea at 0. Default is 2.0.\n";
        output << "LockHmdSpeed = 50.0           #Lock HMD in place speed. May cause nausea at high values if used with low min-max threshold values. You can increase it to make it faster to snap in place. Default "
                  "is 50.0.\n";
        output << "\n";
        output << "# OStimVR Settings General\n";
        output << "EnableVRIKScaling = 1         #Apply Ostim scaling settings to VRIK body.\n";
        output << "\n";
        output << "DisablePLANCKduringScenes = 0   #Disable PLANCK collision during scenes.\n";
        output << "\n";
        output << "DisableGravityGloves = 1   #Disable HIGGS gravity gloves during scenes.\n";
        output << "\n";
        output << "HideVRIKCompass = 1   #Disable compass during scenes.\n";
        output << "\n";
        output << "HeightAdjustSpeed = 1.0       #Snapback speed for viewpoint. Higher speeds may cause nausea. "
                  "Default is 1.0.\n";
        output << "\n";
        output << "ShowControllersInThirdPerson = 1 #Shows openvr controllers while in third person mode. Requires ControllerFixVR mod.\n ";
        output << "\n";
        output << "ShowControllersInFirstPerson = 1 #Shows openvr controllers while in first person mode if a wheel is open and hand tracking is off. Requires ControllerFixVR mod.\n ";
        output << "\n";
        output << "DefaultThirdPerson = 0        #If set to 1, scenes will start in third person camera. You can always switch during scenes using Ostim Wheel.\n";
        output << "\n";

        output.close();
    }

    void loadConfig() 
    {
        fs::path fbtPath = "Data\\SKSE\\Plugins\\SkyrimVR-FBT.dll";
        FBTExists = std::filesystem::exists(fbtPath);

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
                            else if (variableName == "ChangeHeadForwardDistance") {
                                ChangeHeadForwardDistance = std::stoi(variableValue);
                            } 
                            else if (variableName == "ChangeHeadAboveDistance") {
                                ChangeHeadAboveDistance = std::stoi(variableValue);
                            } 
                            else if (variableName == "HeadForwardDistance") {
                                HeadForwardDistance = std::strtof(variableValue.c_str(), 0);
                            } 
                            else if (variableName == "HeadAboveDistance") {
                                HeadAboveDistance = std::strtof(variableValue.c_str(), 0);
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
                            else if (variableName == "ShowControllersInFirstPerson") {
                                showControllersInFirstPerson = std::stoi(variableValue);
                            } 
                            else if (variableName == "DefaultThirdPerson") {
                                defaultThirdPerson = std::stoi(variableValue);

                                CurrentCameraFirstPerson = !defaultThirdPerson;
                            } 
                            else if (variableName == "DisablePLANCKduringScenes") {
                                disablePLANCKduringScenes = std::stoi(variableValue);
                            } 
                            else if (variableName == "DisableGravityGloves") {
                                disableGravityGloves = std::stoi(variableValue);
                            }  
                            else if (variableName == "HideVRIKCompass") {
                                hideVRIKCompass = std::stoi(variableValue);
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

        RE::SendHUDMessage::ShowHUDMessage("Global alignments saved.");
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

        RE::SendHUDMessage::ShowHUDMessage("Scene alignments saved.");
    }

    void PrintNodesTree(int depth, std::vector<Graph::Node*>& visitedList, Graph::Node* node) 
    {
        if (node) 
        {
            auto text = std::string(depth, '.');
            logger::info("{}{} - {} - {} - transition:{} - navs:{}", text, node->scene_id, node->scene_name,
                         node->modpack, node->isTransition, node->navigations.size());
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
                for (Graph::Node* navNode : nav.nodes) 
                {
                    if (navNode && std::find(visitedList.begin(), visitedList.end(), navNode) == visitedList.end() &&
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