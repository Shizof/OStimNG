#include "ActionActor.h"

#include "Util/VectorUtil.h"

namespace Graph {
    namespace Action {
        void ActionActor::merge(ActionActor& other) {
            if (other.fullStrip) {
                fullStrip = true;
            }

            if (other.moan) {
                moan = true;
            }

            if (other.talk) {
                talk = true;
            }

            if (other.muffled) {
                muffled = true;
            }

            for (std::string requirement : other.requirements) {
                requirements.insert(requirement);
            }

            strippingMask |= other.strippingMask;

            for (std::string equipObject : other.equipObjects) {
                equipObjects.insert(equipObject);
            }

            for (GameAPI::GameFaction faction : other.factions) {
                if (!VectorUtil::contains(factions, faction)) {
                    factions.push_back(faction);
                }
            }

            for (GameAPI::GameFaction faction : other.statFactions) {
                if (!VectorUtil::contains(statFactions, faction)) {
                    statFactions.push_back(faction);
                }
            }

            for (GameAPI::GameFaction faction : other.playerStatFactions) {
                if (!VectorUtil::contains(playerStatFactions, faction)) {
                    playerStatFactions.push_back(faction);
                }
            }

            for (GameAPI::GameFaction faction : other.climaxStatFactions) {
                if (!VectorUtil::contains(climaxStatFactions, faction)) {
                    climaxStatFactions.push_back(faction);
                }
            }

            for (GameAPI::GameFaction faction : other.partnerClimaxStatFactions) {
                if (!VectorUtil::contains(partnerClimaxStatFactions, faction)) {
                    partnerClimaxStatFactions.push_back(faction);
                }
            }

            for (GameAPI::GameFaction faction : other.playerClimaxStatFactions) {
                if (!VectorUtil::contains(playerClimaxStatFactions, faction)) {
                    playerClimaxStatFactions.push_back(faction);
                }
            }

            for (GameAPI::GameFaction faction : other.playerPartnerClimaxStatFactions) {
                if (!VectorUtil::contains(playerPartnerClimaxStatFactions, faction)) {
                    playerPartnerClimaxStatFactions.push_back(faction);
                }
            }

            for (GameAPI::GameList list : other.playerStatLists) {
                if (!VectorUtil::contains(playerStatLists, list)) {
                    playerStatLists.push_back(list);
                }
            }

            for (GameAPI::GameList list : other.playerClimaxStatLists) {
                if (!VectorUtil::contains(playerClimaxStatLists, list)) {
                    playerClimaxStatLists.push_back(list);
                }
            }

            for (GameAPI::GameList list : other.playerPartnerClimaxStatLists) {
                if (!VectorUtil::contains(playerPartnerClimaxStatLists, list)) {
                    playerPartnerClimaxStatLists.push_back(list);
                }
            }


            for (std::string& slot : other.toySlots) {
                if (!VectorUtil::contains(toySlots, slot)) {
                    toySlots.push_back(slot);
                }
            }
        }
    }
}