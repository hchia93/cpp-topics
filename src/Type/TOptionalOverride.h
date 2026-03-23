#pragma once

// Optional override for a single value. Separates "whether set" from
// "what value", eliminating sentinel conventions (e.g. -1.0f meaning
// "not set", 1.0f meaning "no multiplier").
//
// Set marks the override as active. Reset clears it back to passive.
// AssignOr and MultiplyOr resolve against a fallback or accumulator,
// passing through unchanged when the override is not set.
//
// Usage:
//     TOptionalOverride<float> Speed;
//     Speed.Set(300.0f);
//     float Result = Speed.AssignOr(600.0f);      // 300.0f
//     Speed.Reset();
//     float Fallback = Speed.AssignOr(600.0f);    // 600.0f
//
template<typename T>
struct TOptionalOverride
{
    void Set(T InValue)
    {
        StoredValue = InValue;
        bSet = true;
    }

    void Reset()
    {
        bSet = false;
    }

    bool IsSet() const
    {
        return bSet;
    }

    T GetValue() const
    {
        return StoredValue;
    }

    // Return the override value if set, otherwise the fallback.
    T AssignOr(T Fallback) const
    {
        return bSet ? StoredValue : Fallback;
    }

    // Multiply accumulator by the override value if set, otherwise pass through.
    T MultiplyOr(T Accumulator) const
    {
        return bSet ? Accumulator * StoredValue : Accumulator;
    }

private:
    T StoredValue{};
    bool bSet = false;
};
