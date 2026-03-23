#pragma once

#include <string>
#include <unordered_map>
#include <concepts>

template<typename T>
concept CBoolCastable = requires(T Value)
{
    { static_cast<bool>(Value) } -> std::same_as<bool>;
};

// Shared activation control from named sources to allow deactivation idempotent.
// Used for stateful operation from multiple systems, where the operations
// may be overlapped.
//
// Usage:
//     TSharedState<bool> State;
//     State.Set("P-Source", true);
//     State.Set("Q-Source", true);
//     State.Unset("P-Source");       // Q still holds -> active
//     State.Unset("Q-Source");       // no entries left -> inactive
//     State.Unset("Q-Source");       // no-op, safe
//
template<CBoolCastable T>
struct TSharedState
{
    // Set from a source. Duplicate sets from the same source refresh the value.
    void Set(const std::string& Source, T Value)
    {
        Entries[Source] = Value;
    }

    // Unset from a source. No-op if no existing entry is found.
    void Unset(const std::string& Source)
    {
        Entries.erase(Source);
    }

    // Remove all entries. Use for system-level reset only.
    void Reset()
    {
        Entries.clear();
    }

    // Return true if any source holds a truthy value.
    bool IsActive() const
    {
        for (const auto& [Source, Value] : Entries)
        {
            if (static_cast<bool>(Value))
            {
                return true;
            }
        }
        return false;
    }

    bool HasSource(const std::string& Source) const
    {
        return Entries.contains(Source);
    }

private:
    std::unordered_map<std::string, T> Entries;
};
