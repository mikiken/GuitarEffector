#pragma once
#include "EffectBlock.h"
#include <JuceHeader.h>
#include <atomic>

//==============================================================================
// ReverbBlockEditor: GUI Component (View) for ReverbBlock
//==============================================================================
class ReverbBlockEditor : public juce::Component {
public:
  ReverbBlockEditor(std::atomic<float> &roomSizeParam,
                    std::atomic<float> &dampingParam,
                    std::atomic<float> &mixParam)
      : targetRoomSize(roomSizeParam), targetDamping(dampingParam),
        targetMix(mixParam) {
    // Room Size Slider Setup
    roomSizeSlider.setRange(0.0, 10.0, 0.1);
    roomSizeSlider.setValue(targetRoomSize.load());
    roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    roomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    roomSizeSlider.onValueChange = [this] {
      targetRoomSize.store(static_cast<float>(roomSizeSlider.getValue()));
    };

    // Damping Slider Setup
    dampingSlider.setRange(0.0, 10.0, 0.1);
    dampingSlider.setValue(targetDamping.load());
    dampingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dampingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    dampingSlider.onValueChange = [this] {
      targetDamping.store(static_cast<float>(dampingSlider.getValue()));
    };

    // Mix Slider Setup
    mixSlider.setRange(0.0, 10.0, 0.1);
    mixSlider.setValue(targetMix.load());
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    mixSlider.onValueChange = [this] {
      targetMix.store(static_cast<float>(mixSlider.getValue()));
    };

    // Labels
    titleLabel.setText("Reverb", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    roomSizeLabel.setText("Room Size", juce::dontSendNotification);
    roomSizeLabel.setJustificationType(juce::Justification::centred);

    dampingLabel.setText("Damping", juce::dontSendNotification);
    dampingLabel.setJustificationType(juce::Justification::centred);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(roomSizeLabel);
    addAndMakeVisible(roomSizeSlider);
    addAndMakeVisible(dampingLabel);
    addAndMakeVisible(dampingSlider);
    addAndMakeVisible(mixLabel);
    addAndMakeVisible(mixSlider);
  }

  void resized() override {
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(40));

    // Divide remaining height equally among the 3 rotary controls
    const int sectionHeight = area.getHeight() / 3;

    auto roomArea = area.removeFromTop(sectionHeight);
    roomSizeLabel.setBounds(roomArea.removeFromTop(20));
    roomSizeSlider.setBounds(roomArea);

    auto dampingArea = area.removeFromTop(sectionHeight);
    dampingLabel.setBounds(dampingArea.removeFromTop(20));
    dampingSlider.setBounds(dampingArea);

    auto mixArea = area;
    mixLabel.setBounds(mixArea.removeFromTop(20));
    mixSlider.setBounds(mixArea);
  }

private:
  std::atomic<float> &targetRoomSize;
  std::atomic<float> &targetDamping;
  std::atomic<float> &targetMix;

  juce::Slider roomSizeSlider;
  juce::Slider dampingSlider;
  juce::Slider mixSlider;

  juce::Label titleLabel;
  juce::Label roomSizeLabel;
  juce::Label dampingLabel;
  juce::Label mixLabel;
};

//==============================================================================
// ReverbBlock: Audio Processing (Model/Processor) using juce::dsp::Reverb
//==============================================================================
class ReverbBlock : public EffectBlock {
public:
  ReverbBlock() { updateParameters(); }

  std::atomic<float> roomSize{5.0f};
  std::atomic<float> damping{5.0f};
  std::atomic<float> mix{3.5f};

  void prepareToPlay(double sampleRate,
                     int samplesPerBlockExpected = 2048) override {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlockExpected);
    spec.numChannels = 2; // Default for stereo processing

    reverb.prepare(spec);
    reverb.reset();
    updateParameters();
  }

  void process(juce::AudioBuffer<float> &buffer) override {
    updateParameters();

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
  }

  std::unique_ptr<juce::Component> createEditor() override {
    return std::make_unique<ReverbBlockEditor>(roomSize, damping, mix);
  }

  juce::String getName() const override { return "Reverb"; }

private:
  void updateParameters() {
    juce::dsp::Reverb::Parameters params;

    // Map 0.0-10.0 slider ranges to 0.0-1.0 Reverb parameter ranges
    const float roomVal = juce::jlimit(0.0f, 1.0f, roomSize.load() / 10.0f);
    const float dampVal = juce::jlimit(0.0f, 1.0f, damping.load() / 10.0f);
    const float mixVal = juce::jlimit(0.0f, 1.0f, mix.load() / 10.0f);

    params.roomSize = roomVal;
    params.damping = dampVal;
    params.wetLevel = mixVal;
    params.dryLevel = 1.0f - (mixVal * 0.5f);
    params.width = 1.0f;
    params.freezeMode = 0.0f;

    reverb.setParameters(params);
  }

  juce::dsp::Reverb reverb;
};
