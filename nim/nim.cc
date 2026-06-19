#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <thread>
#include <string>
#include <limits>
#include <print> // C++23 standard printing

// Type aliases to make the code read cleanly like Python
using State = std::vector<int>;
using Action = std::pair<int, int>;

class Nim {
public:
    State piles;
    int player;
    std::optional<int> winner;

    // Initialize with default piles [1, 3, 5, 7]
    Nim(State initial = {1, 3, 5, 7})
        : piles(std::move(initial)), player(0), winner(std::nullopt) {}

    static std::set<Action> available_actions(const State& current_piles) {
        std::set<Action> actions;
        for (size_t i = 0; i < current_piles.size(); ++i) {
            for (int j = 1; j <= current_piles[i]; ++j) {
                actions.insert({static_cast<int>(i), j});
            }
        }
        return actions;
    }

    static int other_player(int p) {
        return p == 1 ? 0 : 1;
    }

    void switch_player() {
        player = other_player(player);
    }

    void move(Action action) {
        int pile = action.first;
        int count = action.second;

        // Check for errors
        if (winner.has_value()) {
            throw std::runtime_error("Game already won");
        } else if (pile < 0 || pile >= static_cast<int>(piles.size())) {
            throw std::invalid_argument("Invalid pile");
        } else if (count < 1 || count > piles[pile]) {
            throw std::invalid_argument("Invalid number of objects");
        }

        // Update pile
        piles[pile] -= count;
        switch_player();

        // Check for a winner (C++20/23 ranges)
        if (std::ranges::all_of(piles, [](int p) { return p == 0; })) {
            winner = player;
        }
    }
};

class NimAI {
public:
    // Q-learning dictionary maps: pair(state, action) -> Q-value
    std::map<std::pair<State, Action>, double> q;
    double alpha;
    double epsilon;
    std::mt19937 gen;

    NimAI(double alpha = 0.5, double epsilon = 0.1)
        : alpha(alpha), epsilon(epsilon) {
        std::random_device rd;
        gen = std::mt19937(rd());
    }

    void update(const State& old_state, Action action, const State& new_state, double reward) {
        double old_q = get_q_value(old_state, action);
        double best_future = best_future_reward(new_state);
        update_q_value(old_state, action, old_q, reward, best_future);
    }

    double get_q_value(const State& state, Action action) const {
        auto it = q.find({state, action});
        if (it != q.end()) {
            return it->second;
        }
        return 0.0;
    }

    void update_q_value(const State& state, Action action, double old_q, double reward, double future_rewards) {
        q[{state, action}] = old_q + alpha * (reward + future_rewards - old_q);
    }

    double best_future_reward(const State& state) const {
        auto actions = Nim::available_actions(state);
        if (actions.empty()) {
            return 0.0;
        }
        
        bool found = false;
        double best = 0.0;
        for (const auto& action : actions) {
            double val = get_q_value(state, action);
            if (!found || val > best) {
                best = val;
                found = true;
            }
        }
        return found ? best : 0.0;
    }

    Action choose_action(const State& state, bool use_epsilon = true) {
        auto actions_set = Nim::available_actions(state);
        std::vector<Action> actions(actions_set.begin(), actions_set.end());
        double best_reward = best_future_reward(state);

        std::uniform_real_distribution<> prob_dis(0.0, 1.0);
        
        // Epsilon-greedy random choice
        if (use_epsilon && prob_dis(gen) < epsilon) {
            std::uniform_int_distribution<> action_dis(0, actions.size() - 1);
            return actions[action_dis(gen)];
        }

        // Collect best actions
        std::vector<Action> best_actions;
        for (const auto& action : actions) {
            // Check for float equality cleanly
            if (std::abs(get_q_value(state, action) - best_reward) < 1e-9) {
                best_actions.push_back(action);
            }
        }

        // Return a random choice among ties
        std::uniform_int_distribution<> best_dis(0, best_actions.size() - 1);
        return best_actions[best_dis(gen)];
    }
};

NimAI train(int n) {
    NimAI player;

    for (int i = 0; i < n; ++i) {
        std::println("Playing training game {}", i + 1);
        Nim game;

        // Keep track of last move made by either player
        struct StateAction {
            std::optional<State> state;
            std::optional<Action> action;
        };
        std::map<int, StateAction> last;
        last[0] = {std::nullopt, std::nullopt};
        last[1] = {std::nullopt, std::nullopt};

        while (true) {
            State state = game.piles;
            Action action = player.choose_action(game.piles);

            last[game.player].state = state;
            last[game.player].action = action;

            // Make move
            game.move(action);
            State new_state = game.piles;

            // When game is over, update Q values with rewards
            if (game.winner.has_value()) {
                player.update(state, action, new_state, -1.0);
                
                // Note: game.move() switches player. So game.player here represents the OTHER player.
                player.update(last[game.player].state.value(), last[game.player].action.value(), new_state, 1.0);
                break;
            } 
            // If game is continuing, no rewards yet
            else if (last[game.player].state.has_value()) {
                player.update(last[game.player].state.value(), last[game.player].action.value(), new_state, 0.0);
            }
        }
    }

    std::println("Done training");
    return player;
}

void play(NimAI& ai, std::optional<int> human_player = std::nullopt) {
    std::mt19937 gen(std::random_device{}());
    
    // If no player order set, choose human's order randomly
    if (!human_player.has_value()) {
        std::uniform_int_distribution<> dis(0, 1);
        human_player = dis(gen);
    }

    Nim game;

    while (true) {
        std::println("");
        std::println("Piles:");
        for (size_t i = 0; i < game.piles.size(); ++i) {
            std::println("Pile {}: {}", i, game.piles[i]);
        }
        std::println("");

        auto available_actions = Nim::available_actions(game.piles);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (game.player == human_player.value()) {
            std::println("Your Turn");
            while (true) {
                int pile = -1, count = -1;
                std::print("Choose Pile: ");
                std::cin >> pile;
                std::print("Choose Count: ");
                std::cin >> count;

                // Handle string inputs failing standard cin gracefully
                if (
