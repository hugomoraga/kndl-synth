#include <JuceHeader.h>

//==============================================================================
// KndlSynth Test Runner
//
// Runs all registered juce::UnitTest instances and reports results.
//==============================================================================
int main()
{
    // Initialize JUCE's application base (needed for some JUCE internals)
    juce::ScopedJuceInitialiser_GUI init;
    
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false); // Don't abort on first failure
    
    std::cout << "=== KndlSynth DSP Tests ===" << std::endl;
    std::cout << std::endl;
    
    // Run only KndlSynth tests (not JUCE internal tests)
    runner.runTestsInCategory("KndlSynth");
    
    // Print results
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    
    int totalTests = 0;
    int totalPasses = 0;
    int totalFailures = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult(i);
        if (result != nullptr)
        {
            totalTests++;
            totalPasses += result->passes;
            totalFailures += result->failures;
            
            juce::String status = result->failures > 0 ? "FAIL" : "PASS";
            std::cout << "  [" << status << "] " 
                      << result->unitTestName
                      << " - " << result->passes << " passed";
            
            if (result->failures > 0)
                std::cout << ", " << result->failures << " FAILED";
            
            std::cout << std::endl;
            
            // Print failure messages
            for (const auto& msg : result->messages)
            {
                if (msg.startsWith("FAIL"))
                    std::cout << "         " << msg << std::endl;
            }
        }
    }
    
    std::cout << std::endl;
    std::cout << "Total: " << totalPasses << " passed, " 
              << totalFailures << " failed"
              << " (" << totalTests << " test classes)" << std::endl;
    
    return totalFailures > 0 ? 1 : 0;
}
