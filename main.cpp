#include "Solver.h"

#include "json.hpp"

#include <cstdio>
#include <fstream>

using json = nlohmann::json;

namespace
{

static void loadConfiguration(char const * name);

std::vector<std::string> players;
std::vector<std::string> suspects;
std::vector<std::string> weapons;
std::vector<std::string> rooms;

} // anonymous namespace

int main(int argc, char ** argv)
{

    // Load configuration
    loadConfiguration(*++argv);

    // Load player list
    std::string input;
    std::getline(std::cin, input);
    players = json::parse(input);

    std::cout << "players = " << json(players).dump() << std::endl;
    std::cout << "suspects = " << json(suspects).dump() << std::endl;
    std::cout << "weapons = " << json(weapons).dump() << std::endl;
    std::cout << "rooms = " << json(rooms).dump() << std::endl;

    Solver solver(players, suspects, weapons, rooms);

    Solver::NameList answer = solver.mightBeHeldBy(Solver::ANSWER_PLAYER_NAME);
    while (answer.size() > 3)
    {
        std::getline(std::cin, input);
        json turn = json::parse(input);
        if (turn.find("reveal") != turn.end())
        {
            auto r = turn["reveal"];
            solver.reveal(r["player"], r["card"]);
        }
        else if (turn.find("suggest") != turn.end())
        {
            auto s = turn["suggest"];
            solver.suggest(s["suspect"], s["weapon"], s["room"], s["players"]);
        }
        else if (turn.find("accuse") != turn.end())
        {
            auto a = turn["accuse"];
            solver.accuse(a["suspect"], a["weapon"], a["room"]);
        }

        answer = solver.mightBeHeldBy(Solver::ANSWER_PLAYER_NAME);

        for (auto const & p : players)
        {
            std::cout << json(solver.mightBeHeldBy(p)).dump() << std::endl;
        }
        std::cout << "Answer = " << json(answer).dump() << std::endl << std::endl;
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
