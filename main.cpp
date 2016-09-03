#include "Solver.h"

#include <json.hpp>

#include <fstream>
#include <iomanip>

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
    { "suspect",    { "suspect", "Suspects", ""            } },
    { "weapon",     { "weapon",  "Weapons",  "with the"    } },
    { "room",       { "room",    "Rooms",    "in the"      } }
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

std::string s_rules = "classic";
std::vector<Solver::Id> s_players;
} // anonymous namespace

int main(int argc, char ** argv)
{
    char * configurationFileName = nullptr;
    char * inputFileName         = nullptr;
    char * outputFileName        = nullptr;
    std::ifstream infilestream;
    std::ofstream outfilestream;
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

    int suggestionId    = 0;
    Solver::Rules rules = { s_rules, s_types, s_cards };
    Solver solver(rules, s_players);
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
                *out << "---- " << input << std::endl;
                auto s = event["show"];
                Solver::Id player = s["player"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                Solver::Id card = s["card"];
                if (!solver.cardIsValid(card))
                    throw std::domain_error("Invalid card");
                solver.show(player, card);
            }
            else if (event.find("suggest") != event.end())
            {
                *out << '(' << std::setw(2) << suggestionId << ") " << input << std::endl;
                auto s = event["suggest"];
                Solver::Id player = s["player"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                Solver::IdList cards = s["cards"];
                if (!solver.cardsAreValid(cards))
                    throw std::domain_error("Invalid cards");
                Solver::IdList showed = s["showed"];
                if (!solver.playersAreValid(showed))
                    throw std::domain_error("Invalid players");
                solver.suggest(player, cards, showed, suggestionId);
                ++suggestionId;
            }
            else if (event.find("hand") != event.end())
            {
                *out << "**** " << input << std::endl;
                auto h = event["hand"];
                Solver::Id player    = h["player"];
                Solver::IdList cards = h["cards"];
                if (!solver.playerIsValid(player))
                    throw std::domain_error("Invalid player");
                if (!solver.cardsAreValid(cards))
                    throw std::domain_error("Invalid hand");
                solver.hand(player, cards);
            }
            else
            {
                throw std::domain_error("Invalid event type");
            }

            {
                std::vector<std::string> discoveries = solver.discoveries();
                for (auto const & d : discoveries)
                {
                    *out << d << std::endl;
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
//            { "id" : "suspect", "name" : "Suspect", "preposition" : ""      },
//            { "id" : "weapon",  "name" : "Weapon",  "preposition" : "with the" },
//            { "id" : "room",    "name" : "Room",    "preposition" : "in the" }
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
        json jtypes = j["types"];
        for (auto const & a : jtypes)
        {
            if (a.find("id") == a.end() || a.find("name") == a.end() || a.find("preposition") == a.end())
                throw std::domain_error("Invalid card type, missing information.");

            Solver::TypeInfo type = { a["name"], a["preposition"] };
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
    for (auto const & c : cards)
    {
        if (c.second.type == typeId)
        {
            out << "'" << c.second.name << "' ";
        }
    }
    out << std::endl;
}

void listTypes(std::ostream & out, Solver::TypeInfoList const & types)
{
    out << "Types: ";
    for (auto const & t : types)
    {
        out << '\'' << t.first << "' ";
    }
    out << std::endl;
}
} // anonymous namespace
