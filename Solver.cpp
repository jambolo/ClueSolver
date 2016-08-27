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

char const * const Solver::ANSWER_PLAYER_NAME = "ANSWER";

Solver::Solver(NameList const & players,
               NameList const & suspects,
               NameList const & weapons,
               NameList const & rooms)
{
    for (auto const & p : players)
    {
        Player & player = players_[p];
        player.suspects = suspects;
        player.weapons  = weapons;
        player.rooms    = rooms;
    }

    players_[ANSWER_PLAYER_NAME].suspects = suspects;
    players_[ANSWER_PLAYER_NAME].weapons  = weapons;
    players_[ANSWER_PLAYER_NAME].rooms    = rooms;

    for (auto const & s : suspects)
    {
        suspects_[s].holders = players;
        suspects_[s].holders.push_back(ANSWER_PLAYER_NAME);
    }

    for (auto const & w : weapons)
    {
        weapons_[w].holders = players;
        weapons_[w].holders.push_back(ANSWER_PLAYER_NAME);
    }

    for (auto const & r : rooms)
    {
        rooms_[r].holders = players;
        rooms_[r].holders.push_back(ANSWER_PLAYER_NAME);
    }
}

void Solver::hand(Name const & name, NameList const & cards)
{
    // Clear the possible cards for the player. The ones that are actually held will be added back.
    Player & player = players_[name];
    player.suspects.clear();
    player.weapons.clear();
    player.rooms.clear();

    // Remove the player from every card's list of possible holders. The player will be added back to the cards he actually holds.
    for (auto & s : suspects_)
    {
        s.second.removeHolder(name);
    }

    for (auto & w : weapons_)
    {
        w.second.removeHolder(name);
    }

    for (auto & r : rooms_)
    {
        r.second.removeHolder(name);
    }

    // Add back the cards that the player holds and remove other players from the cards' lists of holders
    for (auto const & c : cards)
    {
        if (isSuspect(c))
        {
            player.suspects.push_back(c);
            suspects_[c].assignHolder(name);
            nobodyElseHoldsThisSuspect(player, c);
        }
        else if (isWeapon(c))
        {
            player.weapons.push_back(c);
            weapons_[c].assignHolder(name);
            nobodyElseHoldsThisWeapon(player, c);
        }
        else
        {
            player.rooms.push_back(c);
            rooms_[c].assignHolder(name);
            nobodyElseHoldsThisRoom(player, c);
        }
    }
}

void Solver::show(Name const & player, Name const & card)
{
    bool changed = false;
    if (isSuspect(card))
    {
        revealSuspect(player, card, changed);
    }
    else if (isWeapon(card))
    {
        revealWeapon(player, card, changed);
    }
    else
    {
        revealRoom(player, card, changed);
    }

    answerHasOnlyOneOfEach(changed);

    // If something changed, then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        reapplySuggestions(changed);
        answerHasOnlyOneOfEach(changed);
    }
}

void Solver::suggest(Name const & player, NameList const & cards, NameList const & holders)
{
    bool changed = false;

    Suggestion s;
    s.player = player;
    for (auto const & c : cards)
    {
        if (isSuspect(c))
            s.suspect = c;
        else if (isWeapon(c))
            s.weapon = c;
        else
            s.room = c;
    }
    s.holders = holders;

    // Apply the suggestion
    apply(s, changed);

    // Remember the suggestion
    suggestions_.push_back(s);

    answerHasOnlyOneOfEach(changed);

    // If anything has changed, then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        reapplySuggestions(changed);
        answerHasOnlyOneOfEach(changed);
    }
}

void Solver::accuse(Name const & suspect, Name const & weapon, Name const & room)
{
    // A failed accusation means that the answer does not hold those cards.

    bool changed = false;

    disassociatePlayerAndSuspect(ANSWER_PLAYER_NAME, suspect, changed);
    disassociatePlayerAndWeapon(ANSWER_PLAYER_NAME, weapon, changed);
    disassociatePlayerAndRoom(ANSWER_PLAYER_NAME, room, changed);

    answerHasOnlyOneOfEach(changed);

    // Then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        reapplySuggestions(changed);
        answerHasOnlyOneOfEach(changed);
    }
}

void Solver::disassociatePlayerAndRoom(Name const & playerName, Name const & room, bool & changed)
{
    Player & player = players_[playerName];
    if (player.mightHaveRoom(room))
    {
        player.removeRoom(room);
        rooms_[room].removeHolder(playerName);
        changed = true;
    }
}

void Solver::disassociatePlayerAndWeapon(Name const & playerName, Name const & weapon, bool & changed)
{
    Player & player = players_[playerName];
    if (player.mightHaveWeapon(weapon))
    {
        player.removeWeapon(weapon);
        weapons_[weapon].removeHolder(playerName);
        changed = true;
    }
}

void Solver::disassociatePlayerAndSuspect(Name const & playerName, Name const & suspect, bool & changed)
{
    Player & player = players_[playerName];
    if (player.mightHaveSuspect(suspect))
    {
        player.removeSuspect(suspect);
        suspects_[suspect].removeHolder(playerName);
        changed = true;
    }
}

Solver::SortedNameList Solver::mightBeHeldBy(Name const & player) const
{
    Player const & p = players_.find(player)->second;

    SortedNameList cards;
    cards.push_back(p.suspects);
    cards.push_back(p.weapons);
    cards.push_back(p.rooms);
    return cards;
}

Solver::NameList Solver::mightHold(Name const & card) const
{
    CardList::const_iterator s = suspects_.find(card);
    if (s != suspects_.end())
    {
        return s->second.holders;
    }

    CardList::const_iterator w = weapons_.find(card);
    if (w != weapons_.end())
    {
        return w->second.holders;
    }

    CardList::const_iterator r = rooms_.find(card);
    if (r != rooms_.end())
    {
        return r->second.holders;
    }
    return NameList();
}

void Solver::revealSuspect(Name const & player, Name const & card, bool & changed)
{
    if (suspects_[card].holders.size() > 1)
    {
        suspects_[card].assignHolder(player);
        nobodyElseHoldsThisSuspect(players_[player], card);
        changed = true;
    }
}

void Solver::revealWeapon(Name const & player, Name const & card, bool & changed)
{
    if (weapons_[card].holders.size() > 1)
    {
        weapons_[card].assignHolder(player);
        nobodyElseHoldsThisWeapon(players_[player], card);
        changed = true;
    }
}

void Solver::revealRoom(Name const & player, Name const & card, bool & changed)
{
    if (rooms_[card].holders.size() > 1)
    {
        rooms_[card].assignHolder(player);
        nobodyElseHoldsThisRoom(players_[player], card);
        changed = true;
    }
}

void Solver::apply(Suggestion const & suggestion, bool & changed)
{
    // You can infer from a suggestion that:
    //		If a player shows a card but does not have two of the cards, the player must have the third.
    //		If a player (other than the answer and suggester) does not show a card, the player has none of those cards.
    //		If all three cards are shown, then the answer and the suggester have none of those cards.

    for (auto & p : players_)
    {
        Name const & name = p.first;
        Player & player   = p.second;
        if (std::find(suggestion.holders.begin(), suggestion.holders.end(), name) != suggestion.holders.end())
        {
            // The player showed a card. If the player does not have two of the cards, the player must have the third.
            bool noSuspect = !player.mightHaveSuspect(suggestion.suspect);
            bool noWeapon  = !player.mightHaveWeapon(suggestion.weapon);
            bool noRoom    = !player.mightHaveRoom(suggestion.room);

            if (noSuspect && noWeapon)
            {
                revealRoom(name, suggestion.room, changed);
            }
            else if (noSuspect && noRoom)
            {
                revealWeapon(name, suggestion.weapon, changed);
            }
            else if (noWeapon && noRoom)
            {
                revealSuspect(name, suggestion.suspect, changed);
            }
        }
        else if ((name != ANSWER_PLAYER_NAME && name != suggestion.player) || suggestion.holders.size() == 3)
        {
            // If the player could have shown cards but did not, then they don't have any of them. If all three cards were shown,
            // then even players that don't show cards don't have them.
            playerDoesntHaveTheseCards(name, suggestion, changed);
        }
    }
}

void Solver::reapplySuggestions(bool & changed)
{
    for (auto & s : suggestions_)
    {
        apply(s, changed);
    }
}

bool Solver::isSuspect(Name const & c) const
{
    return suspects_.find(c) != suspects_.end();
}

bool Solver::isWeapon(Name const & c) const
{
    return weapons_.find(c) != weapons_.end();
}

bool Solver::isRoom(Name const & c) const
{
    return rooms_.find(c) != rooms_.end();
}

void Solver::nobodyElseHoldsThisSuspect(Player & player, Name const & suspect)
{
    for (auto & p : players_)
    {
        if (&p.second != &player)
        {
            p.second.removeSuspect(suspect);
        }
    }
}

void Solver::nobodyElseHoldsThisWeapon(Player & player, Name const & weapon)
{
    for (auto & p : players_)
    {
        if (&p.second != &player)
        {
            p.second.removeWeapon(weapon);
        }
    }
}

void Solver::nobodyElseHoldsThisRoom(Player & player, Name const & room)
{
    for (auto & p : players_)
    {
        if (&p.second != &player)
        {
            p.second.removeRoom(room);
        }
    }
}

void Solver::playerDoesntHaveTheseCards(Name const & name, Suggestion const & suggestion, bool & changed)
{
    // Remove all suggested cards from the player.
    disassociatePlayerAndSuspect(name, suggestion.suspect, changed);
    disassociatePlayerAndWeapon(name, suggestion.weapon, changed);
    disassociatePlayerAndRoom(name, suggestion.room, changed);
}

void Solver::answerHasOnlyOneOfEach(bool & changed)
{
    Player & answer = players_[ANSWER_PLAYER_NAME];

    // If there is a suspect that is held by the answer, then the answer can not hold any other suspects
    Name suspect;
    for (auto const & s : suspects_)
    {
        NameList const & holders = s.second.holders;
        if (holders.size() == 1 && holders[0] == ANSWER_PLAYER_NAME)
        {
            suspect = s.first;
            break;
        }
    }
    if (!suspect.empty())
    {
        NameList suspects = answer.suspects;
        for (auto const & s : suspects)
        {
            if (s != suspect)
                disassociatePlayerAndSuspect(ANSWER_PLAYER_NAME, s, changed);
        }
    }

    // If there is a weapon that is held by the answer, then the answer can not hold any other weapons
    Name weapon;
    for (auto const & w : weapons_)
    {
        NameList const & holders = w.second.holders;
        if (holders.size() == 1 && holders[0] == ANSWER_PLAYER_NAME)
        {
            weapon = w.first;
            break;
        }
    }
    if (!weapon.empty())
    {
        NameList weapons = answer.weapons;
        for (auto const & w : weapons)
        {
            if (w != weapon)
                disassociatePlayerAndWeapon(ANSWER_PLAYER_NAME, w, changed);
        }
    }

    // If there is a room that is held by the answer, then the answer can not hold any other rooms
    Name room;
    for (auto const & r : rooms_)
    {
        NameList const & holders = r.second.holders;
        if (holders.size() == 1 && holders[0] == ANSWER_PLAYER_NAME)
        {
            room = r.first;
            break;
        }
    }
    if (!room.empty())
    {
        NameList rooms = answer.rooms;
        for (auto const & r : rooms)
        {
            if (r != room)
                disassociatePlayerAndRoom(ANSWER_PLAYER_NAME, r, changed);
        }
    }
}

json Solver::toJson() const
{
    json j;
    j["suspects"]    = ::toJson(suspects_);
    j["weapons"]     = ::toJson(weapons_);
    j["rooms"]       = ::toJson(rooms_);
    j["players"]     = ::toJson(players_);
    j["suggestions"] = ::toJson(suggestions_);
    return j;
}

void Solver::Card::assignHolder(Name const & player)
{
    holders.clear();
    holders.push_back(player);
}

void Solver::Card::removeHolder(Name const & player)
{
    holders.erase(std::remove(holders.begin(), holders.end(), player), holders.end());
}

bool Solver::Card::mightBeHeldBy(Name const & player) const
{
    return std::find(holders.begin(), holders.end(), player) != holders.end();
}

json Solver::Card::toJson() const
{
    json j;
    j["holders"] = holders;
    return j;
}

void Solver::Player::removeSuspect(Name const & card)
{
    suspects.erase(std::remove(suspects.begin(), suspects.end(), card), suspects.end());
}

void Solver::Player::removeWeapon(Name const & card)
{
    weapons.erase(std::remove(weapons.begin(), weapons.end(), card), weapons.end());
}

void Solver::Player::removeRoom(Name const & card)
{
    rooms.erase(std::remove(rooms.begin(), rooms.end(), card), rooms.end());
}

bool Solver::Player::mightHaveSuspect(Name const & suspect) const
{
    return std::find(suspects.begin(), suspects.end(), suspect) != suspects.end();
}

bool Solver::Player::mightHaveWeapon(Name const & weapon) const
{
    return std::find(weapons.begin(), weapons.end(), weapon) != weapons.end();
}

bool Solver::Player::mightHaveRoom(Name const & room) const
{
    return std::find(rooms.begin(), rooms.end(), room) != rooms.end();
}

json Solver::Player::toJson() const
{
    json j;
    j["suspects"] = suspects;
    j["weapons"]  = weapons;
    j["rooms"]    = rooms;
    return j;
}

json Solver::Suggestion::toJson() const
{
    json j;
    j["player"]  = player;
    j["suspect"] = suspect;
    j["weapon"]  = weapon;
    j["room"]    = room;
    j["holders"] = holders;
    return j;
}
