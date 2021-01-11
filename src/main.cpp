#include <algorithm>
#include <array>
#include <fstream>
#include <numeric>
#include <queue>
#include <stack>
#include <vector>

#include <fmt/format.h>
#include <spdlog/spdlog.h>


static constexpr char EMPTY_CELL = '0';

struct Cell
{
  size_t x;
  size_t y;
};



static constexpr std::size_t SIZE = 9;
static constexpr std::size_t NUM_CELLS = 81;

static const std::array<Cell, SIZE> quadrants = {{ {1, 1}, {1, 4}, {1, 7},
                                                   {4, 1}, {4, 4}, {4, 7},
                                                   {7, 1}, {7, 4}, {7, 7} }};


template<typename T>
struct type_is { using type = T; };


template <typename T, std::size_t Size, std::size_t Depth>
struct NestedArray
{
  static_assert(Depth <= 10ul);
  using type = typename NestedArray<std::array<T, Size>, Size, Depth-1>::type;
};


template <typename T, std::size_t Size>
struct NestedArray<T, Size, 0> : type_is<T>
{};


template <typename T, std::size_t Size, std::size_t Depth>
using NestedArray_t = typename NestedArray<T, Size, Depth>::type;


template <typename T, std::size_t Size>
using grid = std::array<T, Size * Size>;


template <template <typename, std::size_t> class Grid, typename T, std::size_t Size>
inline T& get_cell(std::size_t x, std::size_t y, Grid<T, Size>& grid)
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


/**
 * Converts the character digit value to the appropriate index.
 * **STRONG** assumption - value will never be less than '1'.
 */
size_t digit_to_idx(char value)
{
  return static_cast<size_t>(value - '1');
}


std::vector<char> available_digits(auto& constraints)
{
  std::vector<char> digits{SIZE};
  for(auto [digit, i] = std::tuple{'1', 0ul}; i < SIZE; ++i, ++digit)
  {
    if(constraints[i])
    {
      digits.push_back(digit);
    }
  }

  if(digits.size()) return digits;
  digits.push_back(EMPTY_CELL);

  return digits;
}


using ConstraintsCube = NestedArray_t<bool, SIZE, 3>;


enum class GameState
{
  SOLVED = 1,
  VIOLATION = 2,
  VALID = 3,
  NO_CHOICES_FOR_EMPTY_CELL = 4,
};


class SudokuSolver
{
private: /** ============================= MEMBER VARS ============================= **/
  grid<char, SIZE> grid_{};
  grid<size_t, SIZE> constraint_counts_{};
  ConstraintsCube constraints_{};

private: /** ============================= MEMBER METHODS ============================= **/
  void update_constraints()
  {
    for(size_t x = 0; x < SIZE; ++x)
    {
      for(size_t y = 0; y < SIZE; ++y)
      {
        update_constraint(x, y);
      }
    }
  }

  void update_constraint(size_t x, size_t y)
  {
    char& value = get_value(x, y);
    auto& cell_constraints = constraints_[x][y];

    bool filled_cell = value != EMPTY_CELL;
    if(filled_cell)
    {
      cell_constraints.fill(false);
      return;
    }

    for(size_t step = 0; step < SIZE; ++step)
    {
      if(step != y)
      {
        char row_value = get_value(x, step);
        filled_cell = row_value != EMPTY_CELL;
        if(filled_cell)
        {
          size_t idx = digit_to_idx(row_value);
          cell_constraints[idx] = false;
        }
      }

      if(step != x)
      {
        char col_value = get_value(step, y);
        filled_cell = col_value != EMPTY_CELL;
        if(filled_cell)
        {
          size_t idx = digit_to_idx(col_value);
          cell_constraints[idx] = false;
        }
      }
    }

    update_constraint_from_quadrant(x, y, cell_constraints);

    size_t& count = get_constraint_count(x, y);
    count = std::accumulate(cell_constraints.begin(), cell_constraints.end(), 0ul);
  }

  size_t find_closest_quadrant_idx(size_t x, size_t y)
  {
    size_t min_idx = 0, dist = 1000; // 1000 is as good as MAX_INT in this case
    for(size_t i = 0ul; i < quadrants.size(); ++i)
    {
      auto& q = quadrants[i];
      auto x_diff = std::max(x, q.x) - std::min(x, q.x);
      auto y_diff = std::max(y, q.y) - std::min(y, q.y);
      auto cur_dist = x_diff + y_diff;
      if(cur_dist < dist)
      {
        dist = cur_dist;
        min_idx = i;
      }
    }

    return min_idx;
  }

  void update_constraint_from_quadrant(size_t x, size_t y, std::array<bool, SIZE>& constraints)
  {
    auto min_idx = find_closest_quadrant_idx(x, y);
    Cell closest = quadrants[min_idx];

    for(auto i = closest.x - 1; i <= closest.x + 1; ++i)
    {
      for(auto j = closest.y - 1; j <= closest.y + 1; ++j)
      {
        auto value = get_value(i, j);
        if(value != EMPTY_CELL)
        {
          size_t idx = digit_to_idx(value);
          constraints[idx] = false;
        }
      }
    }
  }

  /**
   * Limit possible choices based on the row, column and square constraints
   * then apply back-tracking algorithm.
   */
  bool solve(size_t row, size_t start_col)
  {
    if(row >= SIZE)
    {
      return true;
    }

    bool solved = false;
    for(size_t col = start_col; col < SIZE; ++col)
    {
      if(solved) break;
      auto value = get_value(row, col);
      bool is_filled = value != EMPTY_CELL;

      // go to the next cell
      if(is_filled)
      {
        continue;
      }

      auto& constraints = constraints_[row][col];

      for(auto digit : available_digits(constraints))
      {
        if(digit == EMPTY_CELL)
        {
          set_value(row, col, EMPTY_CELL);
          break;
        }
        set_value(row, col, digit);

        auto state = get_game_state();

        if(state == GameState::SOLVED)
        {
          return true;
        }
        else if(state != GameState::VALID)
        {
          set_value(row, col, EMPTY_CELL);
          continue;
        }

        solved = solve(row, col+1);
        if(not solved)
        {
          set_value(row, col, EMPTY_CELL);
        }
        else
        {
          break;
        }
      }

      // tried every valid digit and still no solution? dead path
      if(not solved)
      {
        return false;
      }
    }

    return solve(row + 1, 0);
  }


public: /** ============================= MEMBER METHODS ============================= **/
  bool solve()
  {
    return solve(0, 0);
  }

  SudokuSolver(std::string_view puzzle)
  {
    for(auto& row : constraints_)
    {
      for(auto& col : row)
      {
        // assume all digits are available at first
        col.fill(true);
      }
    }

    std::size_t x = 0, y = 0;

    size_t num_cells = 0;
    for(auto c : puzzle)
    {
      set_value(x, y++, c);

      if(y == SIZE)
      {
        y = 0;
        ++x;
      }
      ++num_cells;

      if(num_cells > NUM_CELLS)
      {
        throw std::range_error("Too many digits in the puzzle");
      }
    }

    update_constraints();
  }

  GameState get_game_state()
  {
    size_t num_filled_cells = 0;
    for(size_t row_col = 0; row_col < SIZE; ++row_col)
    {
      std::array<bool, SIZE> row_set{};
      std::array<bool, SIZE> col_set{};

      for(size_t step = 0; step < SIZE; ++step)
      {
        size_t row = row_col;
        size_t col = row_col;

        char row_value = get_value(row, step);
        char col_value = get_value(step, col);

        bool filled_cell = row_value != EMPTY_CELL;
        if(filled_cell)
        {
          // count filled cells by rows
          ++num_filled_cells;
          size_t row_idx = digit_to_idx(row_value);
          bool is_duplicate = row_set[row_idx];
          if(is_duplicate)
          {
            return GameState::VIOLATION;
          }
          row_set[row_idx] = true;
        }
        else
        {
          auto constraint_count = get_constraint_count(row, step);
          if(constraint_count == 0)
          {
            return GameState::NO_CHOICES_FOR_EMPTY_CELL;
          }
        }

        filled_cell = col_value != EMPTY_CELL;
        if(filled_cell)
        {
          size_t col_idx = digit_to_idx(col_value);
          bool is_duplicate = col_set[col_idx];
          if(is_duplicate)
          {
            return GameState::VIOLATION;
          }
          col_set[col_idx] = true;
        }
      }
    }

    for(auto& q : quadrants)
    {
      std::array<bool, SIZE> quadrant_set{};
      for(auto i = q.x - 1; i <= q.x + 1; ++i)
      {
        for(auto j = q.y - 1; j <= q.y + 1; ++j)
        {
          auto value = get_value(i, j);
          if(value != EMPTY_CELL)
          {
            size_t idx = digit_to_idx(value);
            bool is_duplicate = quadrant_set[idx];
            if(is_duplicate)
            {
              return GameState::VIOLATION;
            }
            quadrant_set[idx] = true;
          }
        }
      }
    }

    if(num_filled_cells == NUM_CELLS) return GameState::SOLVED;
    return GameState::VALID;
  }

  inline char& get_value(std::size_t x, std::size_t y)
  {
    return get_cell(x, y, grid_);
  }

  inline size_t& get_constraint_count(std::size_t x, std::size_t y)
  {
    return get_cell(x, y, constraint_counts_);
  }

  inline void set_value(std::size_t x, std::size_t y, char value)
  {
    set_cell(x, y, grid_, value);
  }

  void print_grid(size_t hx=1000, size_t hy=1000)
  {
    static const std::string GREEN = "\033[32m";
    static const std::string RESET = "\033[0m";

    for(size_t x = 0; x < SIZE; ++x)
    {
      for(size_t y = 0; y < SIZE; ++y)
      {
        if(x == hx and y == hy)
        {
          fmt::print(" {}{}{}", GREEN, get_value(x, y), RESET);
        }
        else
        {
          fmt::print(" {}", get_value(x, y));
        }
      }
      fmt::print("\n");
    }
  }


  void print_solution()
  {
    fmt::print("{}\n", fmt::join(grid_, ""));
  }

  void print_constraint_counts(size_t hx=1000, size_t hy=1000)
  {
    static const std::string GREEN = "\033[32m";
    static const std::string RESET = "\033[0m";

    for(size_t x = 0; x < SIZE; ++x)
    {
      for(size_t y = 0; y < SIZE; ++y)
      {
        if(x == hx and y == hy)
        {
          fmt::print(" {}{}{}", GREEN, get_constraint_count(x, y), RESET);
        }
        else
        {
          fmt::print(" {}", get_constraint_count(x, y));
        }
      }
      fmt::print("\n");
    }
  }
};


static constexpr char USAGE[] =
R"(solve

    Usage:
      solve <file>    Input file contain the puzzle.
    Options:
      -h --help     Show this screen.
)";


int main(int argc, const char **argv)
{
  std::string file_name{};
  if(argc < 2)
  {
    fmt::print(USAGE);
    return 1;
  }

  file_name = argv[1];
  std::ifstream input{file_name, std::ios_base::in | std::ios_base::binary};

  std::string line;
  while(std::getline(input, line))
  {
    auto solver = SudokuSolver{line};

    if(solver.solve())
    {
      solver.print_solution();
    }
    else
    {
      fmt::print("Failed to find solution\n");
    }
  }


  return 0;
}
