# Compilation

## Getting the source

```
git clone git://github.com/mikrosimage/loudness_validator.git  
cd loudness_validator
```


## Build

#### Build all tools in release mode
```
scons
```

#### More build info ('Local Options' section)
```
scons --help
```


## Dependencies

#### Libraries

* __loudnessAnalyser__  
Depends on boost accumulators (headers only).

* __loudnessCorrector__  
Depends on loudnessAnalyser.

* __loudnessTools__  
Depends on loudnessAnalyser.

* __loudnessIO__  
Depends on loudnessAnalyser and [libsndfile](http://www.mega-nerd.com/libsndfile/).

#### Apps

* __loudness-analyser__  
To analyse given audio files.

* __loudness-corrector__  
To analyse and correct given audio files.

* __loudness-validator__  
___External dependencies to [Qt4](http://qt.nokia.com/products/).___  
Simple GUI to analyse and correct given audio files.

* __loudness-media-analyser__  
___External dependencies to [avtranscoder](https://github.com/avTranscoder/avTranscoder).___  
To analyse audio in given files of any types.

## Tested compilers

* GCC 4.4.1 on OpenSUSE 11.2
* GCC 4.6.2 on OpenSUSE 11.2
* CLang 3.1 on OpenSUSE 11.2

* GCC 4.5.2 on Windows XP
* MSVC 12 on Windows Se7en


## Tested libraries

* Qt 4.5.3  
* Qt 4.8.6

* libsndfile 1.0.24-22.2
* libsndfile 1.0.25
