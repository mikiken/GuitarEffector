#include "MainComponent.h"

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

    // Set gain slider style
    gainSlider.setRange(0.0, 10.0);
    gainSlider.setValue(3.0);
    gainSlider.setSkewFactorFromMidPoint(3.0);
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    // Reflect slider value to gain value
    gainSlider.onValueChange = [this] { gain = gainSlider.getValue(); };
    addAndMakeVisible(gainSlider);

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
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI
    // thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
    // Iterate through all channels in the buffer (e.g., L/R)
    for (int channel = 0; channel < bufferToFill.buffer->getNumChannels();
         ++channel) {
        // Get the pointer to the buffer
        float *channelData = bufferToFill.buffer->getWritePointer(
            channel, bufferToFill.startSample);

        // Iterate thorugh each sample in the buffer
        for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
            // Multiply the original amplitude by gain
            channelData[sample] *= gain;
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
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    gainSlider.setBounds(100, 100, 150, 150);
}
