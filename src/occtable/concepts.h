#pragma once

#include <utility>
#include <cstdint>
#include <cstddef>

template<typename T>
concept OccTable = requires(T t) {
    t.rank(uint64_t{}, uint8_t{});
    t.prefix_rank(uint64_t{}, uint8_t{});
    T::Sigma;
    t.size();
    T::expectedMemoryUsage(0);
};

template<template <auto> typename T>
concept checkOccTable = OccTable<T<1>>
                     && OccTable<T<2>>
                     && OccTable<T<4>>
                     && OccTable<T<5>>
                     && OccTable<T<254>>;
