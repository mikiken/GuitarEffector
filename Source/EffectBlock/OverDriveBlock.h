#pragma once
#include "EffectBlock.h"
#include <JuceHeader.h>
#include <atomic>
#include <cmath>
#include <vector>

//==============================================================================
// OverDriveBlockEditor: GUI Component (View) for OverDriveBlock
//==============================================================================
class OverDriveBlockEditor : public juce::Component {
public:
  OverDriveBlockEditor(std::atomic<float> &driveParam,
                       std::atomic<float> &levelParam)
      : targetDrive(driveParam), targetLevel(levelParam) {
    // Drive Slider Setup
    driveSlider.setRange(0.0, 10.0, 0.1);
    driveSlider.setValue(targetDrive.load());
    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    driveSlider.onValueChange = [this] {
      targetDrive.store(static_cast<float>(driveSlider.getValue()));
    };

    // Level Slider Setup
    levelSlider.setRange(0.0, 10.0, 0.1);
    levelSlider.setValue(targetLevel.load());
    levelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    levelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    levelSlider.onValueChange = [this] {
      targetLevel.store(static_cast<float>(levelSlider.getValue()));
    };

    // Labels
    titleLabel.setText("OverDrive", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);

    levelLabel.setText("Level", juce::dontSendNotification);
    levelLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(driveLabel);
    addAndMakeVisible(driveSlider);
    addAndMakeVisible(levelLabel);
    addAndMakeVisible(levelSlider);
  }

  void resized() override {
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(100));

    // Divide remaining height equally between Drive and Level controls
    auto driveArea = area.removeFromTop(area.getHeight() / 2);
    driveLabel.setBounds(driveArea.removeFromTop(20));
    driveSlider.setBounds(driveArea);

    levelLabel.setBounds(area.removeFromTop(20));
    levelSlider.setBounds(area);
  }

private:
  std::atomic<float> &targetDrive;
  std::atomic<float> &targetLevel;

  juce::Slider driveSlider;
  juce::Slider levelSlider;

  juce::Label titleLabel;
  juce::Label driveLabel;
  juce::Label levelLabel;
};

//==============================================================================
// FirstOrderFilter: 1st-order IIR filter (HPF / LPF) for tone shaping
//==============================================================================
class FirstOrderFilter {
public:
  enum class Type { LowPass, HighPass };

  void setCoefficients(Type type, float cutoffHz, float sampleRate) {
    if (sampleRate <= 0.0f)
      return;
    const float pi = juce::MathConstants<float>::pi;
    float K = std::tan(pi * juce::jlimit(10.0f, sampleRate * 0.49f, cutoffHz) /
                       sampleRate);

    if (type == Type::LowPass) {
      b0 = K / (1.0f + K);
      b1 = b0;
      a1 = (K - 1.0f) / (1.0f + K);
    } else {
      b0 = 1.0f / (1.0f + K);
      b1 = -b0;
      a1 = (K - 1.0f) / (1.0f + K);
    }
  }

  void reset() { std::fill(z1.begin(), z1.end(), 0.0f); }

  float processSample(int channel, float input) {
    if (channel < 0 || channel >= static_cast<int>(z1.size()))
      return input;

    const size_t chIdx = static_cast<size_t>(channel);
    float output = b0 * input + z1[chIdx];
    z1[chIdx] = b1 * input - a1 * output;
    return output;
  }

  void ensureNumChannels(int numChannels) {
    if (numChannels >= 0 && static_cast<size_t>(numChannels) != z1.size()) {
      z1.resize(static_cast<size_t>(numChannels), 0.0f);
    }
  }

private:
  float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f;
  std::vector<float> z1{0.0f, 0.0f};
};

//==============================================================================
// OverDriveBlock: Audio Processing (Model/Processor) for OD-1 OverDrive
//==============================================================================
class OverDriveBlock : public EffectBlock {
public:
  OverDriveBlock() { updateFilters(); }

  std::atomic<float> drive{5.0f};
  std::atomic<float> level{5.0f};

  void prepareToPlay(double sampleRate) override {
    currentSampleRate = static_cast<float>(sampleRate);
    updateFilters();
    preClipFilter.reset();
    postClipFilter.reset();
    dcBlocker.reset();
  }

  void process(juce::AudioBuffer<float> &buffer) override {
    const float currentDrive = drive.load();
    const float currentLevel = level.load();

    // 1. Calculate linear gain from OVERDRIVE pot (1MB) + R24 (33k) / R1 (4.7k)
    // Gain ranges from ~8.0x (min drive) to ~220.8x (max drive)
    const float driveGain =
        1.0f + (33.0f + 1000.0f * (currentDrive / 10.0f)) / 4.7f;

    // 2. Calculate output volume level gain from VOL pot (10KB)
    const float levelGain = (currentLevel / 10.0f) * 1.5f;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    preClipFilter.ensureNumChannels(numChannels);
    postClipFilter.ensureNumChannels(numChannels);
    dcBlocker.ensureNumChannels(numChannels);

    for (int ch = 0; ch < numChannels; ++ch) {
      float *channelData = buffer.getWritePointer(ch);

      for (int i = 0; i < numSamples; ++i) {
        float x = channelData[i];

        // Step A: Pre-clipping High-Pass Filter (~720.5 Hz)
        // Models R1 (4.7k) & C3 (0.047uF) in U3 feedback network
        x = preClipFilter.processSample(ch, x);

        // Step B: Apply Overdrive Gain
        x *= driveGain;

        // Step C: Asymmetrical Soft Clipping
        // Models D1 (1N4148) vs D2+D4 (2x 1N4148 in series) in feedback loop
        // Positive threshold ~1.2V (2 diodes), Negative threshold ~0.6V (1
        // diode)
        if (x >= 0.0f) {
          x = 1.2f * std::tanh(x / 1.2f);
        } else {
          x = 0.6f * std::tanh(x / 0.6f);
        }

        // Step D: Post-clipping Low-Pass Filter (~884.2 Hz)
        // Models R9 (10k) & C6 (0.018uF) in U1 active tone filter
        x = postClipFilter.processSample(ch, x);

        // Step E: DC Blocking Filter (~10 Hz)
        // Models C9 (0.047uF) & C10 (1uF) coupling caps removing DC offset
        x = dcBlocker.processSample(ch, x);

        // Step F: Output Level Control
        x *= levelGain;

        channelData[i] = x;
      }
    }
  }

  std::unique_ptr<juce::Component> createEditor() override {
    return std::make_unique<OverDriveBlockEditor>(drive, level);
  }

  juce::String getName() const override { return "OverDrive"; }

private:
  void updateFilters() {
    // Cutoff frequencies derived from BOSS OD-1 schematic:
    // Pre-clip HPF: f_c = 1 / (2 * pi * 4.7k * 0.047uF) ≈ 720.5 Hz
    preClipFilter.setCoefficients(FirstOrderFilter::Type::HighPass, 720.5f,
                                  currentSampleRate);

    // Post-clip LPF: f_c = 1 / (2 * pi * 10k * 0.018uF) ≈ 884.2 Hz
    postClipFilter.setCoefficients(FirstOrderFilter::Type::LowPass, 884.2f,
                                   currentSampleRate);

    // DC Blocker: High-pass filter at 10 Hz to remove DC bias from asymmetrical
    // clipping
    dcBlocker.setCoefficients(FirstOrderFilter::Type::HighPass, 10.0f,
                              currentSampleRate);
  }

  float currentSampleRate = 44100.0f;
  FirstOrderFilter preClipFilter;
  FirstOrderFilter postClipFilter;
  FirstOrderFilter dcBlocker;
};
