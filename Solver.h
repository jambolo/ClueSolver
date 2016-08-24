#pragma once
#if !defined(SOLVER_H)
#define SOLVER_H 1

#include <string>
#include <vector>

class Solver
{
public:
    Solver(std::vector<std::string> const & players,
           std::vector<std::string> const & rooms,
           std::vector<std::string> const & characters,
           std::vector<std::string> const & weapons);
    virtual ~Solver();

    void playerHas(std::string const & player, std::string const & card);
    void suggest(std::string const & character, std::string const & weapon, std::string const & room, std::vector<std::string> const & players);
    std::vector<std::string> mightBeHeldBy(std::string const & player) const;
    std::vector<std::string> mightHold(std::string const & card) const;
    
private:
    PlayerList players_;
    RoomList rooms_;
    CharacterList characters_;
    WeaponList weapons_;
};

#endif // !defined(SOLVER_H)
