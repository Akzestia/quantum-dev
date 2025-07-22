#include <cmath>
#include <cstdio>
#include <qpp/qpp.hpp>
#include <random>
#include <vector>

using namespace qpp;

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLD "\033[1m"
#define DIM "\033[2m"

class E91Protocol {
  private:
    std::mt19937 rng;
    idx n_pairs;
    std::vector<idx> alice_bases;
    std::vector<idx> bob_bases;
    std::vector<idx> alice_results;
    std::vector<idx> bob_results;
    std::vector<bool> shared_key;

    const std::vector<double> measurement_angles = {0.0, M_PI / 8, M_PI / 4,
                                                    3 * M_PI / 8};

  public:
    E91Protocol(idx num_pairs, unsigned seed = std::random_device{}())
        : rng(seed), n_pairs(num_pairs) {
        alice_bases.reserve(num_pairs);
        bob_bases.reserve(num_pairs);
        alice_results.reserve(num_pairs);
        bob_results.reserve(num_pairs);
    }

    void run_protocol() {
        std::uniform_int_distribution<idx> alice_basis_dist(0, 2);  // 0, 1, 2
        std::uniform_int_distribution<idx> bob_basis_dist(1, 3);    // 1, 2, 3

        printf(
            CYAN
            "Generating %lu EPR pairs and performing measurements...\n" RESET,
            static_cast<unsigned long>(n_pairs));

        for (idx i = 0; i < n_pairs; ++i) {
            idx alice_basis = alice_basis_dist(rng);
            alice_bases.push_back(alice_basis);

            idx bob_basis = bob_basis_dist(rng);
            bob_bases.push_back(bob_basis);

            auto results = measure_bell_pair(alice_basis, bob_basis);
            alice_results.push_back(results.first);
            bob_results.push_back(results.second);
        }
    }

    std::pair<idx, idx> measure_bell_pair(idx alice_basis, idx bob_basis) {
        double alice_angle = measurement_angles[alice_basis];
        double bob_angle = measurement_angles[bob_basis];

        double angle_diff = alice_angle - bob_angle;
        double correlation = -std::cos(2 * angle_diff);

        std::uniform_real_distribution<double> uniform(0.0, 1.0);

        idx alice_result = (uniform(rng) < 0.5) ? 0 : 1;

        double prob_same = (1 + correlation) / 2;
        idx bob_result;
        if (uniform(rng) < prob_same) {
            bob_result = alice_result;
        } else {
            bob_result = 1 - alice_result;
        }

        return std::make_pair(alice_result, bob_result);
    }

    void sift_key() {
        shared_key.clear();

        printf(YELLOW
               "Sifting key from measurements with matching bases...\n" RESET);

        idx matching_count = 0;
        for (idx i = 0; i < n_pairs; ++i) {
            if (alice_bases[i] == bob_bases[i]) {
                shared_key.push_back(alice_results[i] == 1);
                matching_count++;
            }
        }

        printf(WHITE "Matching measurements: " GREEN "%lu" WHITE " out of " BLUE
                     "%lu" WHITE " pairs\n" RESET,
               static_cast<unsigned long>(matching_count),
               static_cast<unsigned long>(n_pairs));
        printf(WHITE "Sifted key length: " BOLD GREEN "%lu" RESET WHITE
                     " bits\n" RESET,
               static_cast<unsigned long>(shared_key.size()));
    }

    double perform_bell_test() {
        printf(MAGENTA "Performing Bell inequality test for security "
                       "verification...\n" RESET);

        double E_ab =
            calculate_correlation(0, 1);  // Alice angle 0, Bob angle 1
        double E_ab_prime =
            calculate_correlation(0, 2);  // Alice angle 0, Bob angle 2
        double E_a_prime_b =
            calculate_correlation(2, 1);  // Alice angle 2, Bob angle 1
        double E_a_prime_b_prime =
            calculate_correlation(2, 2);  // Alice angle 2, Bob angle 2

        double S =
            std::abs(E_ab - E_ab_prime + E_a_prime_b + E_a_prime_b_prime);

        printf(DIM "CHSH correlations:\n" RESET);
        printf("  " WHITE "E(0,π/8) = " CYAN "%.6f\n" RESET, E_ab);
        printf("  " WHITE "E(0,π/4) = " CYAN "%.6f\n" RESET, E_ab_prime);
        printf("  " WHITE "E(π/4,π/8) = " CYAN "%.6f\n" RESET, E_a_prime_b);
        printf("  " WHITE "E(π/4,π/4) = " CYAN "%.6f\n" RESET,
               E_a_prime_b_prime);
        printf(BOLD WHITE "CHSH parameter S = " YELLOW "%.6f\n" RESET, S);

        double quantum_bound = 2.0 * std::sqrt(2.0);
        printf(DIM "Quantum bound: " GREEN "%.6f\n" RESET, quantum_bound);
        printf(DIM "Classical bound: " RED "2.000000\n" RESET);

        return S;
    }

    double calculate_correlation(idx alice_angle_idx, idx bob_angle_idx) {
        idx count_00 = 0, count_01 = 0, count_10 = 0, count_11 = 0;
        idx total = 0;

        for (idx i = 0; i < n_pairs; ++i) {
            if (alice_bases[i] == alice_angle_idx
                && bob_bases[i] == bob_angle_idx) {
                if (alice_results[i] == 0 && bob_results[i] == 0)
                    count_00++;
                else if (alice_results[i] == 0 && bob_results[i] == 1)
                    count_01++;
                else if (alice_results[i] == 1 && bob_results[i] == 0)
                    count_10++;
                else
                    count_11++;
                total++;
            }
        }

        if (total == 0)
            return 0.0;

        double p00 = static_cast<double>(count_00) / total;
        double p01 = static_cast<double>(count_01) / total;
        double p10 = static_cast<double>(count_10) / total;
        double p11 = static_cast<double>(count_11) / total;

        return p00 + p11 - p01 - p10;
    }

    double estimate_error_rate() {
        idx matching_pairs = 0;
        idx total_same_basis = 0;

        for (idx i = 0; i < n_pairs; ++i) {
            if (alice_bases[i] == bob_bases[i]) {
                total_same_basis++;
                if (alice_results[i] == bob_results[i]) {
                    matching_pairs++;
                }
            }
        }

        if (total_same_basis == 0)
            return 0.0;

        double error_rate =
            1.0 - static_cast<double>(matching_pairs) / total_same_basis;
        return error_rate;
    }

    void display_results() {
        printf("\n" BOLD BLUE "=== E91 QKD Protocol Results ===" RESET "\n");
        printf(WHITE "Total EPR pairs generated: " BLUE "%lu\n" RESET,
               static_cast<unsigned long>(n_pairs));
        printf(WHITE "Final shared key length: " BOLD GREEN "%lu" RESET WHITE
                     " bits\n" RESET,
               static_cast<unsigned long>(shared_key.size()));

        if (shared_key.size() > 0) {
            double efficiency =
                static_cast<double>(shared_key.size()) / n_pairs * 100;
            printf(WHITE "Key efficiency: " YELLOW "%.1f%%\n" RESET,
                   efficiency);
        }

        double error_rate = estimate_error_rate();
        if (error_rate < 0.05) {
            printf(WHITE "Estimated error rate: " GREEN "%.2f%%\n" RESET,
                   error_rate * 100);
        } else if (error_rate < 0.15) {
            printf(WHITE "Estimated error rate: " YELLOW "%.2f%%\n" RESET,
                   error_rate * 100);
        } else {
            printf(WHITE "Estimated error rate: " RED "%.2f%%\n" RESET,
                   error_rate * 100);
        }

        if (shared_key.size() > 0) {
            size_t display_bits = std::min(20UL, shared_key.size());
            printf(WHITE "First %lu bits of shared key: " BOLD,
                   static_cast<unsigned long>(display_bits));
            for (idx i = 0; i < display_bits; ++i) {
                if (shared_key[i]) {
                    printf(GREEN "1" RESET);
                } else {
                    printf(RED "0" RESET);
                }
            }
            printf("\n");
        }
    }

    const std::vector<bool>& get_shared_key() const { return shared_key; }
};

int main() {
    try {
        printf(BOLD CYAN
               "E91 Quantum Key Distribution Protocol Simulation\n" RESET);
        printf(BOLD CYAN
               "================================================\n\n" RESET);

        E91Protocol e91(1000);

        e91.run_protocol();
        printf("\n");

        e91.sift_key();
        printf("\n");

        double chsh_parameter = e91.perform_bell_test();

        e91.display_results();

        printf("\n" BOLD MAGENTA "=== Security Analysis ===" RESET "\n");
        if (chsh_parameter > 2.0) {
            printf(BOLD GREEN "✓ Bell inequality VIOLATED - Quantum "
                              "correlations confirmed\n" RESET);
            if (chsh_parameter > 2.5) {
                printf(BOLD GREEN "✓ Strong quantum correlations - Protocol "
                                  "appears secure\n" RESET);
            } else {
                printf(YELLOW "⚠ Weak quantum correlations - Check for noise "
                              "or eavesdropping\n" RESET);
            }
        } else {
            printf(BOLD RED "✗ Bell inequality NOT violated - Classical "
                            "correlations detected\n" RESET);
            printf(BOLD RED
                   "✗ Potential eavesdropping or system malfunction\n" RESET);
        }

        printf("\n" DIM "Protocol completed successfully." RESET "\n");

    } catch (const std::exception& e) {
        printf(BOLD RED "Error: %s\n" RESET, e.what());
        return 1;
    }

    return 0;
}
