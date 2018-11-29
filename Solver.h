#pragma once
#if !defined(SOLVER_H)
#define SOLVER_H 1

#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

class Solver
{
public:
    using Id     = std::string;
    using IdList = std::vector<Id>;

    //! Info associated with a card type
    struct TypeInfo
    {
        std::string name;           //!< Name of the type
        std::string title;          //!< Name of the type (suitable as a title)
        std::string preposition;    //!< Preposition used in the suggestion (if any)
        std::string article;        //!< Article used with the name (if any)
    };
    using TypeInfoList = std::map<std::string, TypeInfo>; //!< List of card types by ID

    //! Info about a card
    struct CardInfo
    {
        std::string name;           //!< Name of the card
        Id type;                    //!< Type ID
    };
    using CardInfoList = std::map<std::string, CardInfo>;   //!< List of card info by card ID

    //! Information about the rules
    struct Rules
    {
        Id id;                  //!< Id of the rules used, valid values are currently "master" and "classic"
        TypeInfoList types;     //!< Types by ID
        CardInfoList cards;     //!< Cards by ID
    };

    // Constructor
    Solver(Rules const & rules, IdList const & players);

    //! Processes a player's hand
    void hand(Id const & playerId, IdList const & cardIds);

    //! Processes a card being revealed by a player
    void show(Id const & playerId, Id const & cardId);

    //! Processes the result of a suggestion
    void suggest(Id const & playerId, IdList const & cardIds, IdList const & showed, int id);

    //! Returns a list of cards that might be held by the player
    IdList mightBeHeldBy(Id const & playerId) const;

    //! Returns a list of players that might hold the card
    IdList mightHold(Id const & cardId) const;

    //! Stores the state of the solver in a json object
    nlohmann::json toJson() const;

    //! Returns latest discoveries
    std::vector<std::string> discoveries() const { return discoveriesLog_; }

    //! Validates a list of player IDs
    bool playersAreValid(IdList const & playerIds) const;

    //! Validates a player ID
    bool playerIsValid(Id const & playerId) const;

    //! Validates a list of card IDs
    bool cardsAreValid(IdList const & cardIds) const;

    //! Validates a card ID
    bool cardIsValid(Id const & cardId) const;

    //! Validates a type ID
    bool typeIsValid(Id const & typeId) const;

    static char const * const ANSWER_PLAYER_ID;   //!< Player ID of the answer

private:
    struct Player
    {
        IdList possible;         // List of IDs of cards that the player might be holding

        void           remove(Id const & cardId);
        bool           mightHold(Id const & cardId) const;
        nlohmann::json toJson() const;
    };

    struct Card
    {
        IdList possible;         // List of IDs of players that might be holding this card
        CardInfo info;

        void           remove(Id const & playerId);
        bool           mightBeHeldBy(Id const & playerId) const;
        bool           isHeldBy(Id const & playerId) const;
        nlohmann::json toJson() const;
    };

    struct Suggestion
    {
        int id;
        Id player;
        IdList cards;
        IdList showed;         // Value depends on the rules
        nlohmann::json toJson() const;
    };

    using Fact = std::pair<std::string, Id>;

    using PlayerList     = std::map<Id, Player>;
    using CardList       = std::map<Id, Card>;
    using SuggestionList = std::vector<Suggestion>;
    using FactList       = std::map<Fact, bool>;

    void deduce(Suggestion const & suggestion, bool & changed);
    void deduce(Id const & playerId, IdList const & cardIds, bool & changed);
    void deduce(Id const & playerId, Id const & cardId, bool & changed);
    void deduceWithClassicRules(Suggestion const & suggestion, bool & changed);
    void deduceWithMasterRules(Suggestion const & suggestion, bool & changed);

    bool makeOtherDeductions(bool changed);
    void checkThatAnswerHoldsExactlyOneOfEach(bool & changed);

    void associatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed);
    void disassociatePlayerWithCard(Id const & playerId, Id const & cardId, bool & changed);
    void disassociatePlayerWithCards(Id const & playerId, IdList const & cardIds, bool & changed);
    void disassociateOtherPlayersWithCard(Id const & playerId, Id const & cardId, bool & changed);

    bool cardIsType(Id const & cardId, Id const & type) const;

    void addCardHoldersToDiscoveries();
    void addDiscovery(Id const & playerId, Id const & cardId, bool holds, std::string const & reason = std::string());

    std::string rulesId_;
    PlayerList players_;            // List of all the players by ID
    CardList cards_;                // List of all the cards by ID
    TypeInfoList types_;            // List of all card types by ID
    SuggestionList suggestions_;    // List of all suggestions
    FactList facts_;
    std::vector<std::string> discoveriesLog_;
};

#endif // !defined(SOLVER_H)
