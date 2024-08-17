#include <iostream>
#include <chrono>

extern "C" {
    double area_of_circle(double);
    double loop_test();
    double abs_lisa(double);
    double sqrt_lisa(double);
    double fib_lisa(double);
}

int loop_test_cpp() {
    int a = 0;
    int b = 0;
    for (int i = 0; i < 50; i++) {
        a += i;
        for (int j = 0; j < 10; j++) {
            b = a + j;
        }
    }
    return b;
}

int main() {
    // time the function execution
    auto start = std::chrono::high_resolution_clock::now();
    double res = fib_lisa(4);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "fib_lisa() returned " << res << std::endl;
    std::cout << "Time taken by fib_lisa(): " << duration.count() << " ms" << std::endl;

    return 0;
}
