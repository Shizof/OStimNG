#include "Core.h"

#include "ThreadManager.h"

#include "ActorProperties/ActorPropertyTable.h"
#include "MCM/MCMTable.h"
#include "Trait/TraitTable.h"
#include "Util/ActorUtil.h"
#include "Util/APITable.h"

namespace Threading {
    void freeActor(GameAPI::GameActor actor, bool byGameLoad) {
        if (byGameLoad) {
            if (!MCM::MCMTable::isScalingDisabled()) {
                actor.setScale(1.0);
            }
            // TODO: clear potential heel offset
        }

        Util::APITable::getActorCountFaction().remove(actor);
        Util::APITable::getExcitementFaction().remove(actor);
        Util::APITable::getInWaterFaction().remove(actor);
        Util::APITable::getSchlongifiedFaction().remove(actor);
        Util::APITable::getTimesClimaxedFaction().remove(actor);
        Util::APITable::getTimeUntilClimaxFaction().remove(actor);
        actor.unlock();
        
        actor.updateAI();

        actor.playAnimation("SOSFlaccid");
    }

    bool isEligible(GameAPI::GameActor actor) {   //for vr version
        if (!actor) {
            logger::warn("actor eligibility failed: null actor");
            return false;
        }

        const auto name = actor.getName();
        if (actor.isDisabled()) {
            logger::warn("actor eligibility failed for {}: disabled", name);
            return false;
        }
        if (actor.isDeleted()) {
            logger::warn("actor eligibility failed for {}: deleted", name);
            return false;
        }
        if (actor.isChild()) {
            logger::warn("actor eligibility failed for {}: child actor", name);
            return false;
        }
        if (actor.isDead()) {
            logger::warn(
                "actor eligibility failed for {} ({:08X}): dead; lifeState={}, health={}",
                name,
                actor.form->formID,
                static_cast<std::uint32_t>(actor.getLifeState()),
                actor.getCurrentHealth());
            return false;
        }
        const auto actorType = ActorProperties::ActorPropertyTable::getActorType(actor);
        if (actorType.empty()) {
            logger::warn("actor eligibility failed for {}: no OStim actor type matched", name);
            return false;
        }
        if (ThreadManager::GetSingleton()->findActor(actor)) {
            logger::warn("actor eligibility failed for {}: already in an OStim thread", name);
            return false;
        }

        return true;
    }
}