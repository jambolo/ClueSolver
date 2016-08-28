#pragma once
#if !defined(SOLVER_H)
#define SOLVER_H 1

#include <json.hpp>
#include <map>
#include <string>
#include <vector>

class Solver
{
public:
    using Name			 = std::string;
    using NameList       = std::vector<Name>;
    using SortedNameList = std::vector<std::vector<Name> >;

    // Constructor
    Solver(std::string const & rules,
		   NameList const & players,
           NameList const & suspects,
           NameList const & weapons,
           NameList const & rooms);

    //! Processes a player's hand
    void hand(Name const & player, NameList const & cards);

    //! Processes a card being revealed by a player
    void show(Name const & player, Name const & card);

    //! Processes the result of a suggestion
    void suggest(Name const & player, NameList const & cards, NameList const & showed, int id);

    //! Processes the result of a failed accusation
    void accuse(Name const & suspect, Name const & weapon, Name const & room);

    //! Returns a sorted list of cards that might be held by the player
    SortedNameList mightBeHeldBy(Name const & player) const;

    //! Returns a list of players that might hold the card
    NameList mightHold(Name const & card) const;

    //! Store the state of the solver in a json object
    nlohmann::json toJson() const;
	
	//! Returns latest discoveries
	std::vector<std::string> discoveries() const { return discoveriesLog_; }

    static char const * const ANSWER_PLAYER_NAME;   //<! Player name of the answer

private:
    struct Player
    {
        NameList       suspects; // List of suspects the player might be holding
        NameList       weapons; // List of weapons the player might be holding
        NameList       rooms; // List of rooms the player might be holding

        void           removeSuspect(Name const & card);
        void           removeWeapon(Name const & card);
        void           removeRoom(Name const & card);
        bool           mightHaveSuspect(Name const & suspect) const;
        bool           mightHaveWeapon(Name const & weapon) const;
        bool           mightHaveRoom(Name const & room) const;
        nlohmann::json toJson() const;
    };

    struct Card
    {
        NameList       holders; // List of players that might be holding this card

        void           assignHolder(Name const & player);
        void           removeHolder(Name const & player);
        bool           mightBeHeldBy(Name const & player) const;
        nlohmann::json toJson() const;
    };

    struct Suggestion
    {
		int			   id;
        Name           player;
        Name           suspect;
        Name           weapon;
        Name           room;
        NameList       showed; // Players that showed a card
        nlohmann::json toJson() const;
    };

	typedef std::pair<std::string, std::string> Fact;

    using PlayerList     = std::map<Name, Player>;
    using CardList       = std::map<Name, Card>;
    using SuggestionList = std::vector<Suggestion>;
	using FactList = std::map<Fact, bool>;

    void revealSuspect(Name const & player, Name const & card, bool & changed);
    void revealWeapon(Name const & player, Name const & card, bool & changed);
    void revealRoom(Name const & player, Name const & card, bool & changed);

    void apply(Suggestion const & suggestion, bool & changed);
    void reapplySuggestions(bool & changed);

    void disassociatePlayerAndSuspect(Name const & playerName, Name const & suspect, bool & changed);
    void disassociatePlayerAndWeapon(Name const & playerName, Name const & weapon, bool & changed);
    void disassociatePlayerAndRoom(Name const & playerName, Name const & room, bool & changed);

    bool isSuspect(Name const & card) const;
    bool isWeapon(Name const & card) const;
    bool isRoom(Name const & card) const;

    void removeOtherPlayersFromThisSuspect(Name const & player, Name const & suspect);
    void removeOtherPlayersFromThisWeapon(Name const & player, Name const & weapon);
    void removeOtherPlayersFromThisRoom(Name const & player, Name const & room);
    
	void recordThatPlayerDoesntHoldTheseCards(Name const & name, Suggestion const & suggestion, bool & changed);
    void recordThatAnswerCanHoldOnlyOneOfEach(bool & changed);

	void recordThatAnswerCanHoldOnlyOneSuspect(bool & changed);
	void recordThatAnswerCanHoldOnlyOneWeapon(bool & changed);
	void recordThatAnswerCanHoldOnlyOneRoom(bool & changed);

	void addDiscovery(Name const & player, Name const & card, std::string const & reason, bool has);
	void checkWhoMustHoldWhichCards();

	std::string rules_;
    PlayerList players_;
    CardList suspects_;
    CardList weapons_;
    CardList rooms_;
    SuggestionList suggestions_;
	FactList facts_;
	std::vector<std::string> discoveriesLog_;
};

#endif // !defined(SOLVER_H)
