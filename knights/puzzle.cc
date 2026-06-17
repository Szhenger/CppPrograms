#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include <print>
#include <cctype>

// ============================================================================
// Logic Framework Definition
// ============================================================================

class Sentence {
public:
    virtual ~Sentence() = default;
    virtual bool evaluate(const std::unordered_map<std::string, bool>& model) const = 0;
    virtual std::string formula() const = 0;
    virtual std::unordered_set<std::string> symbols() const = 0;
    virtual std::string repr() const = 0;

    static bool balanced(std::string_view s) {
        int count = 0;
        for (char c : s) {
            if (c == '(') count++;
            else if (c == ')') {
                if (count <= 0) return false;
                count--;
            }
        }
        return count == 0;
    }

    static std::string parenthesize(const std::string& s) {
        if (s.empty()) return s;
        bool is_alpha = std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalpha(c); });
        if (is_alpha) return s;

        if (s.front() == '(' && s.back() == ')') {
            std::string_view inner(s.data() + 1, s.size() - 2);
            if (balanced(inner)) return s;
        }
        return "(" + s + ")";
    }

    static void validate(const std::shared_ptr<Sentence>& sentence) {
        if (!sentence) throw std::invalid_argument("must be a logical sentence");
    }
};

class Symbol : public Sentence {
public:
    std::string name;
    Symbol(std::string name) : name(std::move(name)) {}

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        if (auto it = model.find(name); it != model.end()) return it->second;
        throw std::runtime_error("variable " + name + " not in model");
    }
    std::string formula() const override { return name; }
    std::unordered_set<std::string> symbols() const override { return {name}; }
    std::string repr() const override { return name; }
};

class Not : public Sentence {
public:
    std::shared_ptr<Sentence> operand;
    Not(std::shared_ptr<Sentence> op) : operand(std::move(op)) { Sentence::validate(operand); }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override { return !operand->evaluate(model); }
    std::string formula() const override { return "¬" + Sentence::parenthesize(operand->formula()); }
    std::unordered_set<std::string> symbols() const override { return operand->symbols(); }
    std::string repr() const override { return "Not(" + operand->repr() + ")"; }
};

class And : public Sentence {
public:
    std::vector<std::shared_ptr<Sentence>> conjuncts;
    And(std::initializer_list<std::shared_ptr<Sentence>> args) : conjuncts(args) {
        for (const auto& c : conjuncts) Sentence::validate(c);
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return std::all_of(conjuncts.begin(), conjuncts.end(), [&](const auto& c) { return c->evaluate(model); });
    }
    std::string formula() const override {
        if (conjuncts.empty()) return "";
        if (conjuncts.size() == 1) return conjuncts[0]->formula();
        std::string result = Sentence::parenthesize(conjuncts[0]->formula());
        for (size_t i = 1; i < conjuncts.size(); ++i) {
            result += " ∧ " + Sentence::parenthesize(conjuncts[i]->formula());
        }
        return result;
    }
    std::unordered_set<std::string> symbols() const override {
        std::unordered_set<std::string> total;
        for (const auto& c : conjuncts) {
            auto syms = c->symbols();
            total.insert(syms.begin(), syms.end());
        }
        return total;
    }
    std::string repr() const override {
        std::string result = "And(";
        for (size_t i = 0; i < conjuncts.size(); ++i) {
            result += conjuncts[i]->repr() + (i + 1 < conjuncts.size() ? ", " : "");
        }
        return result + ")";
    }
};

class Or : public Sentence {
public:
    std::vector<std::shared_ptr<Sentence>> disjuncts;
    Or(std::initializer_list<std::shared_ptr<Sentence>> args) : disjuncts(args) {
        for (const auto& d : disjuncts) Sentence::validate(d);
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return std::any_of(disjuncts.begin(), disjuncts.end(), [&](const auto& d) { return d->evaluate(model); });
    }
    std::string formula() const override {
        if (disjuncts.empty()) return "";
        if (disjuncts.size() == 1) return disjuncts[0]->formula();
        std::string result = Sentence::parenthesize(disjuncts[0]->formula());
        for (size_t i = 1; i < disjuncts.size(); ++i) {
            result += " ∨  " + Sentence::parenthesize(disjuncts[i]->formula());
        }
        return result;
    }
    std::unordered_set<std::string> symbols() const override {
        std::unordered_set<std::string> total;
        for (const auto& d : disjuncts) {
            auto syms = d->symbols();
            total.insert(syms.begin(), syms.end());
        }
        return total;
    }
    std::string repr() const override {
        std::string result = "Or(";
        for (size_t i = 0; i < disjuncts.size(); ++i) {
            result += disjuncts[i]->repr() + (i + 1 < disjuncts.size() ? ", " : "");
        }
        return result + ")";
    }
};

class Implication : public Sentence {
public:
    std::shared_ptr<Sentence> antecedent;
    std::shared_ptr<Sentence> consequent;
    Implication(std::shared_ptr<Sentence> ant, std::shared_ptr<Sentence> cons)
        : antecedent(std::move(ant)), consequent(std::move(cons)) {
        Sentence::validate(antecedent);
        Sentence::validate(consequent);
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return !antecedent->evaluate(model) || consequent->evaluate(model);
    }
    std::string formula() const override {
        return Sentence::parenthesize(antecedent->formula()) + " => " + Sentence::parenthesize(consequent->formula());
    }
    std::unordered_set<std::string> symbols() const override {
        auto total = antecedent->symbols();
        auto cons = consequent->symbols();
        total.insert(cons.begin(), cons.end());
        return total;
    }
    std::string repr() const override { return "Implication(" + antecedent->repr() + ", " + consequent->repr() + ")"; }
};

// ============================================================================
// Model Checking Logic Engine
// ============================================================================

bool check_all(
    const std::shared_ptr<Sentence>& knowledge,
    const std::shared_ptr<Sentence>& query,
    std::unordered_set<std::string> symbols,
    std::unordered_map<std::string, bool> model) 
{
    if (symbols.empty()) {
        if (knowledge->evaluate(model)) {
            return query->evaluate(model);
        }
        return true;
    } else {
        auto it = symbols.begin();
        std::string p = *it;
        symbols.erase(it);

        auto model_true = model;   model_true[p] = true;
        auto model_false = model;  model_false[p] = false;

        return check_all(knowledge, query, symbols, model_true) &&
               check_all(knowledge, query, symbols, model_false);
    }
}

bool model_check(const std::shared_ptr<Sentence>& knowledge, const std::shared_ptr<Sentence>& query) {
    auto symbols = knowledge->symbols();
    auto query_symbols = query->symbols();
    symbols.insert(query_symbols.begin(), query_symbols.end());
    return check_all(knowledge, query, symbols, {});
}

// ============================================================================
// Main Application / Puzzle Verification
// ============================================================================

int main() {
    // Definining Global Puzzle Symbols
    auto AKnight = std::make_shared<Symbol>("A is a Knight");
    auto AKnave  = std::make_shared<Symbol>("A is a Knave");
    auto BKnight = std::make_shared<Symbol>("B is a Knight");
    auto BKnave  = std::make_shared<Symbol>("B is a Knave");
    auto CKnight = std::make_shared<Symbol>("C is a Knight");
    auto CKnave  = std::make_shared<Symbol>("C is a Knave");

    // Puzzle 0: A says "I am both a knight and a knave."
    auto knowledge0 = std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Implication>(AKnight, std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Implication>(AKnave, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{
            std::make_shared<Not>(AKnight), std::make_shared<Not>(AKnave)
        }))
    });

    // Puzzle 1: A says "We are both knaves."
    auto knowledge1 = std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{BKnight, BKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{BKnight, BKnave})),
        std::make_shared<Implication>(AKnight, std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnave, BKnave})),
        std::make_shared<Implication>(AKnave, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, BKnave}),
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnave, BKnight})
        }))
    });

    // Puzzle 2: A says "We are the same kind." B says "We are of different kinds."
    auto knowledge2 = std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{BKnight, BKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{BKnight, BKnave})),
        
        std::make_shared<Implication>(AKnight, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, BKnight}),
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnave, BKnave})
        })),
        std::make_shared<Implication>(AKnave, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, BKnave}),
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnave, BKnight})
        })),
        std::make_shared<Implication>(BKnight, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, BKnave}),
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnave, BKnight})
        })),
        std::make_shared<Implication>(BKnave, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, BKnight}),
            std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnave, BKnave})
        }))
    });

    // Puzzle 3: Hidden complexities of A, B, and C's cross-referencing logic statements
    auto knowledge3 = std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{BKnight, BKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{BKnight, BKnave})),
        std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{CKnight, CKnave}),
        std::make_shared<Not>(std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{CKnight, CKnave})),

        std::make_shared<Implication>(AKnight, std::make_shared<Or>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Implication>(AKnave, std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{AKnight, AKnave})),
        std::make_shared<Implication>(BKnight, std::make_shared<Implication>(AKnight, BKnave)),
        std::make_shared<Implication>(BKnight, CKnave),
        std::make_shared<Implication>(BKnave, CKnight),
        std::make_shared<Implication>(CKnight, AKnight),
        std::make_shared<Implication>(CKnave, AKnave)
    });

    std::vector<std::shared_ptr<Symbol>> symbols = {AKnight, AKnave, BKnight, BKnave, CKnight, CKnave};
    
    std::vector<std::pair<std::string, std::shared_ptr<And>>> puzzles = {
        {"Puzzle 0", knowledge0},
        {"Puzzle 1", knowledge1},
        {"Puzzle 2", knowledge2},
        {"Puzzle 3", knowledge3}
    };

    for (const auto& [name, knowledge] : puzzles) {
        std::println("{}", name);
        if (knowledge->conjuncts.empty()) {
            std::println("    Not yet implemented.");
        } else {
            for (const auto& symbol : symbols) {
                if (model_check(knowledge, symbol)) {
                    std::println("    {}", symbol->repr());
                }
            }
        }
    }

    return 0;
}
