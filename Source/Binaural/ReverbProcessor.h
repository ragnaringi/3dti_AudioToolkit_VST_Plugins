/**
* \class ReverbProcessor
*
* \brief Declaration of Toolkit3dtiProcessor interface.
* \date  June 2019
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

#pragma once

#include <BinauralSpatializer/3DTI_BinauralSpatializer.h>
#include <JuceHeader.h>

//==============================================================================
class ReverbProcessor : private Thread // private AsyncUpdater, 
{
public:
    //==========================================================================
    /***/
    ReverbProcessor (Binaural::CCore& core);
    
    /***/
    ~ReverbProcessor();
    
    /***/
    void setup (double sampleRate, int samplesPerBlock);
    
    //==========================================================================
    /***/
    void process (AudioBuffer<float>& buffer);
    
    /***/
    void process (AudioBuffer<float>& quadIn, AudioBuffer<float>& stereoOut);
    
    //==========================================================================
    /***/
    bool loadBRIR (const File& file);
    
    /***/
    // std::function<void()> didReloadBRIR;
    
    /***/
    const File&     getBrirPath() const { return mBRIRPath;  }
    
    /***/
    float           getPower()     const { return mPower; };
    
    //==========================================================================
    /** Public parameters */
    AudioParameterBool  reverbEnabled;
    AudioParameterFloat reverbLevel;               // ranges from -30 to +6 dB
    AudioParameterFloat reverbDistanceAttenuation; // ranges from -6 to 0 dB
    AudioParameterInt   reverbBRIR;                // ranges from 0 to 6
    
    /** */
    StringArray getBrirOptions() const
    {
        return {"Small", "Medium", "Large", "Library", "Trapezoid", "Load 3DTI", "Load SOFA"};
    }
    
private:
    //==========================================================================
    /** Thread */
    void run() override;
    
    bool doLoadBRIR (const File& file);
    
    double getSampleRate();
    
    std::atomic<bool> isLoading {false};
    
    //==========================================================================
    Binaural::CCore& mCore;
    std::shared_ptr<Binaural::CEnvironment> mEnvironment;
    
    Array<File, CriticalSection> mBRIRsToLoad;
    
    File mBRIRPath;
    
    float mPower;
};
