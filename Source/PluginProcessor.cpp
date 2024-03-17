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

    //setting 0.5sec as maxDelay, should be more than enough
    int maxDelayInSamples = 0.5 * sampleRate;

    delayBuffer = juce::AudioBuffer<float>(totalNumInputChannels, maxDelayInSamples);
    delayBuffer.clear();
    delayBufferWriteIndex = 0;

    rmsWindowLength = ceil(1.0 / 60 * sampleRate); // at least 1 full wave while expecting 60Hz as lowest frequency
    rmsBuffer = juce::AudioBuffer<float>(totalNumInputChannels, rmsWindowLength);
    rmsBuffer.clear();
    rmsBufferIndex = std::vector<int>(totalNumInputChannels, 0);
    rmsSum = std::vector <float>(totalNumInputChannels, 0);

    rmsVolumeCoefs = std::vector<std::vector<float> >(totalNumInputChannels,std::vector<float>(samplesPerBlock, 0));

    softAttackWindowLength = rmsWindowLength;
    softAttackWindow = std::vector<float>(softAttackWindowLength);
    float softAttackNormalized;//from 0 to 1
    for (int i = 0; i < softAttackWindowLength; i++)
    {
        softAttackNormalized = static_cast<float>(i) / softAttackWindowLength;

        //a window i chose with start and end at 1 and rapid fall at beginning
        softAttackWindow[i] = 1 / (100 * softAttackNormalized + 1) + 0.9901 * softAttackNormalized * softAttackNormalized;
    }


    softAttackProgress = std::vector<int>(totalNumInputChannels, 0);
    softAttackMaxRise = std::vector<float>(totalNumInputChannels, 0);
    softAttackMaxRiseIndex = std::vector<int>(totalNumInputChannels, 0);
    softAttackInProgress = std::vector<bool>(totalNumInputChannels, false);

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
    //juce::ScopedNoDenormals noDenormals;
    int totalNumInputChannels = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();

    int blockSize = buffer.getNumSamples();

    //to avoid garbage in case more outputs than inputs
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, blockSize);


    writeToDelayBuffer(buffer);

    calcRmsVolumeCoefs(buffer, blockSize);

    int negative;
    float delaySample;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        auto* delayData = delayBuffer.getReadPointer(channel);

        for (int sample = 0; sample < blockSize; sample++)
        {

            delaySample = delayData[delayBufferReadIndex + sample - ( (delayBufferReadIndex + sample >= delayBuffer.getNumSamples()) ) * delayBuffer.getNumSamples()];

            //checking if result should be negative or positive as we have to use the absolute value in the power function. (e.g. -2^2.5 cant be computed)
            negative = channelData[sample]*delaySample < 0 ? -1 : 1;

            channelData[sample] = channelData[sample] * pow(abs(delaySample),exponentValue) * rmsVolumeCoefs[channel][sample] * userVolValue * negative;


        }
    }
}


void SelfMultAudioProcessor::writeToDelayBuffer(juce::AudioBuffer<float>& buffer)
{
    int blockSize = buffer.getNumSamples();
    bool wrap;
    int nSamples;

    int delayInSamples = delayValue / 1000.0 * getSampleRate();   // todo maybe change to float with interpolation

    delayBufferReadIndex = delayBufferWriteIndex - delayInSamples;
    if (delayBufferReadIndex < 0)
    {
        delayBufferReadIndex += delayBuffer.getNumSamples();
    }



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

    delayBufferWriteIndex = wrap ? blockSize - nSamples : delayBufferWriteIndex+blockSize;
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

void SelfMultAudioProcessor::calcRmsVolumeCoefs(juce::AudioBuffer<float>& input, int blockSize)
{
    for (int channel = 0; channel < rmsVolumeCoefs.size(); channel++)
    {
        auto* channelData = input.getReadPointer(channel);
        auto* rmsData = rmsBuffer.getWritePointer(channel);
        float softAttackFactor;
        for (int i = 0; i < blockSize; i++)
        {
            //sum for rms
            rmsSum[channel] -= rmsData[rmsBufferIndex[channel]];
            rmsData[rmsBufferIndex[channel]] = pow(channelData[i], 2);
            rmsSum[channel] += rmsData[rmsBufferIndex[channel]];

            if (!softAttackInProgress[channel]) {
                checkSoftAttackTrigger(channel, rmsData);
            }

            softAttackFactor = getSoftAttackFactor(channel);
            //if volume too low return 0 to avoid having a near inf factor
            if (rmsSum[channel] < 0.0001)
            {
                rmsVolumeCoefs[channel][i] = 0;
            }
            else
            {
                rmsVolumeCoefs[channel][i] = softAttackFactor / pow(std::sqrt(rmsSum[channel] / getSampleRate()) / std::sqrt(rmsWindowLength / (2 * getSampleRate())), exponentValue);
            }

            if (++rmsBufferIndex[channel] >= rmsWindowLength)
            {
                rmsBufferIndex[channel] -= rmsWindowLength;
            }

        }

    }

}


float SelfMultAudioProcessor::getSoftAttackFactor(int channel)
{
    if (softAttackProgress[channel] >= softAttackWindowLength-1) {
        softAttackInProgress[channel] = false;
    }
    return softAttackInProgress[channel] ? pow(softAttackWindow[softAttackProgress[channel]++],exponentValue) : 1;
}

void SelfMultAudioProcessor::activateSoftAttack(int channel)
{
    if (!softAttackInProgress[channel] && rmsSum[channel]>0.0001) {

        softAttackInProgress[channel] = true;
        softAttackProgress[channel] = 0;
    }

}

void SelfMultAudioProcessor::checkSoftAttackTrigger(int channel, float* rmsData)
{
    //auto* rmsData = rmsBuffer.getReadPointer(channel);
    float diff;
    if (rmsBufferIndex[channel] == 0) {
        diff = rmsData[0] - rmsData[rmsWindowLength - 1];
    }
    else {
        diff = rmsData[rmsBufferIndex[channel]] - rmsData[rmsBufferIndex[channel] - 1];
    }

    //when maxRise got replaced by new sample find new maxRise, except new sample
    if (rmsBufferIndex[channel] == softAttackMaxRiseIndex[channel])
    {
        softAttackMaxRise[channel] = 0;
        int lastRmsIndex = rmsWindowLength - 1;
        for (int rmsIndex = 0; rmsIndex < rmsWindowLength; rmsIndex++)
        {
            if (rmsIndex != rmsBufferIndex[channel] && lastRmsIndex != rmsBufferIndex[channel])
            {
                if (softAttackMaxRise[channel] < rmsData[rmsIndex] - rmsData[lastRmsIndex]) {
                    softAttackMaxRise[channel] = rmsData[rmsIndex] - rmsData[lastRmsIndex];
                }
            }

            lastRmsIndex = rmsIndex;
        }
    }

    //trigger if diff from new to last sample is 2x as max in window
    if (softAttackMaxRise[channel] * 1.5 < diff) {
        activateSoftAttack(channel);
    }

    if (softAttackMaxRise[channel] < diff) {
        softAttackMaxRise[channel] = diff;
        softAttackMaxRiseIndex[channel] = rmsBufferIndex[channel];
    }
}
/*

ideas for softer Attack
-trigger if diff from new sample to last is 2x as max in window

-use 1/100*x+1 + x^2 as additional coeffs x:= progress from 0 to 1

-somehow use d value  and exp value in calcs

*/



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SelfMultAudioProcessor();
}
