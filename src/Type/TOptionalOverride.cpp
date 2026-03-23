#include "TOptionalOverride.h"
#include <iostream>
#include <string_view>
#include <cassert>
#include <cmath>

void PrintOverride(std::string_view Label, const TOptionalOverride<float>& Override)
{
    std::cout << Label << " | IsSet: " << Override.IsSet();
    if (Override.IsSet())
    {
        std::cout << ", Value: " << Override.GetValue();
    }
    std::cout << "\n";
}

int main()
{
    // Basic set/reset
    {
        TOptionalOverride<float> Speed;
        assert(Speed.IsSet() == false);

        Speed.Set(300.0f);
        assert(Speed.IsSet() == true);
        assert(Speed.GetValue() == 300.0f);
        PrintOverride("After Set(300)", Speed);

        Speed.Reset();
        assert(Speed.IsSet() == false);
        PrintOverride("After Reset", Speed);
    }

    // AssignOr: override replaces fallback when set, passes through when not
    {
        TOptionalOverride<float> Speed;
        float Default = 600.0f;

        assert(Speed.AssignOr(Default) == 600.0f);

        Speed.Set(300.0f);
        assert(Speed.AssignOr(Default) == 300.0f);
    }

    // MultiplyOr: scales accumulator when set, passes through when not
    {
        TOptionalOverride<float> Multiplier;
        float Base = 100.0f;

        assert(Multiplier.MultiplyOr(Base) == 100.0f);

        Multiplier.Set(1.5f);
        assert(std::abs(Multiplier.MultiplyOr(Base) - 150.0f) < 0.001f);
    }

    // Override with 0 is a valid set state
    {
        TOptionalOverride<float> Gravity;
        Gravity.Set(0.0f);
        assert(Gravity.IsSet() == true);
        assert(Gravity.AssignOr(9.8f) == 0.0f);
    }

    // Integer type
    {
        TOptionalOverride<int> Priority;
        assert(Priority.AssignOr(5) == 5);

        Priority.Set(10);
        assert(Priority.AssignOr(5) == 10);
    }

    std::cout << "All tests passed.\n";
    return 0;
}
