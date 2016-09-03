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
    using Id = std::string;
    using IdList = std::vector<Id>;

    struct TypeInfo
    {
        std::string name;
        std::string title;
        std::string preposition;
    };
    using TypeInfoList = std::map<std::string, TypeInfo>;

    struct CardInfo
    {
        std::string name;
        std::string type;
    };
    using CardInfoList = std::map<std::string, CardInfo>;

    struct Rules
    {
        std::string  id;
        TypeInfoList types;     // Types by ID
        CardInfoList cards;     // Cards by ID
    };

    // Constructor
    Solver(Rules const & rules, IdList const & players);

    //! Processes a player's hand
    void hand(Id const & playerId, IdList const & cardIds);

    //! Processes a card being revealed by a player
    void show(Id const & playerId, Id const & cardId);

    //! Processes the result of a suggestion
    void suggest(Id const & playerId, IdList const & cards, IdList const & showed, int id);

    //! Returns a list of cards that might be held by the player
    IdList mightBeHeldBy(Id const & playerId) const;

    //! Returns a list of players that might hold the card
    IdList mightHold(Id const & cardId) const;

    //! Stores the state of the solver in a json object
    nlohmann::json toJson() const;

    //! Returns latest discoveries
    std::vector<std::string> discoveries() const { return discoveriesLog_; }

    // Validates a list of player IDs
    bool playersAreValid(IdList const & playerIds) const;

    // Validates a player ID
    bool playerIsValid(Id const & playerId) const;

    // Validates a list of card IDs
    bool cardsAreValid(IdList const & cardIds) const;

    // Validates a card ID
    bool cardIsValid(Id const & cardId) const;

    // Validates a type ID
    bool typeIsValid(Id const & typeId) const;

    static char const * const ANSWER_PLAYER_ID;   //<! Player id of the answer

private:
    struct Player
    {
        IdList         cards; // List of IDs of cards that the player might be holding

        void           remove(Id const & cardId);
        bool           mightHold(Id const & cardId) const;
        nlohmann::json toJson() const;
    };

    struct Card
    {
        IdList         holders; // List of IDs of players that might be holding this card
        CardInfo       info;

        void           remove(Id const & playerId);
        bool           mightBeHeldBy(Id const & playerId) const;
        bool           isHeldBy(Id const & playerId) const;
        nlohmann::json toJson() const;
    };

    struct Suggestion
    {
        int            id;
        Id             player;
        IdList         cards;
        IdList         showed; // Value depends on the rules
        nlohmann::json toJson() const;
    };

    typedef std::pair<std::string, Id> Fact;

    using PlayerList     = std::map<Id, Player>;
    using CardList       = std::map<Id, Card>;
    using SuggestionList = std::vector<Suggestion>;
    using FactList       = std::map<Fact, bool>;

    void deduce(Suggestion const & suggestion, bool & changed);
    void deduce(Id const & playerId, IdList const & cardIds, bool & changed);
    void deduce(Id const & playerId, Id const & cardId, bool & changed);

    bool makeOtherDeductions(bool changed);
    void checkThatAnswerHoldsOnlyOneOfEach(bool & changed);

    void associatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed);
    void disassociatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed);
    void disassociatePlayerWithCards(Id const & playerId, IdList const & cardIds, bool & changed);
    void disassociateOtherPlayersWithCard(Id const & playerId, Id const & cardId, bool & changed);

    bool cardIsType(Id const & cardId, Id const & type) const;

    void addDiscoveredCardHolders();
    void addDiscovery(Id const & playerId, Id const & cardId, std::string const & reason, bool holds);

    std::string rulesId_;
    PlayerList players_;            // List of all the players by id
    CardList cards_;                // List of all the cards by id
    TypeInfoList types_;            // List of all card types by id
    SuggestionList suggestions_;    // List of all suggestions by id
    FactList facts_;
    std::vector<std::string> discoveriesLog_;
};

#endif // !defined(SOLVER_H)
