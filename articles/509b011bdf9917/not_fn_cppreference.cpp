template<class F>
struct not_fn_t {
    F f;
    template<class... Args>
    constexpr auto operator()(Args&&... args) &
        noexcept(noexcept(!std::invoke(f, std::forward<Args>(args)...)))
        -> decltype(!std::invoke(f, std::forward<Args>(args)...))
    {
        return !std::invoke(f, std::forward<Args>(args)...);
    }

    template<class... Args>
    constexpr auto operator()(Args&&... args) const&
        noexcept(noexcept(!std::invoke(f, std::forward<Args>(args)...)))
        -> decltype(!std::invoke(f, std::forward<Args>(args)...))
    {
        return !std::invoke(f, std::forward<Args>(args)...);
    }

    template<class... Args>
    constexpr auto operator()(Args&&... args) &&
        noexcept(noexcept(!std::invoke(std::move(f), std::forward<Args>(args)...)))
        -> decltype(!std::invoke(std::move(f), std::forward<Args>(args)...))
    {
        return !std::invoke(std::move(f), std::forward<Args>(args)...);
    }

    template<class... Args>
    constexpr auto operator()(Args&&... args) const&&
        noexcept(noexcept(!std::invoke(std::move(f), std::forward<Args>(args)...)))
        -> decltype(!std::invoke(std::move(f), std::forward<Args>(args)...))
    {
        return !std::invoke(std::move(f), std::forward<Args>(args)...);
    }
};
