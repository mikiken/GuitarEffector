#include "MainComponent.h"
#include "EffectBlock/GainBlock.h"
#include "EffectBlock/OverDriveBlock.h"
#include "EffectBlock/ReverbBlock.h"

//==============================================================================
MainComponent::MainComponent() {
  if (juce::RuntimePermissions::isRequired(
          juce::RuntimePermissions::recordAudio) &&
      !juce::RuntimePermissions::isGranted(
          juce::RuntimePermissions::recordAudio)) {
    juce::RuntimePermissions::request(
        juce::RuntimePermissions::recordAudio,
        [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
  } else {
    setAudioChannels(2, 2);
  }

  // Initialize 8 slots with two default pedals
  effectChain.resize(MAX_SLOTS);
  effectChain[0] = std::make_unique<OverDriveBlock>();
  effectChain[1] = std::make_unique<ReverbBlock>();

  // Top Bar Controls setup
  settingsButton.onClick = [this] {
    juce::DialogWindow::LaunchOptions options;
    auto *selector = new juce::AudioDeviceSelectorComponent(
        deviceManager, 0, 256, 0, 256, false, false, true, false);
    selector->setSize(500, 270);
    options.content.setOwned(selector);
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour =
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.launchAsync();
  };
  addAndMakeVisible(settingsButton);

  modeLabel.setText("Output Mode:", juce::dontSendNotification);
  modeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(modeLabel);

  modeSelector.addItem("Stereo Mode", 1);
  modeSelector.addItem("Mono Mode", 2);
  modeSelector.setSelectedId(1, juce::dontSendNotification);
  modeSelector.onChange = [this] {
    if (modeSelector.getSelectedId() == 1)
      currentOutputMode.store(OutputMode::stereo);
    else
      currentOutputMode.store(OutputMode::mono);
  };
  addAndMakeVisible(modeSelector);

  // Chain View Title
  chainTitleLabel.setText(
      "EFFECT CHAIN : CLICK [+] TO ADD EFFECT OR PEDAL TO EDIT",
      juce::dontSendNotification);
  chainTitleLabel.setJustificationType(juce::Justification::centredLeft);
  chainTitleLabel.setFont(
      juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
  chainTitleLabel.setColour(juce::Label::textColourId,
                            juce::Colours::goldenrod);
  addAndMakeVisible(chainTitleLabel);

  // Initialize 8 SlotButtons
  for (size_t i = 0; i < MAX_SLOTS; ++i) {
    auto btn = std::make_unique<SlotButton>(i, [this](size_t idx) {
      selectedSlotIndex = idx;
      if (effectChain[idx] == nullptr) {
        showView(ViewMode::addEffectView);
      } else {
        showView(ViewMode::editEffectView);
      }
    });
    addAndMakeVisible(btn.get());
    slotButtons.push_back(std::move(btn));
  }

  // Add Effect View Controls
  addTitleLabel.setJustificationType(juce::Justification::centred);
  addTitleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
  addTitleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(addTitleLabel);

  addGainButton = std::make_unique<AddTypeButton>(
      "GAIN",
      "Clean Boost & Attenuation\nLinear Level Control\nInput/Output Trim",
      juce::Colour::fromString("#38a169"), [this] {
        addEffectToSlot(selectedSlotIndex, std::make_unique<GainBlock>());
      });
  addAndMakeVisible(addGainButton.get());

  addOverDriveButton = std::make_unique<AddTypeButton>(
      "OVERDRIVE",
      "BOSS OD-1 Modeling\nAsymmetrical Soft Clipping\nPre/Post Tone Filters",
      juce::Colour::fromString("#e53e3e"), [this] {
        addEffectToSlot(selectedSlotIndex, std::make_unique<OverDriveBlock>());
      });
  addAndMakeVisible(addOverDriveButton.get());

  addReverbButton = std::make_unique<AddTypeButton>(
      "REVERB", "Stereo Schroeder-Moorer\nRoom Size, Damping\n& Mix Controls",
      juce::Colour::fromString("#805ad5"), [this] {
        addEffectToSlot(selectedSlotIndex, std::make_unique<ReverbBlock>());
      });
  addAndMakeVisible(addReverbButton.get());

  // Edit Effect View Title
  editTitleLabel.setJustificationType(juce::Justification::centred);
  editTitleLabel.setFont(
      juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
  editTitleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(editTitleLabel);

  // Common Buttons
  backButton.onClick = [this] { showView(ViewMode::chainView); };
  backButton.setColour(juce::TextButton::buttonColourId,
                       juce::Colour::fromString("#4a5568"));
  addAndMakeVisible(backButton);

  deleteButton.onClick = [this] { deleteEffectInSlot(selectedSlotIndex); };
  deleteButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour::fromString("#e53e3e"));
  addAndMakeVisible(deleteButton);

  setSize(800, 600);
  showView(ViewMode::chainView);
}

MainComponent::~MainComponent() { shutdownAudio(); }

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  currentSampleRate = sampleRate;
  currentSamplesPerBlockExpected = samplesPerBlockExpected;

  const juce::ScopedLock sl(audioLock);
  for (auto &effect : effectChain) {
    if (effect != nullptr) {
      effect->prepareToPlay(sampleRate, samplesPerBlockExpected);
    }
  }
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  juce::AudioBuffer<float> proxyBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      bufferToFill.numSamples);

  const int numChannels = proxyBuffer.getNumChannels();
  const int numSamples = proxyBuffer.getNumSamples();

  if (currentOutputMode.load() == OutputMode::stereo) {
    if (numChannels >= 2) {
      proxyBuffer.copyFrom(1, 0, proxyBuffer.getReadPointer(0), numSamples);
    }
    const juce::ScopedLock sl(audioLock);
    for (auto &effect : effectChain) {
      if (effect != nullptr) {
        effect->process(proxyBuffer);
      }
    }
  } else {
    if (numChannels >= 2) {
      for (int ch = 1; ch < numChannels; ++ch) {
        proxyBuffer.clear(ch, 0, numSamples);
      }
    }

    juce::AudioBuffer<float> monoBuffer(proxyBuffer.getArrayOfWritePointers(),
                                        1, numSamples);
    {
      const juce::ScopedLock sl(audioLock);
      for (auto &effect : effectChain) {
        if (effect != nullptr) {
          effect->process(monoBuffer);
        }
      }
    }

    if (numChannels >= 2) {
      for (int ch = 1; ch < numChannels; ++ch) {
        proxyBuffer.clear(ch, 0, numSamples);
      }
    }
  }
}

void MainComponent::releaseResources() {}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour::fromString("#161920"));

  if (currentViewMode == ViewMode::chainView) {
    // Draw glowing golden signal connection lines across the 8 slots
    g.setColour(juce::Colour::fromString("#d69e2e"));

    // IN circle: center (39, 165) -> connect to Slot 0 (x=75)
    g.fillRect(55, 163, 20, 4);

    // Row 1 connections between Slots 0, 1, 2, 3
    g.fillRect(215, 163, 30, 4); // Slot 0 to 1
    g.fillRect(385, 163, 30, 4); // Slot 1 to 2
    g.fillRect(555, 163, 30, 4); // Slot 2 to 3

    // Snake loop down from right of Slot 3 (725, 165) to Slot 4 (585, 365)
    g.fillRect(725, 163, 30, 4);
    g.fillRect(753, 163, 4, 204);
    g.fillRect(725, 363, 30, 4);

    // Row 2 connections (right to left: Slot 4 -> Slot 5 -> Slot 6 -> Slot 7)
    g.fillRect(555, 363, 30, 4); // Slot 4 to 5
    g.fillRect(385, 363, 30, 4); // Slot 5 to 6
    g.fillRect(215, 363, 30, 4); // Slot 6 to 7

    // Slot 7 to OUT circle: center (39, 365)
    g.fillRect(55, 363, 20, 4);

    // Draw IN circle
    g.setColour(juce::Colour::fromString("#2d3748"));
    g.fillEllipse(16.0f, 142.0f, 46.0f, 46.0f);
    g.setColour(juce::Colour::fromString("#ecc94b"));
    g.drawEllipse(16.0f, 142.0f, 46.0f, 46.0f, 2.5f);
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("IN", 16, 142, 46, 46, juce::Justification::centred);

    // Draw OUT circle
    g.setColour(juce::Colour::fromString("#2d3748"));
    g.fillEllipse(16.0f, 342.0f, 46.0f, 46.0f);
    g.setColour(juce::Colour::fromString("#48bb78"));
    g.drawEllipse(16.0f, 342.0f, 46.0f, 46.0f, 2.5f);
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("OUT", 16, 342, 46, 46, juce::Justification::centred);
  }
}

void MainComponent::resized() {
  settingsButton.setBounds(15, 12, 130, 30);
  modeLabel.setBounds(160, 12, 90, 30);
  modeSelector.setBounds(255, 12, 140, 30);
  chainTitleLabel.setBounds(415, 12, 370, 30);

  if (currentViewMode == ViewMode::chainView) {
    // Layout 8 slots in two rows of 4
    // Row 1: Slots 0, 1, 2, 3 (left to right)
    slotButtons[0]->setBounds(75, 100, 140, 130);
    slotButtons[1]->setBounds(245, 100, 140, 130);
    slotButtons[2]->setBounds(415, 100, 140, 130);
    slotButtons[3]->setBounds(585, 100, 140, 130);

    // Row 2: Slots 4, 5, 6, 7 (right to left for natural snake signal path)
    slotButtons[4]->setBounds(585, 300, 140, 130);
    slotButtons[5]->setBounds(415, 300, 140, 130);
    slotButtons[6]->setBounds(245, 300, 140, 130);
    slotButtons[7]->setBounds(75, 300, 140, 130);
  } else if (currentViewMode == ViewMode::addEffectView) {
    addTitleLabel.setBounds(50, 75, 700, 35);

    const int cardWidth = 210;
    const int cardHeight = 220;
    addGainButton->setBounds(65, 140, cardWidth, cardHeight);
    addOverDriveButton->setBounds(295, 140, cardWidth, cardHeight);
    addReverbButton->setBounds(525, 140, cardWidth, cardHeight);

    backButton.setBounds(295, 420, 210, 45);
  } else if (currentViewMode == ViewMode::editEffectView) {
    editTitleLabel.setBounds(50, 65, 700, 35);

    if (currentEditorComponent != nullptr) {
      currentEditorComponent->setBounds(100, 115, 600, 340);
    }

    backButton.setBounds(175, 490, 210, 45);
    deleteButton.setBounds(415, 490, 210, 45);
  }
}

//==============================================================================
void MainComponent::showView(ViewMode newMode) {
  currentViewMode = newMode;

  if (currentViewMode != ViewMode::editEffectView &&
      currentEditorComponent != nullptr) {
    removeChildComponent(currentEditorComponent.get());
    currentEditorComponent.reset();
  }

  // Hide all view-specific components
  for (auto &btn : slotButtons) {
    btn->setVisible(false);
  }
  addGainButton->setVisible(false);
  addOverDriveButton->setVisible(false);
  addReverbButton->setVisible(false);
  addTitleLabel.setVisible(false);
  editTitleLabel.setVisible(false);
  deleteButton.setVisible(false);
  backButton.setVisible(false);

  // Show required components
  if (currentViewMode == ViewMode::chainView) {
    chainTitleLabel.setVisible(true);
    for (size_t i = 0; i < slotButtons.size(); ++i) {
      slotButtons[i]->setEffectBlock(effectChain[i].get());
      slotButtons[i]->setVisible(true);
    }
  } else if (currentViewMode == ViewMode::addEffectView) {
    chainTitleLabel.setVisible(false);
    addTitleLabel.setText("SLOT " + juce::String(selectedSlotIndex + 1) +
                              " : SELECT EFFECT TYPE TO INSERT",
                          juce::dontSendNotification);
    addTitleLabel.setVisible(true);
    addGainButton->setVisible(true);
    addOverDriveButton->setVisible(true);
    addReverbButton->setVisible(true);
    backButton.setVisible(true);
  } else if (currentViewMode == ViewMode::editEffectView) {
    chainTitleLabel.setVisible(false);
    if (selectedSlotIndex < effectChain.size() &&
        effectChain[selectedSlotIndex] != nullptr) {
      editTitleLabel.setText(
          "SLOT " + juce::String(selectedSlotIndex + 1) + " : " +
              effectChain[selectedSlotIndex]->getName().toUpperCase() +
              " PARAMETER ADJUSTMENT",
          juce::dontSendNotification);
      editTitleLabel.setVisible(true);

      currentEditorComponent = effectChain[selectedSlotIndex]->createEditor();
      addAndMakeVisible(currentEditorComponent.get());
      currentEditorComponent->setVisible(true);

      deleteButton.setVisible(true);
      backButton.setVisible(true);
    } else {
      showView(ViewMode::chainView);
      return;
    }
  }

  resized();
  repaint();
}

void MainComponent::addEffectToSlot(size_t slotIdx,
                                    std::unique_ptr<EffectBlock> newBlock) {
  if (newBlock) {
    newBlock->prepareToPlay(currentSampleRate, currentSamplesPerBlockExpected);
    {
      const juce::ScopedLock sl(audioLock);
      if (slotIdx < effectChain.size()) {
        effectChain[slotIdx] = std::move(newBlock);
      }
    }
  }
  showView(ViewMode::chainView);
}

void MainComponent::deleteEffectInSlot(size_t slotIdx) {
  std::unique_ptr<EffectBlock> removedBlock;
  {
    const juce::ScopedLock sl(audioLock);
    if (slotIdx < effectChain.size()) {
      removedBlock = std::move(effectChain[slotIdx]);
    }
  }
  // removedBlock goes out of scope outside audioLock, safely running
  // destructors on GUI thread
  showView(ViewMode::chainView);
}
