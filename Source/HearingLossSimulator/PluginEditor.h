/**
 * \class HLSPluginAudioProcessorEditor
 *
 * \brief Declaration of HLSPluginAudioProcessorEditor interface.
 * \date  November 2020
 *
 * \authors Reactify Music LLP: R. Hrafnkelsson ||
 * Coordinated by , A. Reyes-Lecuona (University of Malaga) and L.Picinali (Imperial College London) ||
 * \b Contact: areyes@uma.es and l.picinali@imperial.ac.uk
 *
 * \b Project: 3DTI (3D-games for TUNing and lEarnINg about hearing aids) ||
 * \b Website: http://3d-tune-in.eu/
 *
 * \b Copyright: University of Malaga and Imperial College London - 2020
 *
 * \b Licence: This copy of the 3D Tune-In Toolkit Plugin is licensed to you under the terms described in the LICENSE.md file included in this distribution.
 *
 * \b Acknowledgement: This project HLS received funding from the European Union's Horizon 2020 research and innovation programme under grant agreements No 644051 and 726765.
 */

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChannelSwitchComponent.h"
#include "NonLinearAttenuatorComponent.h"
#include "TemporalDistortionComponent.h"
#include "FrequencySmearingComponent.h"
#include "Common/AboutBanner.h"
#include "Common/AudiogramComponent.h"

//==============================================================================
/**
 */
class HLSPluginAudioProcessorEditor : public AudioProcessorEditor, public Timer, public AudiogramComponent::Listener {
public:
    HLSPluginAudioProcessorEditor (HLSPluginAudioProcessor&);
    ~HLSPluginAudioProcessorEditor();
    
    //============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
    void timerCallback() override;
    
    void mouseUp (const MouseEvent &e) override {
        aboutText.setVisible(false);
    };
    
    // AudiogramComponent::Listener
    void audiogramPresetSelected (int channel, int index) override;
    void audiogramSliderValueChanged (int channel, int band, double value) override;
    
private:
    HLSPluginAudioProcessor& processor;
    
    TextEditor aboutText;
    AboutBanner aboutBanner;
    
    ChannelSwitchComponent channelSwitchComponent;
    std::unique_ptr<AudiogramComponent> audiogramComponent;
    NonLinearAttenuatorComponent nonLinearAttenuatorComponent;
    TemporalDistortionComponent temporalDistortionComponent;
    FrequencySmearingComponent frequencySmearingComponent;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HLSPluginAudioProcessorEditor)
};
