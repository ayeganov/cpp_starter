#include <array>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

#include <docopt/docopt.h>
#include <spdlog/spdlog.h>


static constexpr std::size_t SIZE = 9;

template <std::size_t Size>
using grid = std::array<char, Size * Size>;


template <template <typename, std::size_t> class Grid, typename T, std::size_t Size>
inline char get_cell(std::size_t x, std::size_t y, Grid<T, Size>& grid)
{
  if(x >= Size or y >= Size)
  {
    auto msg = fmt::format("Coordinates x and y must not exceed {} in size", SIZE);
    throw std::invalid_argument(msg);
  }

  return grid[x * SIZE + y];
}


template <template <typename, std::size_t> class Grid, typename T, std::size_t Size>
inline void set_cell(std::size_t x, std::size_t y, Grid<T, Size>& grid, T value)
{
  if(x >= Size or y >= Size)
  {
    auto msg = fmt::format("Coordinates x and y must not exceed {} in size", SIZE);
    throw std::invalid_argument(msg);
  }

  grid[x * SIZE + y] = value;
}


class SudokuSolver
{
private: /** ============================= MEMBER VARS ============================= **/
  grid<SIZE> grid_;
  grid<SIZE> constraints_;

private: /** ============================= MEMBER METHODS ============================= **/
  void update_constraints()
  {
    
  }

public: /** ============================= MEMBER METHODS ============================= **/
  SudokuSolver(std::ifstream& file)
  {
    std::size_t x = 0, y = 0;

    if(file.is_open())
    {
      char c;
      file >> c;
      set_value(x, y++, c);
      x = y % 9;
    }

    update_constraints();
  }

  inline char get_value(std::size_t x, std::size_t y)
  {
    return get_cell(x, y, grid_);
  }

  inline char get_constraint(std::size_t x, std::size_t y)
  {
    return get_cell(x, y, constraints_);
  }

  inline void set_value(std::size_t x, std::size_t y, char value)
  {
    set_cell(x, y, grid_, value);
  }

  inline void set_constraint(std::size_t x, std::size_t y, char value)
  {
    set_cell(x, y, constraints_, value);
  }

};


//bool is_solved(const grid& sudoku)
//{
//  for(std::size_t y = 1; y < sudoku.size(); y += 3)
//  {
//    for(std::size_t x = 1; x < sudoku.size(); x += 3)
//    {
//      quadrant.clear();
//    }
//  }
//
//  return sudoku[0][0];
//}
//
//bool solve(grid& sudoku)
//{
//  sudoku[0][0] = 'c';
//  return false;
//}


static constexpr char USAGE[] =
R"(solve

    Usage:
      solve input <file>    Input file contain the puzzle.
    Options:
      -h --help     Show this screen.
)";


int main(int argc, const char **argv)
{
  std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
    { std::next(argv), std::next(argv, argc) },
    true, // show help if requested
    "solve"); // version string

  for(auto const &arg : args)
  {
    std::cout << arg.first << arg.second << std::endl;
  }

  //Use the default logger (stdout, multi-threaded, colored)
  spdlog::info("Hello, {}!", "World");

  fmt::print("Hello, from {}\n", "{fmt}");

  return 0;
}
