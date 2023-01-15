#pragma once

//#include <type_traits>

template <typename T>
void isPodType()
{
    static constexpr bool myStandardLayout = __is_standard_layout(T);
    /*static constexpr bool trivial = std::is_trivial<T>();*/
    static constexpr bool myStandardTrivialCopy = __is_trivially_copyable(T);
    static_assert((myStandardLayout && myStandardTrivialCopy) && "Was not pod-type but required pod");
}

template <typename T>
void isNotPodType()
{
    static constexpr bool myStandardLayout = __is_standard_layout(T);
    /*static constexpr bool trivial = std::is_trivial<T>();*/
    static constexpr bool myStandardTrivialCopy = __is_trivially_copyable(T);
    static_assert(!(myStandardLayout && myStandardTrivialCopy) && "Was pod-type but required non-pod");
}
