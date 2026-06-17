#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <optional>
#include <print>

// ============================================================================
// Core Constants and Structures
// ============================================================================

const double PROBS_MUTATION = 0.01;

const std::map<int, double> PROBS_GENE = {
    {2, 0.01},
    {1, 0.03},
    {0, 0.96}
};

const std::map<int, std::map<bool, double>> PROBS_TRAIT = {
    {2, {{true, 0.65}, {false, 0.35}}},
    {1, {{true, 0.56}, {false, 0.44}}},
    {0, {{true, 0.01}, {false, 0.99}}}
};

struct Person {
    std::string name;
    std::optional<std::string> mother;
    std::optional<std::string> father;
    std::optional<bool> trait;
};

struct Probabilities {
    std::map<int, double> gene = {{0, 0.0}, {1, 0.0}, {2, 0.0}};
    std::map<bool, double> trait = {{true, 0.0}, {false, 0.0}};
};

// ============================================================================
// Helper Functions
// ============================================================================

std::unordered_map<std::string, Person> load_data(const std::string& filename) {
    std::unordered_map<std::string, Person> data;
    std::ifstream f(filename);
    
    if (!f.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    std::getline(f, line); // Skip header

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        
        // Handle Windows carriage returns
        if (line.back() == '\r') line.pop_back();

        std::stringstream ss(line);
        std::string name, mother, father, trait_str;
        
        std::getline(ss, name, ',');
        std::getline(ss, mother, ',');
        std::getline(ss, father, ',');
        std::getline(ss, trait_str, ',');

        Person p{name, std::nullopt, std::nullopt, std::nullopt};
        if (!mother.empty()) p.mother = mother;
        if (!father.empty()) p.father = father;
        if (trait_str == "1") p.trait = true;
        else if (trait_str == "0") p.trait = false;

        data[name] = p;
    }
    return data;
}

// Generates all subsets of a given set (powerset)
std::vector<std::unordered_set<std::string>> powerset(const std::unordered_set<std::string>& s) {
    std::vector<std::string> vec(s.begin(), s.end());
    std::vector<std::unordered_set<std::string>> result;
    int n = vec.size();
    
    // 2^n combinations
    for (int i = 0; i < (1 << n); ++i) {
        std::unordered_set<std::string> subset;
        for (int j = 0; j < n; ++j) {
            if (i & (1 << j)) {
                subset.insert(vec[j]);
            }
        }
        result.push_back(subset);
    }
    return result;
}

// Utility to subtract one set from another
std::unordered_set<std::string> set_difference(
    const std::unordered_set<std::string>& a, 
    const std::unordered_set<std::string>& b) {
    std::unordered_set<std::string> result;
    for (const auto& val : a) {
        if (!b.contains(val)) result.insert(val);
    }
    return result;
}

// ============================================================================
// Core Logic
// ============================================================================

double joint_probability(
    const std::unordered_map<std::string, Person>& people,
    const std::unordered_set<std::string>& one_gene,
    const std::unordered_set<std::string>& two_genes,
    const std::unordered_set<std::string>& have_trait) {
    
    double probability = 1.0;

    for (const auto& [person, info] : people) {
        auto mother = info.mother;
        auto father = info.father;

        double mother_probability = PROBS_MUTATION;
        if (mother.has_value()) {
            if (one_gene.contains(*mother)) mother_probability = 0.5;
            else if (two_genes.contains(*mother)) mother_probability = 1.0 - PROBS_MUTATION;
        }

        double father_probability = PROBS_MUTATION;
        if (father.has_value()) {
            if (one_gene.contains(*father)) father_probability = 0.5;
            else if (two_genes.contains(*father)) father_probability = 1.0 - PROBS_MUTATION;
        }

        if (one_gene.contains(person)) {
            double one_probability = 1.0;
            if (!mother.has_value() && !father.has_value()) {
                one_probability *= PROBS_GENE.at(1);
            } else {
                one_probability *= (mother_probability * (1.0 - father_probability) +
                                    father_probability * (1.0 - mother_probability));
            }

            if (have_trait.contains(person)) one_probability *= PROBS_TRAIT.at(1).at(true);
            else one_probability *= PROBS_TRAIT.at(1).at(false);

            probability *= one_probability;

        } else if (two_genes.contains(person)) {
            double two_probability = 1.0;
            if (!mother.has_value() && !father.has_value()) {
                two_probability *= PROBS_GENE.at(2);
            } else {
                two_probability *= mother_probability * father_probability;
            }

            if (have_trait.contains(person)) two_probability *= PROBS_TRAIT.at(2).at(true);
            else two_probability *= PROBS_TRAIT.at(2).at(false); // Fixed Python Typo here!

            probability *= two_probability;

        } else {
            double zero_probability = 1.0;
            if (!mother.has_value() && !father.has_value()) {
                zero_probability *= PROBS_GENE.at(0);
            } else {
                zero_probability *= (1.0 - mother_probability) * (1.0 - father_probability);
            }

            if (have_trait.contains(person)) zero_probability *= PROBS_TRAIT.at(0).at(true);
            else zero_probability *= PROBS_TRAIT.at(0).at(false);

            probability *= zero_probability;
        }
    }

    return probability;
}

void update(
    std::unordered_map<std::string, Probabilities>& probabilities,
    const std::unordered_set<std::string>& one_gene,
    const std::unordered_set<std::string>& two_genes,
    const std::unordered_set<std::string>& have_trait,
    double p) {
    
    for (auto& [person, probs] : probabilities) {
        int gene = 0;
        if (one_gene.contains(person)) gene = 1;
        else if (two_genes.contains(person)) gene = 2;

        probs.gene[gene] += p;

        bool trait = have_trait.contains(person);
        probs.trait[trait] += p;
    }
}

void normalize(std::unordered_map<std::string, Probabilities>& probabilities) {
    for (auto& [person, probs] : probabilities) {
        // Normalize genes
        double gene_sum = 0;
        for (int i = 0; i < 3; ++i) gene_sum += probs.gene[i];
        for (int i = 0; i < 3; ++i) probs.gene[i] /= gene_sum;

        // Normalize traits
        double trait_sum = probs.trait[true] + probs.trait[false];
        probs.trait[true] /= trait_sum;
        probs.trait[false] /= trait_sum;
    }
}

// ============================================================================
// Main Execution
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::println(std::cerr, "Usage: ./heredity data.csv");
        return 1;
    }

    auto people = load_data(argv[1]);
    std::unordered_map<std::string, Probabilities> probabilities;
    std::unordered_set<std::string> names;
    
    for (const auto& [person, _] : people) {
        probabilities[person] = Probabilities{};
        names.insert(person);
    }

    // Loop over all sets of people who might have the trait
    for (const auto& have_trait : powerset(names)) {
        
        bool fails_evidence = false;
        for (const auto& person : names) {
            if (people[person].trait.has_value() && 
                people[person].trait.value() != have_trait.contains(person)) {
                fails_evidence = true;
                break;
            }
        }
        if (fails_evidence) continue;

        // Loop over all sets of people who might have the gene
        for (const auto& one_gene : powerset(names)) {
            auto remaining_names = set_difference(names, one_gene);
            
            for (const auto& two_genes : powerset(remaining_names)) {
                double p = joint_probability(people, one_gene, two_genes, have_trait);
                update(probabilities, one_gene, two_genes, have_trait, p);
            }
        }
    }

    normalize(probabilities);

    // Print results mapping identically to the Python outputs
    for (const auto& person : names) {
        std::println("{}:", person);
        std::println("  Gene:");
        for (int i = 2; i >= 0; --i) {
            std::println("    {}: {:.4f}", i, probabilities[person].gene[i]);
        }
        std::println("  Trait:");
        std::println("    True: {:.4f}", probabilities[person].trait[true]);
        std::println("    False: {:.4f}", probabilities[person].trait[false]);
    }

    return 0;
}
