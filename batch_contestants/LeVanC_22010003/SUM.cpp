#include <iostream>

int main() {
    long long a, b;
    if (std::cin >> a >> b) {
        // Intentional wrong answer
        std::cout << (a + b + 1) << "\n";
    }
    return 0;
}
