// Entry point that runs every registered juce::UnitTest (each test file
// below registers itself via a static instance). Exits non-zero on any
// failure so this can be wired into CI or `ctest` as a real pass/fail gate.

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>

int main (int, char**)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    int numFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        numFailures += runner.getResult (i)->failures;

    std::cout << "\n" << (numFailures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << "\n";
    return numFailures == 0 ? 0 : 1;
}
