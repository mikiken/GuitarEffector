#include "MainComponent.h"
#include "EffectBlock/GainBlock.h"
#include "EffectBlock/OverDriveBlock.h"
#include "EffectBlock/ReverbBlock.h"

//==============================================================================
MainComponent::MainComponent() {
  // Some platforms require permissions to open input channels so request that
  // here
  if (juce::RuntimePermissions::isRequired(
          juce::RuntimePermissions::recordAudio) &&
      !juce::RuntimePermissions::isGranted(
          juce::RuntimePermissions::recordAudio)) {
    juce::RuntimePermissions::request(
        juce::RuntimePermissions::recordAudio,
        [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
  } else {
    // Specify the number of input and output channels that we want to open
    setAudioChannels(2, 2);
  }

  // Push EffectBlocks to effect chain.
  // effectChain.push_back(std::make_unique<GainBlock>());
  effectChain.push_back(std::make_unique<OverDriveBlock>());
  effectChain.push_back(std::make_unique<ReverbBlock>());

  // Create and add GUI editors for each effect in the chain
  for (auto &effect : effectChain) {
    auto editor = effect->createEditor();
    addAndMakeVisible(editor.get());
    effectEditors.push_back(std::move(editor));
  }

  // Handling the Settings button click event
  // (pops up JUCE's standard settings dialog)
  settingsButton.onClick = [this] {
    juce::DialogWindow::LaunchOptions options;

    // deviceManager is the management variable held by AudioAppComponent behind
    // the scenes
    auto *selector = new juce::AudioDeviceSelectorComponent(
        deviceManager, 0, 256, // Min. to Max. Input Channels
        0, 256,                // Min. to Max. Output Channels
        false, false, true, false);
    selector->setSize(500, 270);

    options.content.setOwned(selector);
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour =
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.launchAsync();
  };

  addAndMakeVisible(settingsButton);

  // Mode selection setup
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

  // Make sure you set the size of the component after
  // you add any child components.
  setSize(800, 600);
}

MainComponent::~MainComponent() {
  // This shuts down the audio device and clears the audio source.
  shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  for (auto &effect : effectChain) {
    effect->prepareToPlay(sampleRate, samplesPerBlockExpected);
  }
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  // Get a pointer to the target buffer and create a sub-buffer proxy
  // representing only the active audio block range.
  juce::AudioBuffer<float> proxyBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      bufferToFill.numSamples);

  const int numChannels = proxyBuffer.getNumChannels();
  const int numSamples = proxyBuffer.getNumSamples();

  if (currentOutputMode.load() == OutputMode::stereo) {
    // Stereo mode: Copy Left channel (0) to Right channel (1) before applying
    // effects
    if (numChannels >= 2) {
      proxyBuffer.copyFrom(1, 0, proxyBuffer.getReadPointer(0), numSamples);
    }
    for (auto &effect : effectChain) {
      effect->process(proxyBuffer);
    }
  } else {
    // Mono mode: Clear right channel before effects, run effects on mono buffer
    // wrapper, and clear right channel after
    if (numChannels >= 2) {
      for (int ch = 1; ch < numChannels; ++ch) {
        proxyBuffer.clear(ch, 0, numSamples);
      }
    }

    juce::AudioBuffer<float> monoBuffer(proxyBuffer.getArrayOfWritePointers(),
                                        1, numSamples);
    for (auto &effect : effectChain) {
      effect->process(monoBuffer);
    }

    if (numChannels >= 2) {
      for (int ch = 1; ch < numChannels; ++ch) {
        proxyBuffer.clear(ch, 0, numSamples);
      }
    }
  }
}

void MainComponent::releaseResources() {
  // This will be called when the audio device stops, or when it is being
  // restarted due to a setting change.

  // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with
  // a solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  // You can add your drawing code here!
}

void MainComponent::resized() {
  // Specify the display area for the settings button and mode selector
  settingsButton.setBounds(10, 10, 130, 30);
  modeLabel.setBounds(155, 10, 90, 30);
  modeSelector.setBounds(250, 10, 140, 30);

  // Layout effect editors horizontally (pedalboard style)
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(40); // Leave space for top controls
  const int pedalWidth = 160;

  for (auto &editor : effectEditors) {
    editor->setBounds(area.removeFromLeft(pedalWidth));
    area.removeFromLeft(10); // spacing between pedals
  }
}
