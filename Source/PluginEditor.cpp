/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SelfMultAudioProcessorEditor::SelfMultAudioProcessorEditor (SelfMultAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    delaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delaySlider.setRange(0.0, 50.0);
    delaySlider.setSkewFactor(0.6);
    delaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    delaySlider.setValue(0.0);

    addAndMakeVisible(&delaySlider);
    delaySlider.addListener(this);

    exponentSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    exponentSlider.setRange(0.0, 5.0);
    exponentSlider.setSkewFactorFromMidPoint(1.0);
    exponentSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    exponentSlider.setValue(1.0);

    addAndMakeVisible(&exponentSlider);
    exponentSlider.addListener(this);

    gainSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    gainSlider.setRange(0.0, 100.0, 0.0);
    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    gainSlider.setSkewFactorFromMidPoint(5);
    gainSlider.setValue(1.0);

    addAndMakeVisible(&gainSlider);
    gainSlider.addListener(this);

}

SelfMultAudioProcessorEditor::~SelfMultAudioProcessorEditor()
{
}

//==============================================================================
void SelfMultAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SelfMultAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    delaySlider.setBounds(40, 50, 80, 80);
    exponentSlider.setBounds(100, 50, 80, 80);
    gainSlider.setBounds(300, 50, 20, getHeight() - 60);

}
void SelfMultAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    audioProcessor.delayValue = delaySlider.getValue();
    audioProcessor.exponentValue = exponentSlider.getValue();
    audioProcessor.gainValue = gainSlider.getValue();
}