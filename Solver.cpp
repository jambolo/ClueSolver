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

Solver::Solver(std::string const & rules,
	NameList const & players,
               NameList const & suspects,
               NameList const & weapons,
               NameList const & rooms)
	: rules_(rules)
{
	assert(rules == "classic" || rules == "master");

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
    discoveriesLog_.clear();
    bool changed    = false;
    Player & player = players_[name];

    for (auto & s : suspects_)
    {
        if (std::find(cards.begin(), cards.end(), s.first) != cards.end())
        {
            addDiscovery(name, s.first, "hand", true);
            revealSuspect(name, s.first, changed);
        }
        else
        {
            addDiscovery(name, s.first, "hand", false);
            disassociatePlayerAndSuspect(name, s.first, changed);
        }
    }

    for (auto & w : weapons_)
    {
        if (std::find(cards.begin(), cards.end(), w.first) != cards.end())
        {
            addDiscovery(name, w.first, "hand", true);
            revealWeapon(name, w.first, changed);
        }
        else
        {
            addDiscovery(name, w.first, "hand", false);
            disassociatePlayerAndWeapon(name, w.first, changed);
        }
    }

    for (auto & r : rooms_)
    {
        if (std::find(cards.begin(), cards.end(), r.first) != cards.end())
        {
            addDiscovery(name, r.first, "hand", true);
            revealRoom(name, r.first, changed);
        }
        else
        {
            addDiscovery(name, r.first, "hand", false);
            disassociatePlayerAndRoom(name, r.first, changed);
        }
    }
	checkWhoMustHoldWhichCards();
}

void Solver::show(Name const & player, Name const & card)
{
    discoveriesLog_.clear();

    bool changed = false;
    if (isSuspect(card))
    {
        addDiscovery(player, card, "revealed", true);
        revealSuspect(player, card, changed);
    }
    else if (isWeapon(card))
    {
        addDiscovery(player, card, "revealed", true);
        revealWeapon(player, card, changed);
    }
    else
    {
        assert(isRoom(card));
        addDiscovery(player, card, "revealed", true);
        revealRoom(player, card, changed);
    }

    recordThatAnswerCanHoldOnlyOneOfEach(changed);

    // If something changed, then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        reapplySuggestions(changed);
        recordThatAnswerCanHoldOnlyOneOfEach(changed);
    }
	checkWhoMustHoldWhichCards();
}

void Solver::suggest(Name const & player, NameList const & cards, NameList const & showed, int id)
{
    discoveriesLog_.clear();

    bool changed = false;

    Suggestion suggestion;
    suggestion.id     = id;
    suggestion.player = player;
    for (auto const & c : cards)
    {
        if (isSuspect(c))
        {
            suggestion.suspect = c;
        }
        else if (isWeapon(c))
        {
            suggestion.weapon = c;
        }
        else
        {
            assert(isRoom(c));
            suggestion.room = c;
        }
    }
    suggestion.showed = showed;

    // Apply the suggestion
    apply(suggestion, changed);

    // Remember the suggestion
    suggestions_.push_back(suggestion);

	recordThatAnswerCanHoldOnlyOneOfEach(changed);

    // While anything has changed, then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        reapplySuggestions(changed);
		recordThatAnswerCanHoldOnlyOneOfEach(changed);
    }
	checkWhoMustHoldWhichCards();
}

void Solver::accuse(Name const & suspect, Name const & weapon, Name const & room)
{
    discoveriesLog_.clear();

    // A failed accusation means that the answer does not hold those cards.

    bool changed = false;

    disassociatePlayerAndSuspect(ANSWER_PLAYER_NAME, suspect, changed);
    disassociatePlayerAndWeapon(ANSWER_PLAYER_NAME, weapon, changed);
    disassociatePlayerAndRoom(ANSWER_PLAYER_NAME, room, changed);
	checkWhoMustHoldWhichCards();
    recordThatAnswerCanHoldOnlyOneOfEach(changed);

    // Then re-apply all the suggestions
    while (changed)
    {
        changed = false;
        reapplySuggestions(changed);
		recordThatAnswerCanHoldOnlyOneOfEach(changed);
    }
	checkWhoMustHoldWhichCards();
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

void Solver::revealSuspect(Name const & player, Name const & card, bool & changed)
{
    Card & suspect = suspects_[card];
    if (suspect.holders.size() == 1)
    {
        assert(suspect.holders[0] == player);
        return;
    }

    suspect.assignHolder(player);
    removeOtherPlayersFromThisSuspect(player, card);
    changed = true;
}

void Solver::revealWeapon(Name const & player, Name const & card, bool & changed)
{
    Card & weapon = weapons_[card];
    if (weapon.holders.size() == 1)
    {
        assert(weapon.holders[0] == player);
        return;
    }

    weapon.assignHolder(player);
    removeOtherPlayersFromThisWeapon(player, card);
    changed = true;
}

void Solver::revealRoom(Name const & player, Name const & card, bool & changed)
{
    Card & room = rooms_[card];
    if (room.holders.size() == 1)
    {
        assert(room.holders[0] == player);
        return;
    }

    room.assignHolder(player);
    removeOtherPlayersFromThisRoom(player, card);
    changed = true;
}

void Solver::apply(Suggestion const & suggestion, bool & changed)
{
    Name const & suggester  = suggestion.player;
    Name const & suspect    = suggestion.suspect;
    Name const & weapon     = suggestion.weapon;
    Name const & room       = suggestion.room;
    NameList const & showed = suggestion.showed;
    int id                  = suggestion.id;

	if (rules_ == "master")
	{
		// You can infer from a suggestion that:
		//		If a player shows a card but does not hold two of the cards, the player must hold the third.
		//		If a player (other than the answer and suggester) does not show a card, the player has none of those cards.
		//		If all three cards are shown, then the answer and the suggester hold none of those cards.

		for (auto const & p : players_)
		{
			Name const & name = p.first;
			if (std::find(showed.begin(), showed.end(), name) != showed.end())
			{
				// The player showed a card. If the player does not hold two of the three cards, the player must hold the third.
				Player const & player = p.second;
				bool noSuspect = !player.mightHaveSuspect(suspect);
				bool noWeapon = !player.mightHaveWeapon(weapon);
				bool noRoom = !player.mightHaveRoom(room);

				if (noSuspect && noWeapon)
				{
					addDiscovery(name,
						room,
						"showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others", true);
					revealRoom(name, room, changed);
				}
				else if (noSuspect && noRoom)
				{
					addDiscovery(name,
						weapon,
						"showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others", true);
					revealWeapon(name, weapon, changed);
				}
				else if (noWeapon && noRoom)
				{
					addDiscovery(name,
						suspect,
						"showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others", true);
					revealSuspect(name, suspect, changed);
				}
			}
			else if (name != ANSWER_PLAYER_NAME && name != suggester)
			{
				// If the player must show cards but did not, then they don't hold any of them.
				addDiscovery(name, suspect, "did not show a card in suggestion #" + std::to_string(id), false);
				addDiscovery(name, weapon, "did not show a card in suggestion #" + std::to_string(id), false);
				addDiscovery(name, room, "did not show a card in suggestion #" + std::to_string(id), false);
				recordThatPlayerDoesntHoldTheseCards(name, suggestion, changed);
			}
			else if (showed.size() == 3)
			{
				// If all three cards were shown, then even players that don't show cards don't hold them.
				addDiscovery(name,
					suspect,
					"all three cards were shown by other players in suggestion #" + std::to_string(id), false);
				addDiscovery(name,
					weapon,
					"all three cards were shown by other players in suggestion #" + std::to_string(id), false);
				addDiscovery(name,
					room,
					"all three cards were shown by other players in suggestion #" + std::to_string(id), false);
				recordThatPlayerDoesntHoldTheseCards(name, suggestion, changed);
			}
		}
	}
	else
	{
		// You can infer from a suggestion that:
		//		If the showed list is empty, then none of the players (except possibly the suggester or the answer) have the cards.
		//		All the players in the showed list but the last hold none of the cards.
		//		If the player that showed a card does not hold two of the cards, the player must hold the third.

		if (showed.empty())
		{
			for (auto const & p : players_)
			{
				Name const & name = p.first;
				if (name != ANSWER_PLAYER_NAME && name != suggester)
				{
					addDiscovery(name, suspect, "did not show a card in suggestion #" + std::to_string(id), false);
					addDiscovery(name, weapon, "did not show a card in suggestion #" + std::to_string(id), false);
					addDiscovery(name, room, "did not show a card in suggestion #" + std::to_string(id), false);
					recordThatPlayerDoesntHoldTheseCards(name, suggestion, changed);
				}
			}
		}
		else
		{
			// For all but the last
			for (int i = 0; i < showed.size() - 1; ++i)
			{
				Name const & name = showed[i];
				addDiscovery(name, suspect, "did not show a card in suggestion #" + std::to_string(id), false);
				addDiscovery(name, weapon, "did not show a card in suggestion #" + std::to_string(id), false);
				addDiscovery(name, room, "did not show a card in suggestion #" + std::to_string(id), false);
				recordThatPlayerDoesntHoldTheseCards(name, suggestion, changed);
			}

			{
				// The last player showed a card. If the player does not hold two of the three cards, the player must hold the
				// third.
				Name const & name = showed[showed.size() - 1];
				Player const & player = players_[name];
				bool noSuspect = !player.mightHaveSuspect(suspect);
				bool noWeapon = !player.mightHaveWeapon(weapon);
				bool noRoom = !player.mightHaveRoom(room);

				if (noSuspect && noWeapon)
				{
					addDiscovery(name,
						room,
						"showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others", true);
					revealRoom(name, room, changed);
				}
				else if (noSuspect && noRoom)
				{
					addDiscovery(name,
						weapon,
						"showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others", true);
					revealWeapon(name, weapon, changed);
				}
				else if (noWeapon && noRoom)
				{
					addDiscovery(name,
						suspect,
						"showed a card in suggestion #" + std::to_string(id) + ", and does not hold the others", true);
					revealSuspect(name, suspect, changed);
				}
			}
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

void Solver::removeOtherPlayersFromThisSuspect(Name const & player, Name const & suspect)
{
    for (auto & p : players_)
    {
        if (p.first != player)
        {
            p.second.removeSuspect(suspect);
        }
    }
}

void Solver::removeOtherPlayersFromThisWeapon(Name const & player, Name const & weapon)
{
    for (auto & p : players_)
    {
        if (p.first != player)
        {
            p.second.removeWeapon(weapon);
        }
    }
}

void Solver::removeOtherPlayersFromThisRoom(Name const & player, Name const & room)
{
    for (auto & p : players_)
    {
        if (p.first != player)
        {
            p.second.removeRoom(room);
        }
    }
}

void Solver::recordThatPlayerDoesntHoldTheseCards(Name const & name, Suggestion const & suggestion, bool & changed)
{
    // Remove all suggested cards from the player.
    disassociatePlayerAndSuspect(name, suggestion.suspect, changed);
    disassociatePlayerAndWeapon(name, suggestion.weapon, changed);
    disassociatePlayerAndRoom(name, suggestion.room, changed);
}

void Solver::recordThatAnswerCanHoldOnlyOneOfEach(bool & changed)
{
    recordThatAnswerCanHoldOnlyOneSuspect(changed);
    recordThatAnswerCanHoldOnlyOneWeapon(changed);
    recordThatAnswerCanHoldOnlyOneRoom(changed);
}

void Solver::recordThatAnswerCanHoldOnlyOneSuspect(bool & changed)
{
    Player & answer = players_[ANSWER_PLAYER_NAME];

    // See if there is a suspect that is held by the answer
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

    // If so, then the answer can not hold any other suspects
    if (!suspect.empty())
    {
        NameList suspects = answer.suspects;
        for (auto const & s : suspects)
        {
            if (s != suspect)
            {
                addDiscovery(ANSWER_PLAYER_NAME, s, "ANSWER can only hold one suspect", false);
                disassociatePlayerAndSuspect(ANSWER_PLAYER_NAME, s, changed);
            }
        }
    }
}

void Solver::recordThatAnswerCanHoldOnlyOneWeapon(bool & changed)
{
    Player & answer = players_[ANSWER_PLAYER_NAME];

    // See if there is a weapon that is held by the answer
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

    // If so, then the answer can not hold any other weapons
    if (!weapon.empty())
    {
        NameList weapons = answer.weapons;
        for (auto const & w : weapons)
        {
            if (w != weapon)
            {
                addDiscovery(ANSWER_PLAYER_NAME, w, "ANSWER can only hold one weapon", false);
                disassociatePlayerAndWeapon(ANSWER_PLAYER_NAME, w, changed);
            }
        }
    }
}

void Solver::recordThatAnswerCanHoldOnlyOneRoom(bool & changed)
{
    Player & answer = players_[ANSWER_PLAYER_NAME];

    // See if there is a room that is held by the answer
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

    // If so, then the answer can not hold any other rooms
    if (!room.empty())
    {
        NameList rooms = answer.rooms;
        for (auto const & r : rooms)
        {
            if (r != room)
            {
                addDiscovery(ANSWER_PLAYER_NAME, r, "ANSWER can only hold one room", false);
                disassociatePlayerAndRoom(ANSWER_PLAYER_NAME, r, changed);
            }
        }
    }
}

void Solver::addDiscovery(Name const & player, Name const & card, std::string const & reason, bool has)
{
    auto fact = std::make_pair(player, card);
    auto f    = facts_.find(fact);
    if (f == facts_.end())
    {
        std::string discovery = player + (has ? " holds " : " does not hold ") + card + ": " + reason;
        discoveriesLog_.push_back(discovery);
        facts_[fact] = has;
    }
    else
    {
        assert(f->second == has);
    }
}

void Solver::checkWhoMustHoldWhichCards()
{
	for (auto & s : suspects_)
	{
		NameList const & holders = s.second.holders;
		if (holders.size() == 1)
			addDiscovery(holders[0], s.first, "nobody else holds it", true);
	}

	for (auto & w : weapons_)
	{
		NameList const & holders = w.second.holders;
		if (holders.size() == 1)
			addDiscovery(holders[0], w.first, "nobody else holds it", true);
	}

	for (auto & r : rooms_)
	{
		NameList const & holders = r.second.holders;
		if (holders.size() == 1)
			addDiscovery(holders[0], r.first, "nobody else holds it", true);
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
    j["showed"]  = showed;
    return j;
}
