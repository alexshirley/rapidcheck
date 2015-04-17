#pragma once

#include "rapidcheck/shrinkable/Create.h"
#include "rapidcheck/gen/Arbitrary.h"
#include "rapidcheck/gen/Transform.h"

namespace rc {
namespace gen {
namespace detail {

template<typename Indexes, typename ...Ts>
class TupleGen;

template<typename ...Ts,
         std::size_t ...Indexes>
class TupleGen<rc::detail::IndexSequence<Indexes...>, Ts...>
{
public:
    template<typename ...Args>
    explicit TupleGen(Args &&...args)
        : m_gens(std::forward<Args>(args)...) {}

    Shrinkable<std::tuple<Ts...>> operator()(
        const Random &random, int size) const
    {
        Random r(random);
        Random randoms[sizeof...(Ts)];
        for (std::size_t i = 0; i < sizeof...(Ts); i++)
            randoms[i] = r.split();

        return joinTuple(
            std::make_tuple(std::get<Indexes>(m_gens)(randoms[Indexes], size)...));
    }

private:
    static Shrinkable<std::tuple<Ts...>> joinTuple(
        std::tuple<Shrinkable<Ts>...> &&tuple)
    {
        return shrinkable::map(
            shrinkable::shrinkRecur(std::move(tuple), &TupleGen::shrinkTuple),
            [](std::tuple<Shrinkable<Ts>...> &&st) {
                return std::make_tuple(std::get<Indexes>(st).value()...);
            });
    }

    static Seq<std::tuple<Shrinkable<Ts>...>> shrinkTuple(
        std::tuple<Shrinkable<Ts>...> &&tuple)
    {
        return seq::concat(shrinkComponent<Indexes>(tuple)...);
    }

    template<std::size_t N>
    static Seq<std::tuple<Shrinkable<Ts>...>> shrinkComponent(
        const std::tuple<Shrinkable<Ts>...> &tuple)
    {
        typedef typename std::tuple_element<N, std::tuple<Ts...>>::type T;
        return seq::map(
            std::get<N>(tuple).shrinks(),
            [=](Shrinkable<T> &&cshrink) {
                auto shrink(tuple);
                std::get<N>(shrink) = cshrink;
                return shrink;
            });
    }

    std::tuple<Gen<Ts>...> m_gens;
};

// Specialization for empty tuples.
template<>
class TupleGen<rc::detail::IndexSequence<>>
{
public:
    Shrinkable<std::tuple<>> operator()(const Random &random, int size) const
    { return shrinkable::just(std::tuple<>()); }
};

template<typename ...Ts>
struct DefaultArbitrary<std::tuple<Ts...>>
{
    static Gen<std::tuple<Decay<Ts>...>> arbitrary()
    {
        return gen::tuple(gen::arbitrary<Decay<Ts>>()...);
    }
};

template<typename T1, typename T2>
struct DefaultArbitrary<std::pair<T1, T2>>
{
    static Gen<std::pair<Decay<T1>, Decay<T2>>> arbitrary()
    {
        return gen::pair(gen::arbitrary<Decay<T1>>(),
                            gen::arbitrary<Decay<T2>>());
    }
};

} // namespace detail

template<typename ...Ts>
Gen<std::tuple<Ts...>> tuple(Gen<Ts> ...gens)
{
    return detail::TupleGen<rc::detail::MakeIndexSequence<sizeof...(Ts)>, Ts...>(
        std::move(gens)...);
}

template<typename T1, typename T2>
Gen<std::pair<T1, T2>> pair(Gen<T1> gen1, Gen<T2> gen2)
{
    return gen::cast<std::pair<T1, T2>>(
        gen::tuple(std::move(gen1),
                      std::move(gen2)));
}

} // namespace gen
} // namespace rc