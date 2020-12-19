#include <cmath>
#include "include/shared.h"

Bitset asBitset(const std::set<uint64_t> &set) {
    Bitset result;
    for (const uint64_t &element: set) {
        result.set(element, true);
    }
    return result;
}

std::set<uint64_t> asSet(const Bitset &set, std::size_t n) {
    std::set<uint64_t> result;
    for (std::size_t v = 0; v < n; ++v) {
        if (!set[v]) continue;
        result.insert(v);
    }
    return result;
}

bool isNumberCloseToInteger(double number, double eps) {
    double up = std::ceil(number);
    double down = std::floor(number);

    if (number + eps > up || number - eps < down) {
        return true;
    }
    return false;
}

bool isNumberInteger(double number) {
    double int_part;
    return modf(number, &int_part) == 0.0;
}