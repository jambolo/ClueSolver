#include "Solver.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

namespace
{
bool loadConfiguration(char const * name);
void listTypes(std::ostream & out, Solver::TypeInfoList const & types);
void listCards(std::ostream &               out,
               Solver::Id const &           typeId,
               Solver::TypeInfoList const & typeInfo,
               Solver::CardInfoList const & cards);

Solver::TypeInfoList s_types =
{
    { "suspect",    { "suspect", "Suspects", "",      ""     } },
    { "weapon",     { "weapon",  "Weapons",  "with ", "the " } },
    { "room",       { "room",    "Rooms",    "in ",   "the " } }
};

Solver::CardInfoList s_cards =
{
    { "mustard",      { "Colonel Mustard", "suspect" } },
    { "white",        { "Mrs. White",      "suspect" } },
    { "plum",         { "Professor Plum",  "suspect" } },
    { "peacock",      { "Mrs. Peacock",    "suspect" } },
    { "green",        { "Mr. Green",       "suspect" } },
    { "scarlet",      { "Miss Scarlet",    "suspect" } },
    { "revolver",     { "Revolver",        "weapon"  } },
    { "knife",        { "Knife",           "weapon"  } },
    { "rope",         { "Rope",            "weapon"  } },
    { "pipe",         { "Lead pipe",       "weapon"  } },
    { "wrench",       { "Wrench",          "weapon"  } },
    { "candlestick",  { "Candlestick",     "weapon"  } },
    { "dining",       { "Dining room",     "room"    } },
    { "conservatory", { "Conservatory",    "room"    } },
    { "kitchen",      { "Kitchen",         "room"    } },
    { "study",        { "Study",           "room"    } },
    { "library",      { "Library",         "room"    } },
    { "billiard",     { "Billiard room",   "room"    } },
    { "lounge",       { "Lounge",          "room"    } },
    { "ballroom",     { "Ballroom",        "room"    } },
    { "hall",         { "Hall",            "room"    } }
};

std::string             s_rules = "classic";
std::vector<Solver::Id> s_players;

void outputSuggestion(std::ostream &         out,
                      int                    id,
                      Solver::Id const &     player,
                      Solver::IdList const & cards,
                      Solver::IdList const & results)
{
    out << '(' << std::setw(2) << id << ") " << player << " suggested";

    for (auto const & c : cards)
    {
        Solver::CardInfo const & cardInfo = s_cards[c];
        out << " " << s_types[cardInfo.type].preposition << s_types[cardInfo.type].article << cardInfo.name;
    }

    out << " ==> ";
    if (results.empty())
    {
        out << "nobody has them";
    }
    else
    {
        if (s_rules == "master")
        {
            out << results[0];
            for (size_t i = 1; i < results.size(); ++i)
            {
                out << ", " << results[i];
            }
            out << " showed a card";
        }
        else
        {
            if (results.size() > 1)
            {
                out << results[0];
                for (size_t i = 1; i < results.size()-1; ++i)
                {
                    out << ", " << results[i];
                }
                out << " had nothing, but ";
            }
            out << results.back() << " showed a card";
        }
    }
    out << std::endl;
}

void outputShow(std::ostream & out, Solver::Id const & player, Solver::Id const & card)
{
    Solver::CardInfo const & cardInfo = s_cards[card];
    out << "---- " << player << " showed " << s_types[cardInfo.type].article << s_cards[card].name << std::endl;
}

void outputHand(std::ostream & out, Solver::Id const & player, Solver::IdList const & cards)
{
    out << "**** " << player << " has this hand: ";
    out << s_cards[cards[0]].name;
    for (size_t i = 1; i < cards.size(); ++i)
    {
        out << ", " << s_cards[cards[i]].name;
    }
    out << std::endl;
}

void outputAccusation(std::ostream & out, int id, Solver::Id const & player, Solver::IdList const & cards, bool correct)
{
    out << '(' << std::setw(2) << id << ") " << player << " accused";

    for (auto const & c : cards)
    {
        Solver::CardInfo const & cardInfo = s_cards[c];
        out << " " << s_types[cardInfo.type].preposition << s_types[cardInfo.type].article << cardInfo.name;
    }

    out << " ==> " << (correct ? "correct" : "wrong");
    out << std::endl;
}

} // anonymous namespace

int main(int argc, char ** argv)
{
    char *         configurationFileName = nullptr;
    char *         inputFileName         = nullptr;
    char *         outputFileName        = nullptr;
    std::ifstream  infilestream;
    std::ofstream  outfilestream;
    std::istream * in  = &std::cin;
    std::ostream * out = &std::cout;

    while (--argc > 0)
    {
        ++argv;
        if (**argv == '-')
        {
            switch ((*argv)[1])
            {
                case 'c':
                    if (--argc > 0)
                        configurationFileName = *++argv;
                    break;
                case 'o':
                    if (--argc > 0)
                        outputFileName = *++argv;
                    break;
            }
        }
        else
        {
            if (!inputFileName)
                inputFileName = *argv;
        }
    }

    // Load configuration
    if (configurationFileName)
    {
        if (!loadConfiguration(configurationFileName))
        {
            std::cerr << "Cannot load the configuration from '" << inputFileName << "'" << std::endl;
            exit(1);
        }
    }

    if (inputFileName)
    {
        infilestream.open(inputFileName);
        if (infilestream.is_open())
        {
            in = &infilestream;
        }
        else
        {
            std::cerr << "Cannot open '" << inputFileName << "' for reading." << std::endl;
            exit(2);
        }
    }

    if (outputFileName)
    {
        outfilestream.open(outputFileName);
        if (outfilestream.is_open())
        {
            out = &outfilestream;
        }
        else
        {
            std::cerr << "Cannot open '" << outputFileName << "' for writing." << std::endl;
            exit(3);
        }
    }

    *out << "Rules: " << s_rules << std::endl;
    listTypes(*out, s_types);
    for (auto const & t : s_types)
    {
        listCards(*out, t.first, s_types, s_cards);
    }

    *out << std::endl;

    // Load player list
    Solver::Id input;
    std::getline(*in, input);
    s_players = json::parse(input);

    *out << "players = " << json(s_players).dump() << std::endl;
    *out << std::endl;

    int           suggestionId = 0;
    int           accusationId = 0;
    Solver::Rules rules        = { s_rules, s_types, s_cards };
    Solver        solver(rules, s_players);
    while (true)
    {
        std::getline(*in, input);
        if (in->eof())
            break;
        try
        {
            json event = json::parse(input);

            if (event.find("show") != event.end())
            {
                auto       s      = event["show"];
                Solver::Id player = s["player"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                Solver::Id card = s["card"];
                if (!solver.cardIsValid(card))
                    throw std::domain_error("Invalid card");
                outputShow(*out, player, card);
                solver.show(player, card);
            }
            else if (event.find("suggest") != event.end())
            {
                auto       s      = event["suggest"];
                Solver::Id player = s["player"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                Solver::IdList cards = s["cards"];
                if (!solver.cardsAreValid(cards))
                    throw std::domain_error("Invalid cards");
                Solver::IdList showed = s["showed"];
                if (!solver.playersAreValid(showed))
                    throw std::domain_error("Invalid players");
                outputSuggestion(*out, suggestionId, player, cards, showed);
                solver.suggest(player, cards, showed, suggestionId);
                ++suggestionId;
            }
            else if (event.find("hand") != event.end())
            {
                auto           h      = event["hand"];
                Solver::Id     player = h["player"];
                Solver::IdList cards  = h["cards"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                if (!solver.cardsAreValid(cards))
                    throw std::domain_error("Invalid hand");
                outputHand(*out, player, cards);
                solver.hand(player, cards);
            }
            else if (event.find("accuse") != event.end())
            {
                auto       s = event["accuse"];
                Solver::Id player = s["player"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                Solver::IdList cards = s["cards"];
                if (!solver.cardsAreValid(cards))
                    throw std::domain_error("Invalid cards");
                bool correct = s["correct"];
                outputAccusation(*out, suggestionId, player, cards, correct);
                solver.accuse(player, cards, correct, accusationId);
                ++accusationId;
            }
            else
            {
                throw std::domain_error("Invalid event type");
            }

            {
                std::vector<std::string> discoveries = solver.discoveries();
                for (auto const & d : discoveries)
                {
                    *out << "     -> " << d << std::endl;
                }
            }

//            *out << "state = " << solver.toJson().dump() << std::endl;
            *out << "ANSWER: " << json(solver.mightBeHeldBy(Solver::ANSWER_PLAYER_ID)).dump() << std::endl;
            *out << std::endl;
        }
        catch (std::exception e)
        {
            std::cerr << e.what() << ": '" << input << "'" << std::endl;
        }
    }
    return 0;
}

namespace
{
//    {
//        "types" : [
//            { "id" : "suspect", "name" : "suspect",  "title" : "Suspects", "article" : "",     "preposition" : ""      },
//            { "id" : "weapon",  "name" : "weapon",   "title" : "Weapons",  "article" : "the ", "preposition" : "with " },
//            { "id" : "room",    "name" : "room",     "title" : "Rooms",    "article" : "the ",  preposition" : "in "   }
//        ]
// }
//    {
//        "cards" : [
//            { "id" : "mustard", "name" : "Colonel Mustard", "type" : "suspect" },
//            { "id" : "knife",   "name" : "Knife",           "type" : "weapon" },
//            { "id" : "studio",  "name" : "Studio",          "type" : "room" }
//        ]
// }

bool loadConfiguration(char const * name)
{
    std::ifstream file(name);
    if (!file.is_open())
        return false;

    try
    {
        json j;
        file >> j;

        s_rules = j["rules"];
        s_types.clear();
        s_cards.clear();

        json jtypes = j["types"];
        for (auto const & a : jtypes)
        {
            if (a.find("id") == a.end() ||
                a.find("name") == a.end() ||
                a.find("title") == a.end() ||
                a.find("article") == a.end() ||
                a.find("preposition") == a.end())
            {
                throw std::domain_error("Invalid card type, missing information.");
            }

            Solver::TypeInfo type = { a["name"], a["title"], a["article"], a["preposition"] };
            s_types[a["id"]] = type;
        }

        json jcards = j["cards"];
        for (auto const & c : jcards)
        {
            if (c.find("id") == c.end() || c.find("name") == c.end() || c.find("type") == c.end())
                throw std::domain_error("Invalid card configuration.");

            Solver::CardInfo card = { c["name"], c["type"] };
            s_cards[c["id"]] = card;
        }
    }
    catch (std::exception e)
    {
        std::cout << "Failed to load configuration file: " << e.what() << std::endl;
        return false;
    }

    return true;
}

void listCards(std::ostream &               out,
               Solver::Id const &           typeId,
               Solver::TypeInfoList const & typeInfo,
               Solver::CardInfoList const & cards)
{
    out << typeInfo.find(typeId)->second.title << ": ";
    bool first = true;
    for (auto const & c : cards)
    {
        if (c.second.type == typeId)
        {
            if (!first)
                out << ", ";
            out << c.second.name;
            first = false;
        }
    }
    out << std::endl;
}

void listTypes(std::ostream & out, Solver::TypeInfoList const & types)
{
    out << "Types: ";
    bool first = true;
    for (auto const & t : types)
    {
        if (!first)
            out << ", ";
        out << t.second.name;
        first = false;
    }
    out << std::endl;
}
} // anonymous namespace
