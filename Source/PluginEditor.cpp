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
    delaySlider.setSkewFactor(0.35);
    delaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    delaySlider.setValue(0.0);

    delaySlider.addListener(this);
    addAndMakeVisible(&delaySlider);

    delayLabel.setText("d",juce::dontSendNotification);
    delayLabel.attachToComponent(&delaySlider, false);
    delayLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(delayLabel);

    exponentSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    exponentSlider.setRange(0.0, 3.0);
    exponentSlider.setSkewFactorFromMidPoint(1.0);
    exponentSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    exponentSlider.setValue(1.0);

    exponentSlider.addListener(this);
    addAndMakeVisible(&exponentSlider);

    exponentLabel.setText("exp", juce::dontSendNotification);
    exponentLabel.attachToComponent(&exponentSlider, false);
    exponentLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(exponentLabel);

    volSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    volSlider.setRange(0.0, 4.0, 0.0);
    volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    volSlider.setSkewFactorFromMidPoint(1);
    volSlider.setValue(1.0);

    volSlider.addListener(this);
    addAndMakeVisible(&volSlider);

    volLabel.setText("vol", juce::dontSendNotification);
    volLabel.attachToComponent(&volSlider, false);
    volLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(volLabel);


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
    volSlider.setBounds(300, 50, 30, getHeight() - 60);

}
void SelfMultAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    audioProcessor.delayValue = delaySlider.getValue();
    audioProcessor.exponentValue = exponentSlider.getValue();
    audioProcessor.userVolValue = volSlider.getValue();
}
