#pragma once

#include <JuceHeader.h>

class EffectBlock {
public:
  virtual ~EffectBlock() = default;

  // Audio processing interface
  virtual void process(juce::AudioBuffer<float> &buffer) = 0;

  // Create and return a GUI component (editor) dedicated to this effect
  virtual std::unique_ptr<juce::Component> createEditor() = 0;

  // Get effect name (useful for GUI labels etc.)
  virtual juce::String getName() const = 0;
};
