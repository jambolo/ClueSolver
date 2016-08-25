#pragma once
#if !defined(SOLVER_H)
#define SOLVER_H 1

#include <map>
#include <string>
#include <vector>

class Solver
{
public:
    using Name     = std::string;
    using NameList = std::vector<Name>;

    // Constructor
    Solver(NameList const & players,
           NameList const & suspects,
           NameList const & weapons,
           NameList const & rooms);

	//! Processes a player's hand
	void hand(Name const & player, NameList const & cards);

    //! Processes a card being revealed by a player
    void reveal(Name const & player, Name const & card);

    //! Processes the result of a suggestion
    void suggest(Name const & player, Name const & suspect, Name const & weapon, Name const & room, NameList const & holders);

    //! Processes the result of a failed accusation
	void accuse(Name const & suspect, Name const & weapon, Name const & room);

    //! Returns a list of cards that might be held by the player
    NameList mightBeHeldBy(Name const & player) const;

    //! Returns a list of players that might hold the card
    NameList mightHold(Name const & card) const;

    // Player name of the answer
    static char const * const ANSWER_PLAYER_NAME;

private:
    struct Player
    {
        NameList suspects;  // List of suspects the player might be holding
        NameList weapons;   // List of weapons the player might be holding
        NameList rooms;     // List of rooms the player might be holding

		void removeSuspect(Name const & card);
		void removeWeapon(Name const & card);
		void removeRoom(Name const & card);
		bool hasSuspect(Name const & suspect) const;
		bool hasWeapon(Name const & weapon) const;
		bool hasRoom(Name const & room) const;
	};

    struct Card
    {
        NameList holders;   // List of players that might be holding this card

		void assignHolder(Name const & player);
		void removeHolder(Name const & player);
		bool isHeldBy(Name const & player) const;
	};

    struct Suggestion
    {
		Name     player;
        Name     suspect;
        Name     weapon;
        Name     room;
        NameList holders;   // Players that showed a card
    };

    using PlayerList     = std::map<Name, Player>;
    using CardList       = std::map<Name, Card>;
    using SuggestionList = std::vector<Suggestion>;

    bool revealSuspect(Name const & player, Name const & card);
    bool revealWeapon(Name const & player, Name const & card);
    bool revealRoom(Name const & player, Name const & card);

	bool apply(Suggestion const & suggestion);
    void reapplySuggestions();

	bool disassociatePlayerAndSuspect(Name const & playerName, Name const & suspect);

	void hasSuspect(Player &player, Name const & suspect);

	bool disassociatePlayerAndWeapon(Name const & playerName, Name const & weapon);
    bool disassociatePlayerAndRoom(Name const & playerName, Name const & room);

	bool isSuspect(Name const & card) const;
	bool isWeapon(Name const & card) const;
	bool isRoom(Name const & card) const;

	void nobodyElseHoldsThisSuspect(Player & player, Name const & suspect);
	void nobodyElseHoldsThisWeapon(Player & player, Name const & weapon);
	void nobodyElseHoldsThisRoom(Player & player, Name const & room);

    PlayerList players_;
    CardList suspects_;
    CardList weapons_;
    CardList rooms_;
    SuggestionList suggestions_;
};

#endif // !defined(SOLVER_H)
