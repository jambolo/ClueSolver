#include "Solver.h"

#include <json.hpp>

#include <fstream>
#include <iomanip>

using json = nlohmann::json;

namespace
{

bool loadConfiguration(char const * name);
void listTypes(std::ostream & out, Solver::TypeInfoList const & types);
void listCards(std::ostream & out, Solver::TypeInfo const & type, Solver::CardInfoList const & cards);

Solver::TypeInfoList s_types =
{
    { "suspect", "suspect", "Suspects", "" },
    { "weapon",  "weapon",  "Weapons",  "with the" },
    { "room",    "room",    "Rooms",    "in the"   }
};

Solver::CardInfoList s_cards =
{
    { "mustard",      { "Colonel Mustard", "suspect" } },
    { "white",        { "Mrs. White",      "suspect" } },
    { "plum",         { "Professor Plum",  "suspect" } },
    { "peacock",      { "Mrs. Peacock",    "suspect" } },
    { "green",        { "Mr. Green",       "suspect" } },
    { "scarlet",      { "Miss Scarlet"     "suspect" } },
    { "revolver",     { "Revolver",        "weapon"  } },
    { "knife",        { "Knife",           "weapon"  } },
    { "rope",         { "Rope",            "weapon"  } },
    { "pipe",         { "Lead pipe",       "weapon"  } },
    { "wrench",       { "Wrench",          "weapon"  } },
    { "candlestick",  { "Candlestick"      "weapon"  } },
    { "dining",       { "Dining room",     "room"    } },
    { "conservatory", { "Conservatory",    "room"    } },
    { "kitchen",      { "Kitchen",         "room"    } },
    { "study",        { "Study",           "room"    } },
    { "library",      { "Library",         "room"    } },
    { "billiard",     { "Billiard room",   "room"    } },
    { "lounge",       { "Lounge",          "room"    } },
    { "ballroom",     { "Ballroom",        "room"    } },
    { "hall",         { "Hall"             "room"    } }
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
    std::istream * in = &std::cin;
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
        listCards(*out, t, s_cards);
    }

    *out << std::endl;

    // Load player list
    Solver::Id input;
    std::getline(*in, input);
    s_players = json::parse(input);

    *out << "players = " << json(s_players).dump() << std::endl;
    *out << std::endl;

    int suggestionId = 0;
    Solver::Rules rules = { s_rules, s_types, s_cards };
    Solver solver(rules, s_players);
    while (true)
    {
        std::getline(*in, input);
        if (in->eof())
            break;

        json turn = json::parse(input);

        if (turn.find("show") != turn.end())
        {
            *out << "---- " << input << std::endl;
            auto r = turn["show"];
            solver.show(r["player"], r["card"]);
        }
        else if (turn.find("suggest") != turn.end())
        {
            *out << '(' << std::setw(2) << suggestionId << ") " << input << std::endl;
            auto s = turn["suggest"];
            solver.suggest(s["player"], s["cards"], s["showed"], suggestionId);
            ++suggestionId;
        }
        else if (turn.find("hand") != turn.end())
        {
            *out << "**** " << input << std::endl;
            auto a = turn["hand"];
            solver.hand(a["player"], a["cards"]);
        }
        else
        {
            assert(false);
        }

        {
            std::vector<std::string> discoveries = solver.discoveries();
            for (auto const & d : discoveries)
            {
                *out << d << std::endl;
            }
        }

//        *out << "state = " << solver.toJson().dump() << std::endl;
        *out << "ANSWER: " << json(solver.mightBeHeldBy(Solver::ANSWER_PLAYER_ID)).dump() << std::endl;
        *out << std::endl;
    }
    return 0;
}

namespace
{

//    {
//        "types" : [
//            { "id" : "suspect", "name" : "Suspect", "preposition" : ""         },
//            { "id" : "weapon",  "name" : "Weapon",  "preposition" : "with the" },
//            { "id" : "room",    "name" : "Room",    "preposition" : "in the"   }
//        ]
//    }
//    {
//        "cards" : [
//            { "id" : "mustard", "name" : "Colonel Mustard", "type" : "suspect" },
//            { "id" : "knife",   "name" : "Knife",           "type" : "weapon"  },
//            { "id" : "studio",  "name" : "Studio",          "type" : "room"    }
//        ]
//    }

bool loadConfiguration(char const * name)
{
    std::ifstream file(name);
    if (!file.is_open())
        return false;

    try
    {
        json j;
        file >> j;

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
}

void listCards(std::ostream & out, Solver::TypeInfo const & type, Solver::CardInfoList const & cards)
{
    out << type.title << ": ";
    for (auto const & c : cards)
    {
        if (c.second.type == type.id)
        {
            out << '\'' << c.second.name << "' ";
        }
    }
    out << std::endl;
}

void listTypes(std::ostream & out, Solver::TypeInfoList const & types)
{
    out << "Types: ";
    for (auto const & t : types)
    {
        out << '\'' << t.name << "' ";
    }
    out << std::endl;
}

} // anonymous namespace
