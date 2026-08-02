#pragma once

#include <memory>
#include <utility>

template <typename T>
class SapOwned {
public:
    SapOwned() = default;
    explicit SapOwned(std::unique_ptr<T> object) noexcept
        : m_object(std::move(object)) {
    }

    ~SapOwned() noexcept {
        Destroy();
    }

    SapOwned(const SapOwned&) = delete;
    SapOwned& operator=(const SapOwned&) = delete;

    SapOwned(SapOwned&& other) noexcept = default;

    SapOwned& operator=(SapOwned&& other) noexcept {
        if (this != &other) {
            Destroy();
            m_object = std::move(other.m_object);
        }
        return *this;
    }

    void Reset(std::unique_ptr<T> object = nullptr) noexcept {
        Destroy();
        m_object = std::move(object);
    }

    bool Destroy() noexcept {
        if (m_object && static_cast<bool>(*m_object)) {
            return m_object->Destroy() != 0;
        }
        return true;
    }

    T* Get() const noexcept { return m_object.get(); }
    T* operator->() const noexcept { return m_object.get(); }
    T& operator*() const noexcept { return *m_object; }
    explicit operator bool() const noexcept { return m_object != nullptr; }

private:
    std::unique_ptr<T> m_object;
};
