#pragma once

#include "EffectBlock/EffectBlock.h"
#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

//==============================================================================
// SlotButton: Represents one of the 8 effect slots on the pedalboard grid
//==============================================================================
class SlotButton : public juce::Component {
public:
  SlotButton(size_t idx, std::function<void(size_t)> clickCallback)
      : slotIndex(idx), onClick(std::move(clickCallback)) {}

  void setEffectBlock(EffectBlock *block) {
    if (currentBlock != block) {
      currentBlock = block;
      repaint();
    }
  }

  void paint(juce::Graphics &g) override {
    auto area = getLocalBounds().toFloat().reduced(2.0f);

    if (currentBlock == nullptr) {
      // Empty slot: [+] button
      g.setColour(juce::Colour::fromString("#232833"));
      g.fillRoundedRectangle(area, 12.0f);

      g.setColour(isMouseOver() ? juce::Colour::fromString("#63b3ed")
                                : juce::Colour::fromString("#4a5568"));
      g.drawRoundedRectangle(area, 12.0f, isMouseOver() ? 2.5f : 1.5f);

      g.setColour(isMouseOver() ? juce::Colours::white
                                : juce::Colour::fromString("#a0aec0"));
      g.setFont(36.0f);
      g.drawText("+", getLocalBounds(), juce::Justification::centred);
    } else {
      // Active effect block
      juce::Colour bgColour = juce::Colour::fromString("#2a3242");
      juce::Colour accentColour = juce::Colours::goldenrod;

      const juce::String name = currentBlock->getName();
      if (name == "OverDrive") {
        accentColour = juce::Colour::fromString("#e53e3e"); // Vibrant red
      } else if (name == "Reverb") {
        accentColour = juce::Colour::fromString("#805ad5"); // Purple/Cyan
      } else if (name == "Gain") {
        accentColour = juce::Colour::fromString("#38a169"); // Green
      }

      g.setColour(bgColour);
      g.fillRoundedRectangle(area, 12.0f);

      // Header bar with effect accent color
      auto headerArea = area.removeFromTop(28.0f);
      g.setColour(accentColour.withAlpha(0.85f));
      g.fillRoundedRectangle(headerArea.getX(), headerArea.getY(),
                             headerArea.getWidth(), headerArea.getHeight(),
                             12.0f);
      // Square off bottom corners of header area
      g.fillRect(headerArea.getX(), headerArea.getBottom() - 6.0f,
                 headerArea.getWidth(), 6.0f);

      g.setColour(juce::Colours::white);
      g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
      g.drawText(name.toUpperCase(), headerArea.toNearestInt(),
                 juce::Justification::centred);

      // Draw border
      g.setColour(isMouseOver() ? accentColour.brighter(0.4f) : accentColour);
      g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 12.0f,
                             isMouseOver() ? 2.5f : 1.5f);

      // Center graphic / icon inside the box
      g.setColour(juce::Colours::white.withAlpha(0.9f));
      g.setFont(18.0f);
      if (name == "OverDrive") {
        g.drawText("DRV\nOverDrive", area.toNearestInt(),
                   juce::Justification::centred);
      } else if (name == "Reverb") {
        g.drawText("REV\nReverb", area.toNearestInt(),
                   juce::Justification::centred);
      } else if (name == "Gain") {
        g.drawText("GAIN\nBoost", area.toNearestInt(),
                   juce::Justification::centred);
      } else {
        g.drawText(name, area.toNearestInt(), juce::Justification::centred);
      }
    }
  }

  void mouseEnter(const juce::MouseEvent &) override { repaint(); }
  void mouseExit(const juce::MouseEvent &) override { repaint(); }
  void mouseUp(const juce::MouseEvent &e) override {
    if (e.mouseWasClicked() && onClick) {
      onClick(slotIndex);
    }
  }

private:
  size_t slotIndex;
  std::function<void(size_t)> onClick;
  EffectBlock *currentBlock = nullptr;
};

//==============================================================================
// AddTypeButton: Interactive card component for choosing an effect to insert
//==============================================================================
class AddTypeButton : public juce::Component {
public:
  AddTypeButton(const juce::String &title, const juce::String &subtitle,
                juce::Colour accent, std::function<void()> clickCallback)
      : nameText(title), descText(subtitle), accentColour(accent),
        onClick(std::move(clickCallback)) {}

  void paint(juce::Graphics &g) override {
    auto area = getLocalBounds().toFloat().reduced(4.0f);
    g.setColour(juce::Colour::fromString("#252b37"));
    g.fillRoundedRectangle(area, 14.0f);

    auto topBar = area.removeFromTop(38.0f);
    g.setColour(accentColour.withAlpha(0.85f));
    g.fillRoundedRectangle(topBar.getX(), topBar.getY(), topBar.getWidth(),
                           topBar.getHeight(), 14.0f);
    g.fillRect(topBar.getX(), topBar.getBottom() - 8.0f, topBar.getWidth(),
               8.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    g.drawText(nameText, topBar.toNearestInt(), juce::Justification::centred);

    g.setColour(isMouseOver() ? accentColour.brighter(0.4f) : accentColour);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 14.0f,
                           isMouseOver() ? 3.0f : 1.5f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(14.0f);
    g.drawFittedText(descText, area.toNearestInt().reduced(12),
                     juce::Justification::centred, 4);
  }

  void mouseEnter(const juce::MouseEvent &) override { repaint(); }
  void mouseExit(const juce::MouseEvent &) override { repaint(); }
  void mouseUp(const juce::MouseEvent &e) override {
    if (e.mouseWasClicked() && onClick) {
      onClick();
    }
  }

private:
  juce::String nameText;
  juce::String descText;
  juce::Colour accentColour;
  std::function<void()> onClick;
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent {
public:
  MainComponent();
  ~MainComponent() override;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  enum class ViewMode { chainView, addEffectView, editEffectView };

  void showView(ViewMode newMode);

private:
  void addEffectToSlot(size_t slotIdx, std::unique_ptr<EffectBlock> newBlock);
  void deleteEffectInSlot(size_t slotIdx);

  juce::CriticalSection audioLock;
  double currentSampleRate = 44100.0;
  int currentSamplesPerBlockExpected = 2048;

  static constexpr size_t MAX_SLOTS = 8;
  std::vector<std::unique_ptr<EffectBlock>> effectChain;

  ViewMode currentViewMode = ViewMode::chainView;
  size_t selectedSlotIndex = 0;

  // Top Bar Controls
  juce::TextButton settingsButton{"Audio Settings"};
  enum class OutputMode { stereo, mono };
  std::atomic<OutputMode> currentOutputMode{OutputMode::stereo};
  juce::Label modeLabel;
  juce::ComboBox modeSelector;

  // Chain View Controls
  juce::Label chainTitleLabel;
  std::vector<std::unique_ptr<SlotButton>> slotButtons;

  // Add Effect View Controls
  juce::Label addTitleLabel;
  std::unique_ptr<AddTypeButton> addGainButton;
  std::unique_ptr<AddTypeButton> addOverDriveButton;
  std::unique_ptr<AddTypeButton> addReverbButton;

  // Edit Effect View Controls
  juce::Label editTitleLabel;
  std::unique_ptr<juce::Component> currentEditorComponent;

  // Common Buttons for Add / Edit
  juce::TextButton backButton{"Back"};
  juce::TextButton deleteButton{"Delete Effect"};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
