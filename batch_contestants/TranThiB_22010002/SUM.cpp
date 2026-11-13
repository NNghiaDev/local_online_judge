#include <iostream>

int main() {
    long long a, b;
    std::cin >> a >> b;
    // Intentional infinite loop to test Windows Job Object TLE killer
    while (true) {
        // busy wait
    }
    std::cout << a + b << "\n";
    return 0;
}
