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
    using Name = std::string;
    using NameList       = std::vector<Name>;
    using SortedNameList = std::vector<std::vector<Name> >;

    // Constructor
    Solver(NameList const & players,
           NameList const & suspects,
           NameList const & weapons,
           NameList const & rooms);

    //! Processes a player's hand
    void hand(Name const & player, NameList const & cards);

    //! Processes a card being revealed by a player
    void show(Name const & player, Name const & card);

    //! Processes the result of a suggestion
    void suggest(Name const & player, NameList const & cards, NameList const & holders);

    //! Processes the result of a failed accusation
    void accuse(Name const & suspect, Name const & weapon, Name const & room);

    //! Returns a sorted list of cards that might be held by the player
    SortedNameList mightBeHeldBy(Name const & player) const;

    //! Returns a list of players that might hold the card
    NameList mightHold(Name const & card) const;

    //! Store the state of the solver in a json object
    nlohmann::json toJson() const;

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
        Name           player;
        Name           suspect;
        Name           weapon;
        Name           room;
        NameList       holders; // Players that showed a card
        nlohmann::json toJson() const;
    };

    using PlayerList     = std::map<Name, Player>;
    using CardList       = std::map<Name, Card>;
    using SuggestionList = std::vector<Suggestion>;

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

    void nobodyElseHoldsThisSuspect(Player & player, Name const & suspect);
    void nobodyElseHoldsThisWeapon(Player & player, Name const & weapon);
    void nobodyElseHoldsThisRoom(Player & player, Name const & room);
    void playerDoesntHaveTheseCards(Name const & name, Suggestion const & suggestion, bool & changed);
    void answerHasOnlyOneOfEach(bool & changed);

    PlayerList players_;
    CardList suspects_;
    CardList weapons_;
    CardList rooms_;
    SuggestionList suggestions_;
};

#endif // !defined(SOLVER_H)
