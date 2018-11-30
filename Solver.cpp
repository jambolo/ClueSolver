#include "Solver.h"

#include <nlohmann/json.hpp>

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
        cards_[id].possible = playerIds;
        cards_[id].possible.emplace_back(ANSWER_PLAYER_ID);
        cards_[id].info = c.second;
    }

    types_ = rules.types;

    for (auto const & p : playerIds)
    {
        assert(p != ANSWER_PLAYER_ID);
        Player & player = players_[p];
        player.possible = cardIds;
    }
    players_[ANSWER_PLAYER_ID].possible = cardIds;
}

void Solver::hand(Id const & playerId, IdList const & cardsIds)
{
    discoveriesLog_.clear();
    bool changed = false;

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

void Solver::accuse(Id const & playerId, IdList const & cardIds, bool outcome, int id)
{
    discoveriesLog_.clear();
    bool changed = false;

    Accusation accusation = { id, playerId, cardIds, outcome };
    accusations_.push_back(accusation);

    deduce(accusation, changed);
    makeOtherDeductions(changed);
}

Solver::IdList Solver::mightBeHeldBy(Id const & playerId) const
{
    assert(players_.find(playerId) != players_.end());
    Player const & p = players_.find(playerId)->second;
    return p.possible;
}

Solver::IdList Solver::mightHold(Id const & cardId) const
{
    assert(cards_.find(cardId) != cards_.end());
    Card const & c = cards_.find(cardId)->second;
    return c.possible;
}

json Solver::toJson() const
{
    json j;
    j["cards"]       = ::toJson(cards_);
    j["players"]     = ::toJson(players_);
    j["suggestions"] = ::toJson(suggestions_);
    return j;
}

bool Solver::playersAreValid(IdList const & playerIds) const
{
    for (auto const & p : playerIds)
    {
        if (!playerIsValid(p))
            return false;
    }
    return true;
}

bool Solver::playerIsValid(Id const & playerId) const
{
    return playerId != ANSWER_PLAYER_ID && players_.find(playerId) != players_.end();
}

bool Solver::cardsAreValid(IdList const & cardIds) const
{
    for (auto const & c : cardIds)
    {
        if (!cardIsValid(c))
            return false;
    }
    return true;
}

bool Solver::cardIsValid(Id const & cardId) const
{
    return cards_.find(cardId) != cards_.end();
}

bool Solver::typeIsValid(Id const & typeId) const
{
    return types_.find(typeId) != types_.end();
}

// If the player must hold one of the cards, but we know it doesn't hold all but one, then that one must be the one that is held
bool Solver::mustHoldOne(Id const & playerId, IdList const & cardIds, Id & held)
{
    int count = 0;
    for (auto const & cardId : cardIds)
    {
        if (players_[playerId].mightHold(cardId))
        {
            if (++count > 1)
                return false;
            held = cardId;
        }
    }
    return true;
}

// If the player must not hold one of the cards, but we know it holds all but one, then that one is the one it doesn't hold
bool Solver::mustNotHoldOne(Id const & playerId, IdList const & cardIds, Id & notHeld)
{
    int count = 0;
    for (auto const & cardId : cardIds)
    {
        if (!cards_[cardId].isHeldBy(playerId))
        {
            if (++count > 1)
                return false;
            notHeld = cardId;
        }
    }
    return true;
}

void Solver::deduce(Suggestion const & suggestion, bool & changed)
{
    if (rulesId_ == "master")
        deduceWithMasterRules(suggestion, changed);
    else
        deduceWithClassicRules(suggestion, changed);
}

// Make deductions based on the results of this accusation
void Solver::deduce(Accusation const & accusation, bool & changed)
{
    // You can deduce from an accusation that :
    //    The accuser does not have the cards in the accusation (assuming no suicidal intentions).
    //    If the accusation is correct, then those are the cards (and the game is over).
    //    If the accusation is incorrect, then
    //      At least one of the cards is not held by the answer, but if we know that two of the cards are held by the answer,
    //      then the third is not held

    int            id      = accusation.id;
    Id const &     accuser = accusation.player;
    IdList const & cards   = accusation.cards;
    bool           correct = accusation.correct;

    addDiscoveries(accuser, cards, false, "made accusation #" + std::to_string(id));
    disassociatePlayerWithCards(accuser, cards, changed);

    if (correct)
    {
        for (auto const & card : cards)
        {
            associatePlayerWithCard(ANSWER_PLAYER_ID, card, changed);
        }
    }
    else
    {
        Id mustNotHold;
        if (mustNotHoldOne(ANSWER_PLAYER_ID, cards, mustNotHold))
        {
            addDiscovery(ANSWER_PLAYER_ID, mustNotHold, false, "holds the other cards in accusation #" + std::to_string(id));
            disassociatePlayerWithCard(ANSWER_PLAYER_ID, mustNotHold, changed);
        }
    }
}

// Make deductions based on the player having exactly these cards
void Solver::deduce(Id const & playerId, IdList const & cards, bool & changed)
{
    Player & player = players_[playerId];

    // Associate the player with every card in the list and disassociate the player with every other card.
    for (auto & c : cards_)
    {
        if (std::find(cards.begin(), cards.end(), c.first) != cards.end())
        {
            addDiscovery(playerId, c.first, true, "hand");
            associatePlayerWithCard(playerId, c.first, changed);
        }
        else
        {
            addDiscovery(playerId, c.first, false, "hand");
            disassociatePlayerWithCard(playerId, c.first, changed);
        }
    }
}

// Make deductions based on the player having this cardId
void Solver::deduce(Id const & playerId, Id const & cardId, bool & changed)
{
    addDiscovery(playerId, cardId, true, "revealed");
    associatePlayerWithCard(playerId, cardId, changed);
}

void Solver::deduceWithClassicRules(Suggestion const & suggestion, bool & changed)
{
    assert(rulesId_ == "classic");
    int            id        = suggestion.id;
    Id const &     suggester = suggestion.player;
    IdList const & cards     = suggestion.cards;
    IdList const & showed    = suggestion.showed;

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
                addDiscoveries(playerId, cards, false, "did not show a card in suggestion #" + std::to_string(id));
                disassociatePlayerWithCards(playerId, suggestion.cards, changed);
            }
        }
    }
    else
    {
        // All but the last player have none of the cards
        for (size_t i = 0; i < showed.size() - 1; ++i)
        {
            Id const & playerId = showed[i];
            for (auto const & c : cards)
            {
                addDiscovery(playerId, c, false, "did not show a card in suggestion #" + std::to_string(id));
            }
            disassociatePlayerWithCards(playerId, suggestion.cards, changed);
        }

        // The last player showed a card.
        {
            // If the player does not hold all but one of cards, the player must hold the one.
            Id const &     playerId       = showed[showed.size() - 1];
            Player const & player         = players_[playerId];
            int            mightHoldCount = 0;
            Id             mustHold;
            for (auto const & c : cards)
            {
                if (player.mightHold(c))
                {
                    ++mightHoldCount;
                    if (mightHoldCount == 1)
                        mustHold = c;   // Assuming none of the others are held
                    else
                        break;          // Optimization
                }
            }
            assert(mightHoldCount >= 1);
            if (mightHoldCount == 1)
            {
                addDiscovery(playerId,
                             mustHold,
                             true,
                             "showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others");
                associatePlayerWithCard(playerId, mustHold, changed);
            }
        }
    }
}

void Solver::deduceWithMasterRules(Suggestion const & suggestion, bool & changed)
{
    assert(rulesId_ == "master");
    int            id        = suggestion.id;
    Id const &     suggester = suggestion.player;
    IdList const & cards     = suggestion.cards;
    IdList const & showed    = suggestion.showed;

    // You can deduce from a suggestion that:
    //		If a player shows a card but does not all but one of the suggested cards, the player must hold the one.
    //		If a player (other than the answer and suggester) does not show a card, the player has none of the suggested cards.
    //		If all suggested cards are shown, then the answer and the suggester hold none of the suggested cards.

    for (auto const & p : players_)
    {
        Id const &     playerId = p.first;
        Player const & player   = p.second;

        // If the player showed a card ...
        if (std::find(showed.begin(), showed.end(), playerId) != showed.end())
        {
            // ..., then if the player does not hold all but one of the cards, the player must hold the one.
            int mightHoldCount = 0;
            Id  mustHold;
            for (auto const & c : cards)
            {
                if (player.mightHold(c))
                {
                    ++mightHoldCount;
                    if (mightHoldCount == 1)
                        mustHold = c;   // Assuming none of the others are held
                    else
                        break;          // Optimization
                }
            }
            assert(mightHoldCount >= 1);
            if (mightHoldCount == 1)
            {
                addDiscovery(playerId,
                             mustHold,
                             true,
                             "showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others");
                associatePlayerWithCard(playerId, mustHold, changed);
            }
        }

        // Otherwise, if the player is other than the answer and suggester ...
        else if (playerId != ANSWER_PLAYER_ID && playerId != suggester)
        {
            // ... then they don't hold any of them.
            for (auto const & c : cards)
            {
                addDiscovery(playerId, c, false, "did not show a card in suggestion #" + std::to_string(id));
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
                             false,
                             "all three cards were shown by other players in suggestion #" + std::to_string(id));
            }
            disassociatePlayerWithCards(playerId, suggestion.cards, changed);
        }
    }
}

bool Solver::makeOtherDeductions(bool changed)
{
    addCardHoldersToDiscoveries();
    checkThatAnswerHoldsExactlyOneOfEach(changed);

    // Re-apply all the suggestions and accusations until knowledge has not changed
    while (changed)
    {
        changed = false;
        for (auto & s : suggestions_)
        {
            deduce(s, changed);
        }
        for (auto & a : accusations_)
        {
            deduce(a, changed);
        }
        addCardHoldersToDiscoveries();
        checkThatAnswerHoldsExactlyOneOfEach(changed);
    }
    addCardHoldersToDiscoveries();
    return changed;
}

void Solver::checkThatAnswerHoldsExactlyOneOfEach(bool & changed)
{
    Player & answer = players_[ANSWER_PLAYER_ID];

    // Remove any possible cards that are of the same type as cards known to be held by the answer
    {
        // Get a list of the cards known to be held by the answer
        std::map<Id, Id> held;
        for (auto const & cardId : answer.possible)
        {
            Card const & card = cards_[cardId];
            if (card.isHeldBy(ANSWER_PLAYER_ID))
            {
                Id const & type = card.info.type;
                held[type] = cardId;
            }
        }

        // Each type held by the answer, none of others of the same type can be held
        IdList possible = answer.possible;    // Must use a copy because the list may be mutated on the fly
        for (auto const & cardId : possible)
        {
            for (auto const & h : held)
            {
                if (cards_[cardId].info.type == h.first && cardId != h.second)
                {
                    addDiscovery(ANSWER_PLAYER_ID, cardId, false, "ANSWER can only hold one " + h.first);
                    disassociatePlayerWithCard(ANSWER_PLAYER_ID, cardId, changed);
                }
            }
        }
    }

    // For each type, if there is only one card that might be held by the answer, then that card must be held by the answer
    {
        std::map<Id, Id> unique;
        for (auto const & cardId : answer.possible)
        {
            Card const & card = cards_[cardId];
            if (!card.isHeldBy(ANSWER_PLAYER_ID))
            {
                Id                         typeId = card.info.type;
                std::map<Id, Id>::iterator i      = unique.find(typeId);
                if (i == unique.end())
                    unique.insert({ typeId, cardId }); // First card for this type
                else
                    i->second.clear(); // Not unique
            }
        }

        // Any unique card of a type is now known to be held
        for (auto const & u : unique)
        {
            Id const & cardId = u.second;
            if (cardId.length() > 0)
            {
                addDiscovery(ANSWER_PLAYER_ID, cardId, true, "Only " + u.first + " that ANSWER can hold");
                associatePlayerWithCard(ANSWER_PLAYER_ID, cardId, changed);
            }
        }
    }
}

void Solver::associatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed)
{
    Card & c = cards_[cardId];
    if (c.possible.size() == 1)
    {
        assert(c.possible[0] == playerId);
        return;
    }

    disassociateOtherPlayersWithCard(playerId, cardId, changed);
    changed = true;
}

void Solver::disassociatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed)
{
    Player & player = players_[playerId];
    if (player.mightHold(cardId))
    {
        player.remove(cardId);
        cards_[cardId].remove(playerId);
        changed = true;
        addDiscovery(playerId, cardId, false);  // Add this discovery, but don't log it
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
            disassociatePlayerWithCard(p.first, cardId, changed);
    }
}

bool Solver::cardIsType(Id const & c, Id const & type) const
{
    return cards_.find(c)->second.info.type == type;
}

void Solver::addDiscovery(Id const & playerId, Id const & cardId, bool holds, std::string const & reason /*= std::string()*/)
{
    auto fact = std::make_pair(playerId, cardId);
    auto f    = facts_.find(fact);
    if (f == facts_.end())
    {
        facts_[fact] = holds;
        if (!reason.empty())
        {
            CardInfo const & cardInfo  = cards_[cardId].info;
            TypeInfo const & typeinfo  = types_[cardInfo.type];
            std::string      discovery = playerId + (holds ? " holds " : " does not hold ") +
                                         typeinfo.article + cardInfo.name +
                                         ": " + reason;
            discoveriesLog_.push_back(discovery);
        }
    }
    else
    {
        assert(f->second == holds);
    }
}

void Solver::addDiscoveries(Id const & playerId, IdList const & cards, bool holds, std::string reason)
{
    for (auto const & c : cards)
    {
        addDiscovery(playerId, c, holds, reason);
    }
}

void Solver::addCardHoldersToDiscoveries()
{
    for (auto & c : cards_)
    {
        IdList const & holders = c.second.possible;
        if (holders.size() == 1)
            addDiscovery(holders[0], c.first, true, "nobody else holds it");
    }
}

void Solver::Card::remove(Id const & playerId)
{
    possible.erase(std::remove(possible.begin(), possible.end(), playerId), possible.end());
}

bool Solver::Card::mightBeHeldBy(Id const & playerId) const
{
    return std::find(possible.begin(), possible.end(), playerId) != possible.end();
}

bool Solver::Card::isHeldBy(Id const & playerId) const
{
    return possible.size() == 1 && possible[0] == playerId;
}

json Solver::Card::toJson() const
{
    json j;
    j["possible"] = possible;
    return j;
}

void Solver::Player::remove(Id const & cardId)
{
    possible.erase(std::remove(possible.begin(), possible.end(), cardId), possible.end());
}

bool Solver::Player::mightHold(Id const & cardId) const
{
    return std::find(possible.begin(), possible.end(), cardId) != possible.end();
}

json Solver::Player::toJson() const
{
    json j;
    j["possible"] = possible;
    return j;
}

json Solver::Suggestion::toJson() const
{
    json j;
    j["player"] = player;
    j["cards"]  = cards;
    j["showed"] = showed;
    return j;
}

nlohmann::json Solver::Accusation::toJson() const
{
    json j;
    j["player"]  = player;
    j["cards"]   = cards;
    j["correct"] = correct;
    return j;
}
