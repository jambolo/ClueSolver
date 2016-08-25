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

void Solver::reveal(Name const & player, Name const & card)
{
    bool changed = false;
    if (isSuspect(card))
    {
        changed = revealSuspect(player, card);
    }
    else if (isWeapon(card))
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

void Solver::suggest(Name const &     player,
	Name const &     suspect,
	Name const &     weapon,
                     Name const &     room,
                     NameList const & holders)
{
    Suggestion s = { player, suspect, weapon, room, holders };

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

    if (disassociatePlayerAndSuspect(ANSWER_PLAYER_NAME, suspect))
        changed = true;
    if (disassociatePlayerAndWeapon(ANSWER_PLAYER_NAME, weapon))
        changed = true;
    if (disassociatePlayerAndRoom(ANSWER_PLAYER_NAME, room))
        changed = true;

    // Then re-apply all the suggestions
    if (changed)
    {
        reapplySuggestions();
    }
}

bool Solver::disassociatePlayerAndRoom(Name const & playerName, Name const & room)
{
	Player & player = players_[playerName];
	if (!player.hasRoom(room))
	{
		return false;
	}
    player.removeRoom(room);
	rooms_[room].removeHolder(playerName);
    return true;
}

bool Solver::disassociatePlayerAndWeapon(Name const & playerName, Name const & weapon)
{
	Player & player = players_[playerName];
    if (!player.hasWeapon(weapon))
	{
		return false;
	}
	player.removeWeapon(weapon);
	weapons_[weapon].removeHolder(playerName);
	return true;
}

bool Solver::disassociatePlayerAndSuspect(Name const & playerName, Name const & suspect)
{
	Player & player = players_[playerName];
	if (!player.hasSuspect(suspect))
	{
		return false;
	}
	player.removeSuspect(suspect);
	suspects_[suspect].removeHolder(playerName);
	return true;
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

bool Solver::revealSuspect(Name const & player, Name const & card)
{
    bool changed       = false;
    if (suspects_[card].holders.size() > 1)
    {
		suspects_[card].assignHolder(player);
		nobodyElseHoldsThisSuspect(players_[player], card);
		changed = true;
    }
    return changed;
}

bool Solver::revealWeapon(Name const & player, Name const & card)
{
    bool changed       = false;
    if (weapons_[card].holders.size() > 1)
    {
		weapons_[card].assignHolder(player);
		nobodyElseHoldsThisWeapon(players_[player], card);
		changed = true;
    }
    return changed;
}

bool Solver::revealRoom(Name const & player, Name const & card)
{
    bool changed       = false;
    if (rooms_[card].holders.size() > 1)
    {
		rooms_[card].assignHolder(player);
		nobodyElseHoldsThisRoom(players_[player], card);
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
		if (name == suggestion.player)
		{
			//  Do nothing for now
		}
        else if (std::find(suggestion.holders.begin(), suggestion.holders.end(), name) != suggestion.holders.end())
        {
            // The player showed a card. If the player does not have two of the cards, the player must have the third.
            bool noSuspect = !player.hasSuspect(suggestion.suspect);
            bool noWeapon  = !player.hasWeapon(suggestion.weapon);
            bool noRoom    = !player.hasRoom(suggestion.room);

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
        else if (name != ANSWER_PLAYER_NAME || suggestion.holders.size() == 3)
        {
            // The player did not show a card. Remove all suggested cards from the player.
            if (disassociatePlayerAndSuspect(name, suggestion.suspect))
                changed = true;
            if (disassociatePlayerAndWeapon(name, suggestion.weapon))
                changed = true;
            if (disassociatePlayerAndRoom(name, suggestion.room))
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

void Solver::Card::assignHolder(Name const & player)
{
	holders.clear();
	holders.push_back(player);
}

void Solver::Card::removeHolder(Name const & player)
{
	holders.erase(std::remove(holders.begin(), holders.end(), player), holders.end());
}

bool Solver::Card::isHeldBy(Name const & player) const
{
	return std::find(holders.begin(), holders.end(), player) != holders.end();
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

bool Solver::Player::hasSuspect(Name const & suspect) const
{
	return std::find(suspects.begin(), suspects.end(), suspect) != suspects.end();
}

bool Solver::Player::hasWeapon(Name const & weapon) const
{
	return std::find(weapons.begin(), weapons.end(), weapon) != weapons.end();
}

bool Solver::Player::hasRoom(Name const & room) const
{
	return std::find(rooms.begin(), rooms.end(), room) != rooms.end();
}

