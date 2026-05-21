#include <JuceHeader.h>

#include "BinauralConvolverTests.h"
#include "IRLoaderTests.h"
#include "LayoutPresetTests.h"
#include "MchBinauralRendererTests.h"
#include "MchDryRoutingTests.h"
#include "MchSlotIrManagerTests.h"
#include "ParameterTests.h"

#include <iostream>

int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    int totalFailures = 0;

    for (int index = 0; index < runner.getNumResults(); ++index)
    {
        if (const auto* result = runner.getResult(index))
        {
            totalFailures += result->failures;

            if (result->failures > 0)
            {
                std::cout << "[FAILED] " << result->unitTestName << " :: " << result->subcategoryName << "\n";
                for (int messageIndex = 0; messageIndex < result->messages.size(); ++messageIndex)
                    std::cout << "  - " << result->messages[messageIndex] << "\n";
            }
            else
            {
                std::cout << "[PASSED] " << result->unitTestName << " :: " << result->subcategoryName << "\n";
                for (int messageIndex = 0; messageIndex < result->messages.size(); ++messageIndex)
                    std::cout << "  " << result->messages[messageIndex] << "\n";
            }
        }
    }

    return totalFailures == 0 ? 0 : 1;
}