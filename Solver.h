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

    //! Processes a card being revealed by a player
    void reveal(Name const & player, Name const & card);

    //! Processes the result of a suggestion
    void suggest(Name const & suspect, Name const & weapon, Name const & room, NameList const & players);

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
    };

    struct Card
    {
        NameList players;   // List of players that might be holding this card
    };

    struct Suggestion
    {
        Name     suspect;
        Name     weapon;
        Name     room;
        NameList players;   // Players that showed a card
    };

    using PlayerList     = std::map<Name, Player>;
    using CardList       = std::map<Name, Card>;
    using SuggestionList = std::vector<Suggestion>;

    bool revealSuspect(Name const & player, Name const & card);
    bool revealWeapon(Name const & player, Name const & card);
    bool revealRoom(Name const & player, Name const & card);
    bool apply(Suggestion const & suggestion);
    void reapplySuggestions();
    bool disassociateSuspect(Player & player, Name const & suspect);
    bool disassociateWeapon(Player & player, Name const & weapon);
    bool disassociateRoom(Player & player, Name const & room);

    PlayerList players_;
    CardList suspects_;
    CardList weapons_;
    CardList rooms_;
    SuggestionList suggestions_;
};

#endif // !defined(SOLVER_H)
