#include "AvSoundFile.hpp"

#include <iomanip>
#include <fstream>
#include <algorithm>
#include <stdexcept>

void AvSoundFile::printProgress()
{
    const int p = (float)_cumulOfSamplesAnalysed / _totalNbSamplesToAnalyse * 100;

    // print progression to file
    if(!_progressionFileName.empty())
    {
        _outputStream->seekp(0);
        *_outputStream << p;
    }
    // print progression to console
    else
        *_outputStream << "[" << std::setw(3) << p << "%]\r" << std::flush;
}

bool AvSoundFile::isEndOfAnalysis()
{
    return _cumulOfSamplesAnalysed >= _totalNbSamplesToAnalyse;
}

AvSoundFile::AvSoundFile(const std::vector<AudioElement>& arrayToAnalyse)
    : _nbChannelsToAnalyse(0)
    , _totalNbSamplesToAnalyse(0)
    , _cumulOfSamplesAnalysed(0)
    , _outputStream(&std::cout)
    , _progressionFileName()
    , _forceDurationToAnalyse(0)
{
    for(size_t fileIndex = 0; fileIndex < arrayToAnalyse.size(); ++fileIndex)
    {
        const std::string filename(arrayToAnalyse.at(fileIndex)._inputFile);
        const size_t streamIndex = arrayToAnalyse.at(fileIndex)._streamIndex;
        const size_t channelIndex = arrayToAnalyse.at(fileIndex)._channelIndex;

        // Analyse input file
        avtranscoder::InputFile* inputFile = NULL;
        std::vector<std::string>::iterator iterFilename =
            std::find(_inputFilenames.begin(), _inputFilenames.end(), filename);
        const bool isAlreadyAllocated = (iterFilename != _inputFilenames.end());
        if(isAlreadyAllocated)
        {
            // get existing InputFile
            const size_t filenameIndex = std::distance(_inputFilenames.begin(), iterFilename);
            inputFile = _inputFiles.at(filenameIndex).first;
        }
        else
        {
            // create new InputFile
            inputFile = new avtranscoder::InputFile(filename);

            // display file properties
            // std::cout << *inputFile;

            // add to list of filename
            _inputFilenames.push_back(filename);
        }
        _inputFiles.push_back(std::make_pair(inputFile, !isAlreadyAllocated));

        // Create reader to convert to float planar
        avtranscoder::AudioReader* reader = new avtranscoder::AudioReader(*inputFile, streamIndex, channelIndex);
        _audioReader.push_back(reader);

        // Get data from audio stream
        const avtranscoder::AudioProperties* audioProperties = reader->getSourceAudioProperties();
        const size_t nbSamples = audioProperties->getNbSamples();
        _inputNbSamples.push_back(nbSamples);
        _totalNbSamplesToAnalyse += nbSamples;
        const int nbChannels = audioProperties->getNbChannels();
        _inputNbChannels.push_back(nbChannels);
        const size_t sampleRate = audioProperties->getSampleRate();
        _inputSampleRate.push_back(sampleRate);

        // Update output of reader
        reader->updateOutput(sampleRate, nbChannels, "fltp");
    }

    // Check the given configuration
    bool nbChannelsAreEqual = true;
    bool sampleRateAreEqual = true;
    const size_t nbChannels = _inputNbChannels.at(0);
    const size_t sampleRate = _inputSampleRate.at(0);
    for(size_t i = 1; i < _inputNbChannels.size(); i++)
    {
        // check number of channels
        if(_inputNbChannels.at(i) != nbChannels)
            nbChannelsAreEqual = false;
        // check sample rate
        if(_inputSampleRate.at(i) != sampleRate)
            sampleRateAreEqual = false;
    }

    if(!nbChannelsAreEqual || !sampleRateAreEqual)
    {
        std::string msg = "The given audio configuration isn't supported by the application.\n";
        msg += "Only audio stream with same sample rate and same number of channels are supported.\n";
        msg += "Error:\n";
        if(nbChannelsAreEqual == false)
            msg += "- Number of channels are not equals\n";
        if(sampleRateAreEqual == false)
            msg += "- Sampling rate are not equals\n";

        throw std::runtime_error(msg);
    }

    // Get number of channels to analyse
    int totalInputNbChannels = 0;
    for(size_t i = 0; i < _inputNbChannels.size(); ++i)
        totalInputNbChannels += _inputNbChannels.at(i);
    _nbChannelsToAnalyse = std::min(totalInputNbChannels, 5); // skip LRE
}

AvSoundFile::~AvSoundFile()
{
    for(std::vector<std::pair<avtranscoder::InputFile*, bool> >::iterator it = _inputFiles.begin(); it != _inputFiles.end();
        ++it)
    {
        // if the input file was allocated
        if(it->second)
            delete it->first;
    }
    for(std::vector<avtranscoder::AudioReader*>::iterator it = _audioReader.begin(); it != _audioReader.end(); ++it)
    {
        delete(*it);
    }
}

void AvSoundFile::analyse(Loudness::analyser::LoudnessAnalyser& analyser)
{
    // update number of samples to analyse
    if(_forceDurationToAnalyse)
    {
        _totalNbSamplesToAnalyse = 0;
        for(size_t i = 0; i < _inputSampleRate.size(); i++)
        {
            _totalNbSamplesToAnalyse += _forceDurationToAnalyse * _inputSampleRate.at(i) * _inputNbChannels.at(i);
        }
    }

    // open file to print duration
    std::ofstream outputFile;
    if(!_progressionFileName.empty())
    {
        outputFile.open(_progressionFileName.c_str());
        _outputStream = &outputFile;
    }

    // init
    analyser.initAndStart(_nbChannelsToAnalyse, _inputSampleRate.at(0));

    // Create planar buffer of float data
    float** audioBuffer = new float*[_nbChannelsToAnalyse];

    bool emptyFrameDecoded = false;
    // Decode audio streams
    while(!isEndOfAnalysis())
    {
        size_t nbSamplesRead = 0;
        size_t nbInputChannelAdded = 0;
        for(size_t fileIndex = 0; fileIndex < _audioReader.size(); ++fileIndex)
        {
            avtranscoder::AudioFrame* dstFrame =
                static_cast<avtranscoder::AudioFrame*>(_audioReader.at(fileIndex)->readNextFrame());

            // empty frame: go to the end of process
            if(dstFrame->getSize() == 0)
            {
                emptyFrameDecoded = true;
                break;
            }

            size_t inputChannel = 0;
            for(size_t channelToAdd = nbInputChannelAdded;
                channelToAdd < nbInputChannelAdded + _inputNbChannels.at(fileIndex); ++channelToAdd)
            {
                audioBuffer[channelToAdd] = (float*)(dstFrame->getData()[inputChannel]);
                ++inputChannel;
                nbSamplesRead += dstFrame->getNbSamplesPerChannel();
            }
            nbInputChannelAdded += _inputNbChannels.at(fileIndex);
        }
        if(emptyFrameDecoded)
            break;

        // Analyse loudness
        const size_t nbSamplesInOneFrame = nbSamplesRead / nbInputChannelAdded;
        analyser.processSamples(audioBuffer, nbSamplesInOneFrame);

        // Progress
        _cumulOfSamplesAnalysed += nbSamplesRead;
        printProgress();
    }

    // Close progression file
    outputFile.close();

    // free audio buffer
    delete audioBuffer;
}

void AvSoundFile::setDurationToAnalyse(const float durationToAnalyse)
{
    if(durationToAnalyse > 0)
        _forceDurationToAnalyse = durationToAnalyse;
}
