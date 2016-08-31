#include "Solver.h"

#include <json.hpp>

#include <fstream>
#include <iomanip>

using json = nlohmann::json;

namespace
{
static void loadConfiguration(char const * name);

std::string rules = "classic";
std::vector<Solver::Name> players;
std::vector<Solver::Name> suspects =
{
    "Colonel Mustard",
    "Mrs. White",
    "Professor Plum",
    "Mrs. Peacock",
    "Mr. Green",
    "Miss Scarlet"
};
std::vector<Solver::Name> weapons =
{
    "Revolver",
    "Knife",
    "Rope",
    "Lead pipe",
    "Wrench",
    "Candlestick"
};
std::vector<Solver::Name> rooms =
{
    "Dining room",
    "Conservatory",
    "Kitchen",
    "Study",
    "Library",
    "Billiard room",
    "Lounge",
    "Ballroom",
    "Hall"
};
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
        loadConfiguration(configurationFileName);

    if (inputFileName)
    {
        infilestream.open(inputFileName);
        if (infilestream.is_open())
            in = &infilestream;
    }

    if (outputFileName)
    {
        outfilestream.open(outputFileName);
        if (outfilestream.is_open())
            out = &outfilestream;
    }

    *out << "rules = " << rules << std::endl;
    *out << "suspects = " << json(suspects).dump() << std::endl;
    *out << "weapons = " << json(weapons).dump() << std::endl;
    *out << "rooms = " << json(rooms).dump() << std::endl;
    *out << std::endl;

    // Load player list
    Solver::Name input;
    std::getline(*in, input);
    players = json::parse(input);

    *out << "players = " << json(players).dump() << std::endl;
    *out << std::endl;

    int suggestionId = 0;
    Solver solver(rules, players, suspects, weapons, rooms);
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
        else if (turn.find("accuse") != turn.end())
        {
            *out << "!!!! " << input << std::endl;
            auto a = turn["accuse"];
            solver.accuse(a["suspect"], a["weapon"], a["room"]);
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
        *out << "ANSWER: " << json(solver.mightBeHeldBy(Solver::ANSWER_PLAYER_NAME)).dump() << std::endl;
        *out << std::endl;
    }
    return 0;
}

namespace
{
void loadConfiguration(char const * name)
{
    std::ifstream file(name);
    if (!file.is_open())
        return;

    json j;
    file >> j;
    if (j.find("rules") != j.end())
        rules = j["rules"];
    if (j.find("suspects") != j.end())
        suspects = j["suspects"];
    if (j.find("weapons") != j.end())
        weapons = j["weapons"];
    if (j.find("rooms") != j.end())
        rooms = j["rooms"];
}
} // anonymous namespace
