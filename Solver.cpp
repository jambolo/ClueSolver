#include "Solver.h"

#include <json.hpp>

#include <algorithm>

using json = nlohmann::json;

namespace
{
template <typename T>
json toJson(std::map<std::string, T> const & m)
{
    json j;
    for (auto const & e : m)
    {
        j[e.first] = e.second.toJson();
    }
    return j;
}

template <typename T>
json toJson(std::vector<T> const & v)
{
    json j = json::array();
    for (auto const & e : v)
    {
        j.push_back(e.toJson());
    }
    return j;
}
} // anonymous namespace

char const * const Solver::ANSWER_PLAYER_ID = "ANSWER";

Solver::Solver(Rules const & rules, IdList const & playerIds)
	: rulesId_(rules.id)
{
	assert(rules.id == "classic" || rules.id == "master");

    std::vector<Id> cardIds;

    cardIds.reserve(rules.cards.size());
    for (auto const & c : rules.cards)
    {
        Id id = c.first;
        cardIds.push_back(id);
        cards_[id].holders = playerIds;
        cards_[id].holders.push_back(ANSWER_PLAYER_ID);
    }

    for (auto const & p : playerIds)
    {
        assert(p != ANSWER_PLAYER_ID);
        Player & player = players_[p];
        player.cards = cardIds;
    }
    players_[ANSWER_PLAYER_ID].cards = cardIds;
}

void Solver::hand(Id const & playerId, IdList const & cardsIds)
{
    discoveriesLog_.clear();
    bool changed    = false;

    deduce(playerId, cardsIds, changed);
    makeOtherDeductions(changed);
}

void Solver::show(Id const & playerId, Id const & cardId)
{
    discoveriesLog_.clear();
    bool changed = false;
    deduce(playerId, cardId, changed);
    makeOtherDeductions(changed);
}

void Solver::suggest(Id const & playerId, IdList const & cardIds, IdList const & showed, int id)
{
    discoveriesLog_.clear();
    bool changed = false;

    Suggestion suggestion = { id, playerId, cardIds, showed };
    suggestions_.push_back(suggestion);

    deduce(suggestion, changed);
    makeOtherDeductions(changed);
}

Solver::IdList Solver::mightBeHeldBy(Id const & playerId) const
{
    assert(players_.find(playerId) != players_.end());
    Player const & p = players_.find(playerId)->second;
    return p.cards;
}

Solver::IdList Solver::mightHold(Id const & cardId) const
{
    assert(cards_.find(cardId) != cards_.end());
    Card const & c = cards_.find(cardId)->second;
    return c.holders;
}

json Solver::toJson() const
{
    json j;
    j["cards"]       = ::toJson(cards_);
    j["players"]     = ::toJson(players_);
    j["suggestions"] = ::toJson(suggestions_);
    return j;
}

bool Solver::PlayersAreValid(IdList const & playerIds) const
{
    for (auto const & p : playerIds)
    {
        if (!PlayerIsValid(p))
            return false;
    }
    return true;
}

bool Solver::PlayerIsValid(Id const & playerId) const
{
    return playerId != ANSWER_PLAYER_ID && players_.find(playerId) != players_.end();
}

bool Solver::CardsAreValid(IdList const & cardIds) const
{
    for (auto const & c : cardIds)
    {
        if (!CardIsValid(c))
            return false;
    }
    return true;
}

bool Solver::CardIsValid(Id const & cardId) const
{
    return cards_.find(cardId) != cards_.end();
}

bool Solver::TypeIsValid(Id const & typeId) const
{
    return cards_.find(typeId) != types_.end();
}

void Solver::deduce(Suggestion const & suggestion, bool & changed)
{
    int id = suggestion.id;
    Id const & suggester = suggestion.player;
    IdList const & cards = suggestion.cards;
    IdList const & showed = suggestion.showed;
    if (rulesId_ == "master")
    {
        // You can deduce from a suggestion that:
        //		If a player shows a card but does not all but one of the suggested cards, the player must hold the one.
        //		If a player (other than the answer and suggester) does not show a card, the player has none of the suggested cards.
        //		If all suggested cards are shown, then the answer and the suggester hold none of the suggested cards.

        for (auto const & p : players_)
        {
            Id const & playerId = p.first;
            Player const & player = p.second;

            // If the player showed a card ...
            if (std::find(showed.begin(), showed.end(), playerId) != showed.end())
            {
                // ..., then if the player does not hold all but one of the cards, the player must hold the one.
                int mightHoldCount = 0;
                Id mustHold;
                for (auto const & c : cards)
                {
                    if (player.mightHold(c))
                    {
                        ++mightHoldCount;
                    }
                    else
                    {
                        if (mustHold.empty())
                            mustHold = c;   // Assuming none of the others are held
                        else
                            break;          // Optimization
                    }
                }
                if (mightHoldCount == cards.size() - 1)
                {
                    addDiscovery(playerId,
                        mustHold,
                        "showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others",
                        true);
                    associatePlayerWithCard(playerId, mustHold, changed);
                }
            }

            // Otherwise, if the player is other than the answer and suggester ...
            else if (playerId != ANSWER_PLAYER_ID && playerId != suggester)
            {
                // ... then they don't hold any of them.
                for (auto const & c : cards)
                {
                    addDiscovery(playerId, c, "did not show a card in suggestion #" + std::to_string(id), false);
                }
                disassociatePlayerWithCards(playerId, suggestion.cards, changed);
            }

            // Otherwise, if all three cards were shown ...
            else if (showed.size() == 3)
            {
                // ... then players that don't show cards don't hold them.
                // ... then they don't hold any of them.
                for (auto const & c : cards)
                {
                    addDiscovery(playerId,
                        c,
                        "all three cards were shown by other players in suggestion #" + std::to_string(id),
                        false);
                }
                disassociatePlayerWithCards(playerId, suggestion.cards, changed);
            }
        }
    }
    else
    {
        // You can deduce from a suggestion that:
        //		If nobody showed a card, then none of the players (except possibly the suggester or the answer) have the cards.
        //		Only the last player in the showed list might hold any of the suggested cards.
        //		If the player that showed a card does not hold all but one of the cards, the player must hold the one.

        if (showed.empty())
        {
            for (auto const & p : players_)
            {
                Id const & playerId = p.first;
                if (playerId != ANSWER_PLAYER_ID && playerId != suggester)
                {
                    for (auto const & c : cards)
                    {
                        addDiscovery(playerId, c, "did not show a card in suggestion #" + std::to_string(id), false);
                    }
                    disassociatePlayerWithCards(playerId, suggestion.cards, changed);
                }
            }
        }
        else
        {
            // All but the last player have none of the cards
            for (int i = 0; i < showed.size() - 1; ++i)
            {
                Id const & playerId = showed[i];
                for (auto const & c : cards)
                {
                    addDiscovery(playerId, c, "did not show a card in suggestion #" + std::to_string(id), false);
                }
                disassociatePlayerWithCards(playerId, suggestion.cards, changed);
            }

            // The last player showed a card.
            {
                // If the player does not hold all but one of cards, the player must hold the one.
                Id const & playerId = showed[showed.size() - 1];
                Player const & player = players_[playerId];
                int mightHoldCount = 0;
                Id mustHold;
                for (auto const & c : cards)
                {
                    if (player.mightHold(c))
                    {
                        ++mightHoldCount;
                    }
                    else
                    {
                        if (mustHold.empty())
                            mustHold = c;   // Assuming none of the others are held
                        else
                            break;          // Optimization
                    }
                }
                if (mightHoldCount == cards.size() - 1)
                {
                    addDiscovery(playerId,
                        mustHold,
                        "showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others",
                        true);
                    associatePlayerWithCard(playerId, mustHold, changed);
                }
            }
        }
    }
}

// Make deductions based on the player having these cards
void Solver::deduce(Id const & playerId, IdList const & cards, bool & changed)
{
    Player & player = players_[playerId];

    for (auto & c : cards_)
    {
        if (std::find(cards.begin(), cards.end(), c.first) != cards.end())
        {
            addDiscovery(playerId, c.first, "hand", true);
            associatePlayerWithCard(playerId, c.first, changed);
        }
        else
        {
            addDiscovery(playerId, c.first, "hand", false);
            disassociatePlayerWithCard(playerId, c.first, changed);
        }
    }
}

// Make deductions based on the player having this cardId
void Solver::deduce(Id const & playerId, Id const & cardId, bool & changed)
{
    addDiscovery(playerId, cardId, "revealed", true);
    associatePlayerWithCard(playerId, cardId, changed);
}

bool Solver::makeOtherDeductions(bool changed)
{
    checkThatAnswerHoldsOnlyOneOfEach(changed);

    // While anything has changed, then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        for (auto & s : suggestions_)
        {
            deduce(s, changed);
        }
        checkThatAnswerHoldsOnlyOneOfEach(changed);
    }
    return changed;
}

void Solver::checkThatAnswerHoldsOnlyOneOfEach(bool & changed)
{
    Player & answer = players_[ANSWER_PLAYER_ID];

    // See if there are any cards that held by the answer
    std::map<Id, Id> held;
    for (auto const & c : cards_)
    {
        IdList const & holders = c.second.holders;
        if (holders.size() == 1 && holders[0] == ANSWER_PLAYER_ID)
        {
            Id const & cardId = c.first;
            Id const & type = cards_[cardId].info.type;
            held[type] = cardId;
        }
    }

    // If so, then the answer can not hold any other cards of the same types
    IdList cards = answer.cards;    // Must use a copy because the list of cards may be changed on the fly
    for (auto const & c : cards)
    {
        for (auto const & h : held)
        {
            if (cards_[c].info.type == h.first && c != h.second)
            {
                addDiscovery(ANSWER_PLAYER_ID, c, "ANSWER can only hold one " + h.first, false);
                disassociatePlayerWithCard(ANSWER_PLAYER_ID, c, changed);
            }
        }
    }
}

void Solver::associatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed)
{
    Card & c = cards_[cardId];
    if (c.holders.size() == 1)
    {
        assert(c.holders[0] == playerId);
        return;
    }

    disassociateOtherPlayersWithCard(playerId, cardId, changed);
    addDiscoveredCardHolders();
    changed = true;
}

void Solver::disassociatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed)
{
    Player & player = players_[playerId];
    if (player.mightHold(cardId))
    {
        player.remove(cardId);
        cards_[cardId].removeHolder(playerId);
        changed = true;
    }
}

void Solver::disassociatePlayerWithCards(Id const & playerId, IdList const & suggestion, bool & changed)
{
    for (auto const & c : suggestion)
    {
        disassociatePlayerWithCard(playerId, c, changed);
    }
}

void Solver::disassociateOtherPlayersWithCard(Id const & playerId, Id const & cardId, bool & changed)
{
    for (auto & p : players_)
    {
        if (p.first != playerId)
        {
            disassociatePlayerWithCard(p.first, cardId, changed);
        }
    }
}

bool Solver::cardIsType(Id const & c, Id const & type) const
{
    return cards_.find(c)->second.info.type == type;
}

void Solver::addDiscovery(Id const & playerId, Id const & cardId, std::string const & reason, bool has)
{
    auto fact = std::make_pair(playerId, cardId);
    auto f    = facts_.find(fact);
    if (f == facts_.end())
    {
        std::string discovery = playerId + (has ? " holds " : " does not hold ") + cardId + ": " + reason;
        discoveriesLog_.push_back(discovery);
        facts_[fact] = has;
    }
    else
    {
        assert(f->second == has);
    }
}

void Solver::addDiscoveredCardHolders()
{
	for (auto & c : cards_)
	{
		IdList const & holders = c.second.holders;
		if (holders.size() == 1)
			addDiscovery(holders[0], c.first, "nobody else holds it", true);
	}
}

void Solver::Card::removeHolder(Id const & playerId)
{
    holders.erase(std::remove(holders.begin(), holders.end(), playerId), holders.end());
}

bool Solver::Card::mightBeHeldBy(Id const & playerId) const
{
    return std::find(holders.begin(), holders.end(), playerId) != holders.end();
}

json Solver::Card::toJson() const
{
    json j;
    j["holders"] = holders;
    return j;
}

void Solver::Player::remove(Id const & cardId)
{
    cards.erase(std::remove(cards.begin(), cards.end(), cardId), cards.end());
}

bool Solver::Player::mightHold(Id const & cardId) const
{
    return std::find(cards.begin(), cards.end(), cardId) != cards.end();
}

json Solver::Player::toJson() const
{
    json j;
    j["cards"] = cards;
    return j;
}

json Solver::Suggestion::toJson() const
{
    json j;
    j["player"]  = player;
    j["cards"] = cards;
    j["showed"]  = showed;
    return j;
}
