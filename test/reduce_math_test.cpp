#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace std;

int cube_of_difference(int a, int b) {
    return a * a * a - 3 * a * a * b + 3 * a * b * b - b * b * b;
}

int square_of_sum3(int a, int b, int c) {
    return a*a + b*b + c*c + 2*a*c + 2*a*b + 2*b*c;
}

int square_of_difference(int a, int b) {
    return a*a - 2*a*b + b*b ;
}

int cubes_sum(int a, int b) {
    return (a+b) * (a*a - a*b + b*b) ;
}

unsigned mul_fractions(unsigned a, unsigned b) {
    return a/b * b/a;
}

template<typename T>
struct Triple {
    T a;
    T b;
    T c;
};

class Tester {
    unsigned iterations;
    unsigned repeats;
    size_t argc;
    
public:

    Tester(unsigned iterations, unsigned repeats, size_t argc) {
        this->iterations = iterations;
        this->repeats = repeats;
        this->argc = argc;
    }

    template<typename T>
    void test(T (*f)(T a, T b), const char *message) {
        int64_t minTime = INT64_MAX;
        T ans = 0;
        vector<Triple<T>> args(argc);
        for(T i = 0; i < args.size(); i++) {
            T v = (T) (i % 600);
            args[i] = {v + 2, v + 1, v};
        }

        for(unsigned i = 0; i < repeats; i++) {
            T res = 0;
            auto start = chrono::high_resolution_clock::now();
            for(unsigned j = 0; j < iterations; j++) {
                auto [a, b, c] = args[j % args.size()];
                T ret = f(a, b);
                res += ret % 2 + 2;
                res %= 1000000;
            }
            auto end = chrono::high_resolution_clock::now();

            auto ms_int = chrono::duration_cast<chrono::milliseconds>(end - start);
            int64_t time = ms_int.count();
            if(time < minTime) {
                minTime = time;
                ans = res;
            }
        }
        
        cout << message << ' ' << minTime << "ms [out=" << ans << "]\n";
    }

    template<typename T>
    void test(T (*f)(T a, T b, T c), const char *message) {
        int64_t minTime = INT64_MAX;
        T ans = 0;
        vector<Triple<T>> args(argc);
        for(T i = 0; i < args.size(); i++) {
            T v = (T) (i % 600);
            args[i] = {v + 2, v + 1, v};
        }

        for(unsigned i = 0; i < repeats; i++) {
            T res = 0;
            auto start = chrono::high_resolution_clock::now();
            for(unsigned j = 0; j < iterations; j++) {
                auto [a, b, c] = args[j % args.size()];
                T ret = f(a, b, c);
                res += ret % 2 + 2;
                res %= 1000000;
            }
            auto end = chrono::high_resolution_clock::now();

            auto ms_int = chrono::duration_cast<chrono::milliseconds>(end - start);
            int64_t time = ms_int.count();
            if(time < minTime) {
                minTime = time;
                ans = res;
            }
        }
        
        cout << message << ' ' << minTime << "ms [out=" << ans << "]\n";
    }
};

int main() {
    Tester tester(1'500'000'000, 10, 10000);

    tester.test(cube_of_difference, "Cube of difference");
    tester.test(square_of_sum3, "Square of sum3");
    tester.test(square_of_difference, "Square of difference");
    tester.test(cubes_sum, "Cubes sum");
    tester.test(mul_fractions, "Mul fractions");

    return 0;
}
