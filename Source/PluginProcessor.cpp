/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SelfMultAudioProcessor::SelfMultAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SelfMultAudioProcessor::~SelfMultAudioProcessor()
{
}

//==============================================================================
const juce::String SelfMultAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SelfMultAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SelfMultAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SelfMultAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SelfMultAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SelfMultAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int SelfMultAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SelfMultAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SelfMultAudioProcessor::getProgramName(int index)
{
    return {};
}

void SelfMultAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SelfMultAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    int totalNumInputChannels = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();

    DBG("totalNumInputChannels: " + std::to_string(totalNumInputChannels));
    DBG("totalNumOutputChannels: " + std::to_string(totalNumOutputChannels));

    //setting 0.5sec as maxDelay, should be more than enough
    int maxDelayInSamples = 0.5 * sampleRate;

    delayBuffer = juce::AudioBuffer<float>(totalNumInputChannels, maxDelayInSamples);
    delayBuffer.clear();
    delayBufferWriteIndex = 0;
}

void SelfMultAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SelfMultAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void SelfMultAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    int totalNumInputChannels = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();


    double sampleRate = getSampleRate();

    //hardcoded 100ms delay for the moment, userInput later
    int delayInSamples = delayValue / 1000.0 * sampleRate;   // todo change to float with interpolation
    float delaySample;

    delayBufferReadIndex = delayBufferWriteIndex - delayInSamples;
    if (delayBufferReadIndex < 0)
    {
        delayBufferReadIndex += delayBuffer.getNumSamples();
    }

    //to avoid garbage in case more outputs than inputs
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    writeToDelayBuffer(buffer);


    //adjustAutoVol
    if (adjustingAutoVol)
    {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); sample++)
            {
                expectedMaxAmp = std::max(expectedMaxAmp, std::abs(channelData[sample]));
            }
        }

        if (juce::Time::currentTimeMillis() - adjustingAutoVolStart > 2000)//after 2sec
        {
            adjustingAutoVol = false;
            updateAutoVolValue();
            adjustingAutoVol = false;

        }
    }



    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);


        int negative;
        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {

            delaySample = delayBuffer.getSample(channel, delayBufferReadIndex + sample - (delayBuffer.getNumSamples() * !(delayBufferReadIndex + sample < delayBuffer.getNumSamples())));

            //checking if result should be negative or positive as we have to use the absolute value in the power function. (e.g. -2^2.5 cant be computed)
            negative = channelData[sample]*delaySample < 0 ? -1 : 1;
            
            channelData[sample] = channelData[sample] * pow(abs(delaySample),exponentValue) * autoVolValue * userVolValue;
            channelData[sample] = negative * channelData[sample];

        }
    }
}


void SelfMultAudioProcessor::writeToDelayBuffer(juce::AudioBuffer<float>& buffer)
{
    int startWriteIndex = delayBufferWriteIndex;
    int blockSize = buffer.getNumSamples();
    bool wrap;
    int nSamples;
    //check if needing to wrap
    if (delayBufferWriteIndex + blockSize >= delayBuffer.getNumSamples())
    {
        wrap = true;
        nSamples = delayBuffer.getNumSamples() - delayBufferWriteIndex;
    }
    else
    {
        wrap = false;
        nSamples = blockSize;
    }

    for (int channel = 0; channel < buffer.getNumChannels(); channel++)
    {
        delayBuffer.copyFrom(channel, delayBufferWriteIndex, buffer.getReadPointer(channel), nSamples);
        if (wrap)
        {
            delayBuffer.copyFrom(channel, 0, buffer.getReadPointer(channel)+nSamples, blockSize-nSamples);
        }
    }

    delayBufferWriteIndex = wrap ? blockSize - nSamples -1 : delayBufferWriteIndex+blockSize;
}


//updates autoVolValue by using expectedMaxAmp and exponentValue 
void SelfMultAudioProcessor::updateAutoVolValue()
{
    //calculating the output of the expected maximal amplitude sample,
    //then the inverse of it so it should normalise it to -1/1
    //in the end multiply with expectedMaxAmp to regain about input level(in praxis probably a bit lower)
    autoVolValue = 1 / pow(expectedMaxAmp, exponentValue + 1) * expectedMaxAmp;
}


//==============================================================================
bool SelfMultAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SelfMultAudioProcessor::createEditor()
{
    return new SelfMultAudioProcessorEditor (*this);
}

//==============================================================================
void SelfMultAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SelfMultAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void SelfMultAudioProcessor::startAdjustingAutoVol()
{
    adjustingAutoVol = true;
    adjustingAutoVolStart = juce::Time::currentTimeMillis();
    expectedMaxAmp = 0;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SelfMultAudioProcessor();
}
