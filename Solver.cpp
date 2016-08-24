#include "Solver.h"

#include <algorithm>

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
        suspects_[s].players = players;
        suspects_[s].players.push_back(ANSWER_PLAYER_NAME);
    }

    for (auto const & w : weapons)
    {
        weapons_[w].players = players;
        weapons_[w].players.push_back(ANSWER_PLAYER_NAME);
    }

    for (auto const & r : rooms)
    {
        rooms_[r].players = players;
        rooms_[r].players.push_back(ANSWER_PLAYER_NAME);
    }
}

void Solver::reveal(Name const & player, Name const & card)
{
    bool changed = false;
    if (suspects_.find(card) != suspects_.end())
    {
        changed = revealSuspect(player, card);
    }
    else if (weapons_.find(card) != weapons_.end())
    {
        changed = revealWeapon(player, card);
    }
    else
    {
        changed = revealRoom(player, card);
    }

    // If something changed, then re-apply all the suggestions
    if (changed)
    {
        reapplySuggestions();
    }
}

void Solver::suggest(Name const &     suspect,
                     Name const &     weapon,
                     Name const &     room,
                     NameList const & players)
{
    Suggestion s = { suspect, weapon, room, players };

    // Remember the suggestion
    suggestions_.push_back(s);

    // Apply the suggestion
    bool changed = apply(s);

    // Then re-apply all the suggestions
    if (changed)
    {
        reapplySuggestions();
    }
}

void Solver::accuse(Name const & suspect, Name const & weapon, Name const & room)
{
    // A failed accusation means that the answer does not hold those cards.

    bool changed    = false;
    Player & answer = players_[ANSWER_PLAYER_NAME];

    if (disassociateSuspect(answer, suspect))
        changed = true;
    if (disassociateWeapon(answer, weapon))
        changed = true;
    if (disassociateRoom(answer, room))
        changed = true;

    // Then re-apply all the suggestions
    if (changed)
    {
        reapplySuggestions();
    }
}

bool Solver::disassociateRoom(Player & player, Name const & room)
{
    bool changed         = false;
    NameList::iterator r = std::find(player.rooms.begin(), player.rooms.end(), room);
    if (r != player.rooms.end())
    {
        player.rooms.erase(r);
        NameList & holders = rooms_[room].players;
        holders.erase(std::remove(holders.begin(), holders.end(), ANSWER_PLAYER_NAME), holders.end());
        changed = true;
    }
    return changed;
}

bool Solver::disassociateWeapon(Player & player, Name const & weapon)
{
    bool changed         = false;
    NameList::iterator w = std::find(player.weapons.begin(), player.weapons.end(), weapon);
    if (w != player.weapons.end())
    {
        player.weapons.erase(w);
        NameList & holders = weapons_[weapon].players;
        holders.erase(std::remove(holders.begin(), holders.end(), ANSWER_PLAYER_NAME), holders.end());
        changed = true;
    }
    return changed;
}

bool Solver::disassociateSuspect(Player & player, Name const & suspect)
{
    bool changed         = false;
    NameList::iterator s = std::find(player.suspects.begin(), player.suspects.end(), suspect);
    if (s != player.suspects.end())
    {
        player.suspects.erase(s);
        NameList & holders = suspects_[suspect].players;
        holders.erase(std::remove(holders.begin(), holders.end(), ANSWER_PLAYER_NAME), holders.end());
        changed = true;
    }
    return changed;
}

Solver::NameList Solver::mightBeHeldBy(Name const & player) const
{
    Player const & p = players_.find(player)->second;

    NameList cards;
    cards.insert(cards.end(), p.suspects.begin(), p.suspects.end());
    cards.insert(cards.end(), p.weapons.begin(), p.weapons.end());
    cards.insert(cards.end(), p.rooms.begin(), p.rooms.end());
    return cards;
}

Solver::NameList Solver::mightHold(Name const & card) const
{
    CardList::const_iterator s = suspects_.find(card);
    if (s != suspects_.end())
    {
        return s->second.players;
    }

    CardList::const_iterator w = weapons_.find(card);
    if (w != weapons_.end())
    {
        return w->second.players;
    }

    CardList::const_iterator r = rooms_.find(card);
    if (r != rooms_.end())
    {
        return r->second.players;
    }
    return NameList();
}

bool Solver::revealSuspect(Name const & player, Name const & card)
{
    bool changed       = false;
    NameList & holders = suspects_[card].players;
    if (holders.size() > 1)
    {
        // Remove all other players from the card
        holders.clear();
        holders.push_back(player);

        // Remove the card from all other players.
        for (auto & p : players_)
        {
            if (p.first != player)
            {
                NameList & suspects = p.second.suspects;
                suspects.erase(std::remove(suspects.begin(), suspects.end(), card), suspects.end());
            }
        }
        changed = true;
    }
    return changed;
}

bool Solver::revealWeapon(Name const & player, Name const & card)
{
    bool changed       = false;
    NameList & holders = weapons_[card].players;
    if (holders.size() > 1)
    {
        // Remove all other players from the card
        holders.clear();
        holders.push_back(player);

        // Remove the card from all other players.
        for (auto & p : players_)
        {
            if (p.first != player)
            {
                NameList & weapons = p.second.weapons;
                weapons.erase(std::remove(weapons.begin(), weapons.end(), card), weapons.end());
            }
        }
        changed = true;
    }
    return changed;
}

bool Solver::revealRoom(Name const & player, Name const & card)
{
    bool changed       = false;
    NameList & holders = rooms_[card].players;
    if (holders.size() > 1)
    {
        // Remove all other players from the card
        holders.clear();
        holders.push_back(player);

        // Remove the card from all other players.
        for (auto & p : players_)
        {
            if (p.first != player)
            {
                NameList & rooms = p.second.rooms;
                rooms.erase(std::remove(rooms.begin(), rooms.end(), card), rooms.end());
            }
        }
        changed = true;
    }
    return changed;
}

bool Solver::apply(Suggestion const & suggestion)
{
    bool changed = false;
    // You can infer from a suggestion that:
    //		If a player shows a card but does not have two of the cards, the player must have the third.
    //		If a player (other than the answer) does not show a card, the player has none of those cards.
    //		If all three cards are shown, then the answer has none of those cards.

    for (auto & p : players_)
    {
        Name const & name = p.first;
        Player & player   = p.second;
        if (std::find(suggestion.players.begin(), suggestion.players.end(), name) != suggestion.players.end())
        {
            // The player showed a card. If the player does not have two of the cards, the player must have the third.
            bool noSuspect = std::find(player.suspects.begin(), player.suspects.end(), suggestion.suspect) == player.suspects.end();
            bool noWeapon  = std::find(player.weapons.begin(), player.weapons.end(), suggestion.weapon) == player.weapons.end();
            bool noRoom    = std::find(player.rooms.begin(), player.rooms.end(), suggestion.room) == player.rooms.end();

            if (noSuspect && noWeapon)
            {
                if (revealRoom(name, suggestion.room))
                    changed = true;
            }
            else if (noSuspect && noRoom)
            {
                if (revealWeapon(name, suggestion.weapon))
                    changed = true;
            }
            else if (noWeapon && noRoom)
            {
                if (revealSuspect(name, suggestion.suspect))
                    changed = true;
            }
        }
        else if (name != ANSWER_PLAYER_NAME || suggestion.players.size() == 3)
        {
            // The player did not show a card. Remove all suggested cards from the player.
            if (disassociateSuspect(player, suggestion.suspect))
                changed = true;
            if (disassociateWeapon(player, suggestion.weapon))
                changed = true;
            if (disassociateRoom(player, suggestion.room))
                changed = true;
        }
    }
    return false;
}

void Solver::reapplySuggestions()
{
    // Keep reapplying suggestions until nothing changes
    bool changed = false;
    do
    {
        changed = false;
        for (auto & s : suggestions_)
        {
            if (apply(s))
                changed = true;
        }
    } while (changed);
}
