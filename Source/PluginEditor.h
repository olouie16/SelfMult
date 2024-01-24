/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SelfMultAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Slider::Listener, private juce::Button::Listener
{
public:
    SelfMultAudioProcessorEditor (SelfMultAudioProcessor&);
    ~SelfMultAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SelfMultAudioProcessor& audioProcessor;

    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    juce::Slider delaySlider;
    juce::Label delayLabel;
    juce::Slider exponentSlider;
    juce::Label exponentLabel;
    juce::Slider volSlider;
    juce::Label volLabel;

    juce::TextButton autoVolButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelfMultAudioProcessorEditor)
};
