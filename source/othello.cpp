#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>

const int infinity = 2147483647;
const int minimax_base_depth = 2;
const int minimax_max_depth = 12;
const double max_wait_time = 0.3;

namespace Directions {
    enum {TOP, RIGHT_TOP, RIGHT, RIGHT_BOTTOM, BOTTOM, LEFT_BOTTOM, LEFT, LEFT_TOP, LAST};

    void increment(char& direction);
    void moveInDirection(char direction, int& x, int& y);

    char begin();
    char end();
}

struct Desk {
    enum {ST_EMPTY, ST_SECOND_PLAYER, ST_FIRST_PLAYER};

    bool canMakeMove(bool is_first_player) const;
    bool canMakeMove(int x, int y, bool is_first_player) const;
    char getState(int x, int y) const;
    void setCell(int x, int y, char state);
    bool isOnDesk(int x, int y) const;
    int getWidth() const;
    int getHeight() const;
    void changeCells(std::vector<std::pair<int, int>* >* cells_to_change);
    bool makeMove(int x, int y, bool is_first_player, std::vector<std::pair<int, int>* >* changed_cells = NULL);
    void free();
    void newDesk();
    void newDesk(int width, int height);
    void copyDesk(Desk* other_desk);
    int numberOfTokens(bool first_player);
    ~Desk() {free(); }
  private:
    std::vector<std::vector<char>* > desk_states;

    bool isReversible(int x, int y, char direction, bool is_first_player) const;
    void reverse(int x, int y, char direction, bool is_first_player, std::vector<std::pair<int, int>* >* changed_cells = NULL);
};

namespace ConsoleInterface {
    void clear();
    void showDesk(Desk* desk);
    void input(int& x, int& y);
}

namespace OthelloGame {
    namespace AI {
        struct Move {
            std::vector<std::pair<int, int>* > reversed_cells;
            int move_coordinate_x;
            int move_coordiante_y;

            ~Move();
        };

        int minimax_depth;
        Desk ai_calculation_desk;
        std::vector<Move*> virtual_moves_history;

        void deleteLastVirtualMoveFromHistory();
        void makeVirtualMove(int x, int y, bool first_player_turn);
        void undoLastVirtualMove();
        int evaluatePosition();
        int miniMax(int& chosen_x, int& chosen_y, int current_depth, bool first_player_turn, int alpha, int beta);
        void chooseMove(int& chosen_move_x, int& chosen_move_y, Desk* desk, bool first_player_turn);
    }

    Desk desk;
    bool is_first_player_human;
    bool is_second_player_human;
    bool first_player_turn;

    void initialize(bool is_first_player_human, bool is_second_player_human);
    void makeMove();
    bool isGameFinished();
    bool mostTokensFirstPlayer();
    bool mostTokensSecondPlayer();
}

char Directions::begin() {
    return TOP;
}

char Directions::end() {
    return LAST;
}

void Directions::increment(char& direction) {
    if (direction < LAST) {
        ++direction;
    } else {
        direction = TOP;
    }
}

void Directions::moveInDirection(char direction, int& x, int& y) {
    switch (direction) {
        case TOP:
            --y;
            return;
        case RIGHT_TOP:
            --y;
            ++x;
            return;
        case RIGHT:
            ++x;
            return;
        case RIGHT_BOTTOM:
            ++x;
            ++y;
            return;
        case BOTTOM:
            ++y;
            return;
        case LEFT_BOTTOM:
            --x;
            ++y;
            return;
        case LEFT:
            --x;
            return;
        case LEFT_TOP:
            --x;
            --y;
            return;
    }
}

void Desk::copyDesk(Desk* other_desk) {
    newDesk(other_desk->getWidth(), other_desk->getHeight());
    for (int i = 0; i < getWidth(); ++i) {
        for (int j = 0; j < getHeight(); ++j) {
            setCell(i, j, other_desk->getState(i, j));
        }
    }
}

int Desk::numberOfTokens(bool first_player) {
    int result = 0;
    for (int i = 0; i < getWidth(); ++i) {
        for (int j = 0; j < getHeight(); ++j) {
            if (first_player && getState(i, j) == ST_FIRST_PLAYER ||
                !first_player && getState(i, j) == ST_SECOND_PLAYER) {
                ++result;
            }
        }
    }
    return result;
}

void Desk::setCell(int x, int y, char state) {
    (*desk_states[x])[y] = state;
}

bool Desk::isReversible(int x, int y, char direction, bool is_first_player) const {
    Directions::moveInDirection(direction, x, y);
    if (!isOnDesk(x, y) || (is_first_player ? (getState(x, y) != ST_SECOND_PLAYER) : (getState(x, y) != ST_FIRST_PLAYER))) {
        return false;
    }
    while (isOnDesk(x, y) && (is_first_player ? (getState(x, y) == ST_SECOND_PLAYER) : (getState(x, y) == ST_FIRST_PLAYER))) {
        Directions::moveInDirection(direction, x, y);
    }
    return isOnDesk(x, y) && (is_first_player ? (getState(x, y) == ST_FIRST_PLAYER) : (getState(x, y) == ST_SECOND_PLAYER));
}

void Desk::reverse(int x, int y, char direction, bool is_first_player, std::vector<std::pair<int, int>* >* changed_cells) {
    Directions::moveInDirection(direction, x, y);
    while (isOnDesk(x, y) && (is_first_player ? (getState(x, y) == ST_SECOND_PLAYER) : (getState(x, y) == ST_FIRST_PLAYER))) {
        setCell(x, y, is_first_player ? ST_FIRST_PLAYER : ST_SECOND_PLAYER);
        if (changed_cells != NULL) {
            changed_cells->push_back(new std::pair<int, int>(x, y));
        }
        Directions::moveInDirection(direction, x, y);
    }
}

void Desk::changeCells(std::vector<std::pair<int, int>* >* cells_to_change) {
    for (auto cell: *cells_to_change) {
        setCell(cell->first,
                cell->second,
                (getState(cell->first, cell->second) == ST_FIRST_PLAYER) ? ST_SECOND_PLAYER : ST_FIRST_PLAYER);
    }
}

bool Desk::canMakeMove(int x, int y, bool is_first_player) const {
    if (!isOnDesk(x, y) || getState(x, y) != ST_EMPTY) {
        return false;
    }
    for (char direction = Directions::begin(); direction < Directions::end(); Directions::increment(direction)) {
        if (isReversible(x, y, direction, is_first_player)) {
            return true;
        }
    }
    return false;
}

int Desk::getWidth() const {
    return desk_states.size();
}

int Desk::getHeight() const {
    if (desk_states.size() == 0) {
        return 0;
    }
    return desk_states[0]->size();
}

bool Desk::canMakeMove(bool is_first_player) const {
    for (int i = 0; i < getWidth(); ++i) {
        for (int j = 0; j < getHeight(); ++j) {
            if (canMakeMove(i, j, is_first_player)) {
                return true;
            }
        }
    }
    return false;
}

char Desk::getState(int x, int y) const {
    return (*desk_states[x])[y];
}

bool Desk::isOnDesk(int x, int y) const {
    return 0 <= x && x < getWidth() && 0 <= y && y < getHeight();
}

bool Desk::makeMove(int x, int y, bool is_first_player, std::vector<std::pair<int, int>* >* changed_cells) {
    if (!canMakeMove(x, y, is_first_player)) {
        return false;
    }
    setCell(x, y, is_first_player ? ST_FIRST_PLAYER : ST_SECOND_PLAYER);
    for (char direction = Directions::begin(); direction < Directions::end(); Directions::increment(direction)) {
        if (isReversible(x, y, direction, is_first_player)) {
            reverse(x, y, direction, is_first_player, changed_cells);
        }
    }
    return true;
}

void Desk::free() {
    for (int i = 0; i < desk_states.size(); ++i) {
        delete desk_states[i];
    }
    desk_states.resize(0);
}

void Desk::newDesk() {
    newDesk(8, 8);
}

void Desk::newDesk(int width, int height) {
    free();
    for (int i = 0; i < width; ++i) {
        desk_states.push_back(new std::vector<char>());
        for (int j = 0; j < height; ++j) {
            desk_states[i]->push_back(ST_EMPTY);
        }
    }
    (*desk_states[(width - 1) / 2])[(height - 1) / 2] = ST_SECOND_PLAYER;
    (*desk_states[(width - 1) / 2 + 1])[(height - 1) / 2 + 1] = ST_SECOND_PLAYER;
    (*desk_states[(width - 1) / 2])[(height - 1) / 2 + 1] = ST_FIRST_PLAYER;
    (*desk_states[(width - 1) / 2 + 1])[(height - 1) / 2] = ST_FIRST_PLAYER;
}

bool OthelloGame::mostTokensFirstPlayer() {
    return desk.numberOfTokens(true) > desk.numberOfTokens(false);
}

bool OthelloGame::mostTokensSecondPlayer() {
    return desk.numberOfTokens(true) < desk.numberOfTokens(false);
}

void OthelloGame::initialize(bool is_first_player_human, bool is_second_player_human) {
    first_player_turn = true;
    OthelloGame::is_first_player_human = is_first_player_human;
    OthelloGame::is_second_player_human = is_second_player_human;
    desk.newDesk();
    ConsoleInterface::clear();
    ConsoleInterface::showDesk(&desk);
}

bool OthelloGame::isGameFinished() {
    return !(desk.canMakeMove(true) || desk.canMakeMove(false));
}

void OthelloGame::makeMove() {
    int x, y;
    if (desk.canMakeMove(first_player_turn)) {
        if (is_first_player_human && first_player_turn || is_second_player_human && !first_player_turn) {
            ConsoleInterface::input(x, y);
            while (!desk.makeMove(x, y, first_player_turn)) {
                ConsoleInterface::clear();
                ConsoleInterface::showDesk(&desk);
                std::cout << "Illegal move." << std::endl;
                ConsoleInterface::input(x, y);
            }
        } else {
            AI::chooseMove(x, y, &desk, first_player_turn);
            desk.makeMove(x, y, first_player_turn);
        }
    }
    ConsoleInterface::clear();
    ConsoleInterface::showDesk(&desk);
    first_player_turn = !first_player_turn;
}

int OthelloGame::AI::evaluatePosition() {
    int result = 0;
    for (int i = 0; i < ai_calculation_desk.getWidth(); ++i) {
        for (int j = 0; j < ai_calculation_desk.getHeight(); ++j) {
            if (ai_calculation_desk.getState(i, j) == Desk::ST_FIRST_PLAYER) {
                if ((i == 0 || i == ai_calculation_desk.getWidth() - 1) &&
                    (j == 0 || j == ai_calculation_desk.getHeight() - 1)) {
                    result += 10;
                }
                ++result;
            }
            if (ai_calculation_desk.getState(i, j) == Desk::ST_SECOND_PLAYER) {
                if ((i == 0 || i == ai_calculation_desk.getWidth() - 1) &&
                    (j == 0 || j == ai_calculation_desk.getHeight() - 1)) {
                    result -= 10;
                }
                --result;
            }
        }
    }
    return result;
}

OthelloGame::AI::Move::~Move() {
    for (int i = 0; i < reversed_cells.size(); ++i) {
        delete reversed_cells[i];
    }
}

void OthelloGame::AI::deleteLastVirtualMoveFromHistory() {
    delete virtual_moves_history[virtual_moves_history.size() - 1];
    virtual_moves_history.resize(virtual_moves_history.size() - 1);
}

void OthelloGame::AI::undoLastVirtualMove() {
    ai_calculation_desk.changeCells(&virtual_moves_history[virtual_moves_history.size() - 1]->reversed_cells);
    ai_calculation_desk.setCell(virtual_moves_history[virtual_moves_history.size() - 1]->move_coordinate_x,
                                 virtual_moves_history[virtual_moves_history.size() - 1]->move_coordiante_y,
                                 Desk::ST_EMPTY);
    deleteLastVirtualMoveFromHistory();
}

void OthelloGame::AI::makeVirtualMove(int x, int y, bool first_player_turn) {
    virtual_moves_history.push_back(new Move());
    virtual_moves_history[virtual_moves_history.size() - 1]->move_coordinate_x = x;
    virtual_moves_history[virtual_moves_history.size() - 1]->move_coordiante_y = y;
    ai_calculation_desk.makeMove(x, y, first_player_turn, &virtual_moves_history[virtual_moves_history.size() - 1]->reversed_cells);
}

int OthelloGame::AI::miniMax(int& chosen_x, int& chosen_y, int current_depth, bool first_player_turn, int alpha, int beta) {
    int current_extreme_value = (first_player_turn ? -infinity : infinity);
    int current_extreme_x = -1;
    int current_extreme_y = -1;
    int current_turn_evaluation;
    bool alpha_beta_interruption = false;
    for (int i = 0; i < ai_calculation_desk.getWidth() && !alpha_beta_interruption; ++i) {
        for (int j = 0; j < ai_calculation_desk.getHeight() && !alpha_beta_interruption; ++j) {
            if (ai_calculation_desk.canMakeMove(i, j, first_player_turn)) {
                makeVirtualMove(i, j, first_player_turn);
                if (current_depth > 0) {
                    current_turn_evaluation = miniMax(chosen_x, chosen_y, current_depth - 1, !first_player_turn, alpha, beta);
                } else {
                    current_turn_evaluation = evaluatePosition();
                }
                if (current_extreme_value < current_turn_evaluation && first_player_turn ||
                    current_extreme_value > current_turn_evaluation && !first_player_turn) {
                    current_extreme_x = i;
                    current_extreme_y = j;
                    current_extreme_value = current_turn_evaluation;
                    if (first_player_turn) {
                        alpha = std::max(current_extreme_value, alpha);
                    } else {
                        beta = std::min(current_extreme_value, beta);
                    }
                    if (alpha > beta) {
                        alpha_beta_interruption = true;
                    }
                }
                undoLastVirtualMove();
            }
        }
    }
    if (current_extreme_x == -1) {
        if (current_depth > 0) {
            current_extreme_value = miniMax(chosen_x, chosen_y, current_depth - 1, !first_player_turn, alpha, beta);
        } else {
            current_extreme_value = evaluatePosition();
        }
    }
    if (current_depth == minimax_depth) {
        chosen_x = current_extreme_x;
        chosen_y = current_extreme_y;
    }
    return current_extreme_value;
}

void OthelloGame::AI::chooseMove(int& chosen_move_x, int& chosen_move_y, Desk* desk, bool first_player_turn) {
    minimax_depth = minimax_base_depth;
    clock_t execution_time = clock();
    ai_calculation_desk.copyDesk(desk);
    miniMax(chosen_move_x, chosen_move_y, minimax_depth, first_player_turn, -infinity, infinity);
    while ((clock() - execution_time + 0.0) / CLOCKS_PER_SEC < max_wait_time && minimax_depth < minimax_max_depth) {
        ++minimax_depth;
        miniMax(chosen_move_x, chosen_move_y, minimax_depth, first_player_turn, -infinity, infinity);
    }
}

void ConsoleInterface::clear() {
    system("clear");
}

void ConsoleInterface::showDesk(Desk* desk) {
    std::cout << "Tokens: " << desk->numberOfTokens(true) << " : ";
    std::cout << desk->numberOfTokens(false) << std::endl;
    std::string line = " ";
    for (int i = 0; i < desk->getWidth(); ++i) {
        line += 'A' + i;
    }
    std::cout << line << std::endl;
    for (int i = 0; i < desk->getHeight(); ++i) {
        line = "";
        line += '1' + i;
        for (int j = 0; j < desk->getWidth(); ++j) {
            switch (desk->getState(j, i)) {
                case Desk::ST_FIRST_PLAYER:
                    line += "O";
                    break;
                case Desk::ST_SECOND_PLAYER:
                    line += "X";
                    break;
                case Desk::ST_EMPTY:
                    line += " ";
                    break;
            }
        }
        std::cout << line << std::endl;
    }
}

void ConsoleInterface::input(int& x, int& y) {
    std::string input_line;
    std::cout << std::endl << "Make your move (e.g. \"a1\")" << std::endl;
    std::cin >> input_line;
    while (input_line.size() != 2 ||
           input_line[0] < 'a' || input_line[1] > 'h' ||
           input_line[0] < '1' || input_line[1] > '8') {
        std::cin >> input_line;
    }
    x = input_line[0] - 'a';
    y = input_line[1] - '1';
}

int main(int argc, char** argv) {
    OthelloGame::initialize(true, false);
    while (!OthelloGame::isGameFinished()) {
        OthelloGame::makeMove();
    }
    if (OthelloGame::mostTokensFirstPlayer()) {
        std::cout << "First player won." << std::endl;
    } else if (OthelloGame::mostTokensSecondPlayer()) {
        std::cout << "Second player won." << std::endl;
    } else {
        std::cout << "Tie." << std::endl;
    }
    return 0;
}
