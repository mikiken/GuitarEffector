#pragma once

#include "EffectBlock/EffectBlock.h"
#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent {
public:
  //==============================================================================
  MainComponent();
  ~MainComponent() override;

  //==============================================================================
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  //==============================================================================
  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  //==============================================================================
  // Button to audio settings screen
  juce::TextButton settingsButton{"Audio Settings"};

  // List of effects to store in order
  std::vector<std::unique_ptr<EffectBlock>> effectChain;

  // List of GUI editors for each effect in the chain
  std::vector<std::unique_ptr<juce::Component>> effectEditors;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
