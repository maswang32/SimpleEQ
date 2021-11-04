/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

//Settings for the Chain
struct ChainSettings
{
    float peakFreq{ 0 };
    float peakGainInDecibels{ 0 };
    float peakQuality{ 1.f };
    float lowCutFreq{ 0 };
    float highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 };
    Slope highCutSlope{ Slope::Slope_12 };
};

//Function to get chain settings
//returns AudioProcessorValueTreeState
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//define chains
//pass in processing contexts, which will run through chain automatically
//aliases are useful for nested namespaces
using Filter = juce::dsp::IIR::Filter<float>;

//Cut filter is made
//using a chain of multiple filters stacked on each other
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

//Can uses filters in notch, peaks, shelfs, etc.
//Monochain is Lowcut -> Parametric -> HighCut, this is the whole signal chain
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

//Enumeration for ease of accessing
//each element in the chain
enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

//Alias for Filter's Coefficients
using Coefficients = Filter::CoefficientsPtr;

//Function to update coefficients
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

//function to make peak filter.
Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

//helper function
//updates coefficients on a chain, unbypasses
//ChainType can be 
template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients,
        coefficients[Index]);
    chain.template setBypassed<Index>(false);
}


//templates have generic types
//updates cutfilter
//refactored so variable names make more sense
template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& cutChain,
    const CoefficientType& cutCoefficients,
    const Slope& lowCutSlope)
{
    //the chain is a processor chain,
    //this bypasses the ith element in the chain
    cutChain.template setBypassed<0>(true);
    cutChain.template setBypassed<1>(true);
    cutChain.template setBypassed<2>(true);
    cutChain.template setBypassed<3>(true);

    switch (lowCutSlope)
    {
    case Slope_48:
    {
        update<3>(cutChain, cutCoefficients);
    }
    case Slope_36:
    {
        update<2>(cutChain, cutCoefficients);
    }
    case Slope_24:
    {
        update<1>(cutChain, cutCoefficients);
    }
    case Slope_12:
    {
        update<0>(cutChain, cutCoefficients);
    }
    }
}


//necessary inline
inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    //arguments: frequency, sample rate, order
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
            sampleRate, 2 * (chainSettings.lowCutSlope + 1));
}


//necessary inline
inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
        sampleRate, 2 * (chainSettings.highCutSlope + 1));
}

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    // audio processor value tree state
    //need all parameters laid out before the tree is created
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };


private:
    //left and right audio chains
    MonoChain leftChain, rightChain;

    //Refactoring using helper functions
    void updatePeakFilter(const ChainSettings& chainSettings);
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateFilters();
 
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};