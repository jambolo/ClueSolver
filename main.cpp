#include "Solver.h"

#include <json.hpp>

#include <fstream>
#include <iomanip>

using json = nlohmann::json;

namespace
{
static void loadConfiguration(char const * name);

std::vector<Solver::Name> players  = { "a", "b", "c", "d" };
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
    std::ifstream infilestream;
    std::istream * in = &std::cin;

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

    // Load player list
    Solver::Name input;
    std::getline(*in, input);
    players = json::parse(input);

    std::cout << "players = " << json(players).dump() << std::endl;
    std::cout << "suspects = " << json(suspects).dump() << std::endl;
    std::cout << "weapons = " << json(weapons).dump() << std::endl;
    std::cout << "rooms = " << json(rooms).dump() << std::endl;

    int index = 0;
    Solver solver(players, suspects, weapons, rooms);
    while (true)
    {
        std::getline(*in, input);
        if (in->eof())
            break;

        json turn = json::parse(input);

        if (turn.find("show") != turn.end())
        {
            std::cout << "---- " << input << std::endl;
            auto r = turn["show"];
            solver.show(r["player"], r["card"]);
        }
        else if (turn.find("suggest") != turn.end())
        {
            std::cout << '(' << std::setw(2) << index++ << ") " << input << std::endl;
            auto s = turn["suggest"];
            solver.suggest(s["player"], s["cards"], s["holders"]);
        }
        else if (turn.find("accuse") != turn.end())
        {
            std::cout << "!!!! " << input << std::endl;
            auto a = turn["accuse"];
            solver.accuse(a["suspect"], a["weapon"], a["room"]);
        }
        else if (turn.find("hand") != turn.end())
        {
            std::cout << "**** " << input << std::endl;
            auto a = turn["hand"];
            solver.hand(a["player"], a["cards"]);
        }
        else
        {
            assert(false);
        }

        std::cout << "state = " << solver.toJson().dump() << std::endl;
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
    suspects = j["suspects"];
    weapons  = j["weapons"];
    rooms    = j["rooms"];
}
} // anonymous namespace
