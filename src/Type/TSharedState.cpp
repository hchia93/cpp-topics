#include "TSharedState.h"
#include <iostream>
#include <string_view>
#include <cassert>

void PrintState(std::string_view Label, const TSharedState<bool>& State)
{
    std::cout << Label << " | IsActive: " << State.IsActive() << "\n";
}

int main()
{
    // Basic set/unset with bool
    {
        TSharedState<bool> State;

        State.Set("P-Source", true);
        State.Set("Q-Source", true);
        PrintState("P+Q set", State);
        assert(State.IsActive() == true);

        State.Unset("P-Source");
        PrintState("P unset", State);
        assert(State.IsActive() == true);

        State.Unset("Q-Source");
        PrintState("Q unset", State);
        assert(State.IsActive() == false);

        // Idempotent unset
        State.Unset("Q-Source");
    }

    // Falsy set: source present but does not contribute
    {
        TSharedState<bool> State;

        State.Set("R-Source", false);
        assert(State.HasSource("R-Source") == true);
        assert(State.IsActive() == false);

        State.Set("R-Source", true);
        assert(State.IsActive() == true);
    }

    // Numeric type: 0 is present but inactive
    {
        TSharedState<int> State;

        State.Set("S-Source", 0);
        assert(State.HasSource("S-Source") == true);
        assert(State.IsActive() == false);

        State.Set("S-Source", 5);
        assert(State.IsActive() == true);
    }

    // Reset clears everything
    {
        TSharedState<bool> State;
        State.Set("P-Source", true);
        State.Set("Q-Source", true);
        State.Reset();
        assert(State.IsActive() == false);
    }

    std::cout << "All tests passed.\n";
    return 0;
}
