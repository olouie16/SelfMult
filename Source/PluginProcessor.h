/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SelfMultAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SelfMultAudioProcessor();
    ~SelfMultAudioProcessor() override;

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

    float delayValue = 0;
    float exponentValue = 1;
    float userVolValue = 1;

    float mixValue = 1; //not implemented yet

private:
    //==============================================================================

    void writeToDelayBuffer(juce::AudioBuffer<float>& buffer);
    void calcRmsVolumeCoefs(juce::AudioBuffer<float>& input, int blockSize);
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferWriteIndex;
    int delayBufferReadIndex;

    juce::AudioBuffer<float> rmsBuffer;
    std::vector<std::vector<float> > rmsVolumeCoefs;
    float rmsSum;
    int rmsBufferIndex;
    int rmsWindowLength;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelfMultAudioProcessor)
};
