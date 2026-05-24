#pragma once

#include <cstddef>

template <size_t DataSize, typename T = float>
class moving_average
{
    T data[DataSize];
    size_t write_index;
    T sum;

public:
    moving_average() noexcept
        : data()
        , write_index()
        , sum()
    {
    }

    void update(T value) noexcept
    {
        sum -= data[write_index];
        data[write_index++] = value;
        sum += value;
        if (write_index >= DataSize)
            write_index = 0;
    }

    T get_value() const noexcept
    {
        return sum / static_cast<T>(DataSize);
    }
};