#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>

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

class Tester {
    mt19937 gen;
    uniform_int_distribution<unsigned> distrib;
    unsigned iterations;
    unsigned repeats;
    
public:

    Tester(mt19937::result_type seed, unsigned iterations, unsigned repeats) 
        : gen(seed), distrib(1, 600) {
        this->iterations = iterations;
        this->repeats = repeats;
    }

    template<typename T>
    void test(T (*f)(T a, T b), const char *message) {
        int64_t min = INT64_MAX;
        T ans = 0;

        for(unsigned i = 0; i < repeats; i++) {
            T res = 0;
            auto start = chrono::high_resolution_clock::now();
            for(unsigned j = 0; j < iterations; j++) {
                auto a = distrib(gen);
                auto b = distrib(gen);
                T ret = f(a, b);
                res += ret % 2;
                res %= 1000000;
            }
            auto end = chrono::high_resolution_clock::now();

            auto ms_int = chrono::duration_cast<chrono::milliseconds>(end - start);
            int64_t time = ms_int.count();
            if(time < min) {
                min = time;
                ans = res;
            }
        }
        
        cout << message << ' ' << min << "ms [out=" << ans << "]\n";
    }

    template<typename T>
    void test(T (*f)(T a, T b, T c), const char *message) {
        int64_t min = INT64_MAX;
        T ans = 0;

        for(unsigned i = 0; i < repeats; i++) {
            T res = 0;
            auto start = chrono::high_resolution_clock::now();
            for(unsigned j = 0; j < iterations; j++) {
                auto a = distrib(gen);
                auto b = distrib(gen);
                auto c = distrib(gen);
                T ret = f(a, b, c);
                res += ret % 2;
                res %= 1000000;
            }
            auto end = chrono::high_resolution_clock::now();

            auto ms_int = chrono::duration_cast<chrono::milliseconds>(end - start);
            int64_t time = ms_int.count();
            if(time < min) {
                min = time;
                ans = res;
            }
        }
        
        cout << message << ' ' << min << "ms [out=" << ans << "]\n";
    }
};

int main() {
    random_device rd;
    mt19937::result_type seed = rd() ^ (
        (mt19937::result_type)
        chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()
            ).count() +
        (mt19937::result_type)
        chrono::duration_cast<chrono::microseconds>(
            chrono::high_resolution_clock::now().time_since_epoch()
            ).count() );

    Tester tester(seed, 1000000000, 6);

    tester.test(cube_of_difference, "Cube of difference");
    tester.test(square_of_sum3, "Square of sum3");
    tester.test(square_of_difference, "Square of difference");
    tester.test(cubes_sum, "Cubes sum");
    tester.test(mul_fractions, "Mul fractions");

    return 0;
}
