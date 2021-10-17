/**
* \class ReverbProcessor
*
* \brief Declaration of Toolkit3dtiProcessor interface.
* \date  October 2021
*
* \authors Reactify Music LLP: R. Hrafnkelsson ||
* Coordinated by , A. Reyes-Lecuona (University of Malaga) and L.Picinali (Imperial College London) ||
* \b Contact: areyes@uma.es and l.picinali@imperial.ac.uk
*
* \b Project: 3DTI (3D-games for TUNing and lEarnINg about hearing aids) ||
* \b Website: http://3d-tune-in.eu/
*
* \b Copyright: University of Malaga and Imperial College London - 2021
*
* \b Licence: This copy of the 3D Tune-In Toolkit Plugin is licensed to you under the terms described in the LICENSE.md file included in this distribution.
*
* \b Acknowledgement: This project has received funding from the European Union's Horizon 2020 research and innovation programme under grant agreements No 644051 and 726765.
*/

#include <BRIR/BRIRFactory.h>
#include <BRIR/BRIRCereal.h>
#include "Utils.h"
#include "ReverbProcessor.h"

//==============================================================================
ReverbProcessor::ReverbProcessor (Binaural::CCore& core)
  :  Thread ("ReverbProcessor")
  ,  reverbEnabled ("Reverb Enabled", "Reverb Enabled", true)
  ,  reverbLevel ("Reverb Level", "Reverb Level", NormalisableRange<float> (-30.f, 6.f, 0.1f), -3.f)
  ,  reverbDistanceAttenuation ("Reverb Distance Attenuation", "Reverb Distance Attenuation", NormalisableRange<float> (-6.f, 0.f, 0.1f), -3.f)
  ,  reverbBRIR ("Reverb BRIR", "Reverb BRIR", 0, 6, 0)
  ,  mCore (core)
{
    // Environment setup
    mEnvironment = core.CreateEnvironment();
    mEnvironment->SetReverberationOrder (TReverberationOrder::BIDIMENSIONAL);
}

ReverbProcessor::~ReverbProcessor()
{
    stopThread (500);
}

//==============================================================================
void ReverbProcessor::setup (double sampleRate, int samplesPerBlock)
{
    loadBRIR (getBundledBRIR (reverbBRIR.get(), sampleRate));
}

//==============================================================================
void ReverbProcessor::process (AudioBuffer<float>& buffer)
{
    if (isLoading.load() || ! reverbEnabled.get())
    {
        buffer.clear();
        return;
    }
    
    auto magnitudes = mCore.GetMagnitudes();
    magnitudes.SetReverbDistanceAttenuation (reverbDistanceAttenuation);
    mCore.SetMagnitudes (magnitudes);
    
    Common::CEarPair<CMonoBuffer<float>> outputBuffer;
    mEnvironment->ProcessVirtualAmbisonicReverb (outputBuffer.left,
                                                 outputBuffer.right);
    
    // If BRIR is not loaded, buffer will be set to zero
    if (outputBuffer.left.size() == 0)
    {
        buffer.clear();
        return;
    }
    
    // Fill the output with processed audio
    // Incoming buffer should have two channels
    // for spatialised audio
    jassert (buffer.getNumChannels() >= 2);
    
    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        buffer.getWritePointer(0)[i] = outputBuffer.left[i];
        buffer.getWritePointer(1)[i] = outputBuffer.right[i];
    }
    
    auto reverbGain  = Decibels::decibelsToGain (reverbLevel.get());
    buffer.applyGain (reverbGain);
    
    mPower = outputBuffer.left.GetPower();
}

void ReverbProcessor::process (AudioBuffer<float>& quadIn, AudioBuffer<float>& stereoOut)
{
    if (isLoading.load() || !reverbEnabled.get())
    {
        stereoOut.clear();
        return;
    }
    
    jassert (quadIn.getNumChannels() == 4);
    jassert (stereoOut.getNumChannels()  >= 2);
    
    auto magnitudes = mCore.GetMagnitudes();
    magnitudes.SetReverbDistanceAttenuation (reverbDistanceAttenuation);
    mCore.SetMagnitudes (magnitudes);
    
    int numSamples = stereoOut.getNumSamples();
    
    CMonoBuffer<float>   input (numSamples);
    CStereoBuffer<float> outputBuffer;
    
    for (int ch = 0; ch < quadIn.getNumChannels(); ch++)
    {
        // Fill input buffer with incoming audio
        std::memcpy (input.data(), quadIn.getReadPointer (ch), numSamples*sizeof(float));
        
        mEnvironment->ProcessEncodedChannelReverb (TBFormatChannel(ch), input, outputBuffer);
        
        // If BRIR is not loaded, buffer will be set to zero
        if (outputBuffer.size() == 0)
        {
            stereoOut.clear();
            return;
        }
        
        // Fill the output with processed audio
        jassert (outputBuffer.GetNChannels() == 2);
        
        numSamples = (int)outputBuffer.GetNsamples();
        
        jassert (numSamples == stereoOut.getNumSamples());
        
        for (int i = 0; i < numSamples; i++)
        {
            stereoOut.getWritePointer(0)[i] += outputBuffer.GetMonoChannel (0)[i];
            stereoOut.getWritePointer(1)[i] += outputBuffer.GetMonoChannel (1)[i];
        }
    }
    
    auto reverbGain = Decibels::decibelsToGain (reverbLevel.get());
    stereoOut.applyGain (reverbGain);
    
    mPower = stereoOut.getRMSLevel (0, 0, numSamples);
}

//==============================================================================
void ReverbProcessor::run()
{
    if (mBRIRsToLoad.size() > 0 && ! isLoading.load())
    {
        doLoadBRIR (mBRIRsToLoad.removeAndReturn (0));
        
        if (mBRIRsToLoad.isEmpty())
            signalThreadShouldExit();
    }
    
    sleep (100);
}

//==============================================================================
bool ReverbProcessor::loadBRIR (const File& file)
{
    if (file == mBRIRPath)
        return false;
    
    mBRIRsToLoad.clearQuick();
    
    mBRIRsToLoad.add (file);

    if (! isThreadRunning())
        startThread();
    
    return true;
}

bool ReverbProcessor::doLoadBRIR (const File& file)
{
    isLoading.store (true);
    
    DBG ("Loading BRIR: " << file.getFullPathName());
    
    if (! file.existsAsFile())
    {
        DBG ("BRIR file doesn't exist");
        isLoading.store (false);
        return false;
    }
    
    int fileSampleRate = checkResourceSampleRate (file, false);
    // TODO: Throw exception / return error and trigger warning from editor
    if (fileSampleRate != getSampleRate())
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          "Wrong sample rate",
                                          "Please select a file that matches the project sample rate",
                                          "OK");
        isLoading.store (false);
        return false;
    }
    
    bool isSofa = isSofaFile (file);
    auto path   = file.getFullPathName().toStdString();
    
    bool success = false;
    
    if (isSofa ? BRIR::CreateFromSofa (path, mEnvironment)
               : BRIR::CreateFrom3dti (path, mEnvironment))
    {
        mBRIRPath  = file;
        
        success = true;
    }
    
    isLoading.store (false);
    
    return success;
}

//==============================================================================
double ReverbProcessor::getSampleRate()
{
    return mCore.GetAudioState().sampleRate;
}
