/*
  ==============================================================================

 JAFFTUNE Real-Time Pitchshifter Powered by Variable-Rate Delay
 Awknowledgments to Karl Yerkes, Miller Puckette, Jazer Sibley-Schwartz, @dude837 on YouTube, @The Audio Programmer on YouTube, @MatKatMusic on YouTube
 
 YET TO IMPLEMENT:
 -Independent Pitch Control per Channel in Stereo Mode
 -Resolve delayWindow to a function of pitchRatio to keep phasorFreq < 5
 -implementation as VST3
 -implementation as AAX for ProTools
 -implementation as AU for Logic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class JafftuneAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    JafftuneAudioProcessor();
    ~JafftuneAudioProcessor() override;

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
    
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();
    juce::AudioProcessorValueTreeState treeState {*this, nullptr, "Parameters", createParameterLayout()};
        
private:
    
    //declare functions
    float scale(float x, float inMin, float inMax, float outMin, float outMax) {
        // Perform linear mapping based on specified input and output ranges
        float scaledValue = ((x - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
        
        return scaledValue;
    }
    
    float msToSamps(float valueInMs) {
        return valueInMs * (getSampleRate() / 1000);
    }
    
    float sampsToMs(float valueInSamps) {
        return valueInSamps * (1000 / getSampleRate());
    }
    
    float dbtoa(float valueIndB) {
        return std::pow(10.0, valueIndB / 20.0);
    }
    
    //onePole from https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
    float onePole (float inputSample, float pastSample, float smoothingTimeInMs, int sampleRate) {
        const int twoPi = 2 * juce::MathConstants<float>::pi;
        float a = exp( -twoPi / (smoothingTimeInMs * 0.001f * sampleRate));
        float b = 1.0f - a;
        float outputSample = (inputSample * b) + (pastSample * a);
        return outputSample;
    };
    
    //initialzie mDelayLine
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> mDelayLineOne { static_cast<int>(getSampleRate())};
    
    //initialzie oscillator gains
    juce::dsp::Gain<float> phasorGain;
    juce::dsp::Gain<float> reversePhasorGain;
   
    //declare global variables
    const float pi = {juce::MathConstants<float>::pi};
    float delayTime = { 0.0f };
    float delayWindow = { 22.0f };
    float pitchRatio = { 1.0f };
    float globalPhasorTap = { 0.0f };
    float lastDelayTimeOne = { 0.0f };
    float lastDelayTimeTwo = { 0.0f };

    //sawtooth oscillator -> replicates "phasor~"
    juce::dsp::Oscillator<float> phasor { [](float x) { return ((x / juce::MathConstants<float>::pi) + 1.0f) / 2.0f; }};
    
    //reverse sawtooth oscillator -> replicates "phasor~" when input freq is negative
    juce::dsp::Oscillator<float> reversePhasor { [](float x) { return ((x / -juce::MathConstants<float>::pi) + 1.0f) / 2.0f; }};
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JafftuneAudioProcessor)
};
