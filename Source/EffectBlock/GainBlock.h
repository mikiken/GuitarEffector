#pragma once
#include "EffectBlock.h"
#include <JuceHeader.h>

//==============================================================================
// GainBlockEditor: GUI Component (View) for GainBlock
//==============================================================================
class GainBlockEditor : public juce::Component {
public:
  GainBlockEditor(std::atomic<float> &gainParam) : targetGain(gainParam) {
    gainSlider.setRange(0.0, 10.0);
    gainSlider.setValue(targetGain.load());
    gainSlider.setSkewFactorFromMidPoint(3.0);
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);

    gainSlider.onValueChange = [this] {
      targetGain.store(static_cast<float>(gainSlider.getValue()));
    };

    addAndMakeVisible(gainSlider);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Gain", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
  }

  void resized() override {
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(24));
    gainSlider.setBounds(area);
  }

private:
  std::atomic<float> &targetGain;
  juce::Slider gainSlider;
  juce::Label titleLabel;
};

//==============================================================================
// GainBlock: Audio Processing (Model/Processor)
//==============================================================================
class GainBlock : public EffectBlock {
public:
  std::atomic<float> gain{3.0f};

  void process(juce::AudioBuffer<float> &buffer) override {
    // Safely load atomic gain value once per block
    const float currentGain = gain.load();
    buffer.applyGain(currentGain);
  }

  std::unique_ptr<juce::Component> createEditor() override {
    return std::make_unique<GainBlockEditor>(gain);
  }

  juce::String getName() const override { return "Gain"; }
};