# YoAudio

- [About](#about)
- [Overview](#overview)
- [Build Instructions](#build)
- [Playing a sound](#sound)
- [TODO](#todo)

### About <a name="about"></a>

A C++ audio environment. It is meant as a playground for experimentation with game audio concepts and DSP algorithms. The initial development was heavily inspired by this [Simple SDL2 Audio example](https://github.com/jakebesworth/Simple-SDL2-Audio).

**Goals**

- develop a usable audio framework for games (and other applications)
- learn C++ development: low level, high performance, flexible and robust audio plaback for games as well as digital signal processing (DSP)

## Overview <a name="overview"></a>

### Dependencies <a name="dependencies"></a>

- Simple DirectMedia Layer [SDL version 2.0.7 (stable)](http://libsdl.org/download-2.0.php)

### Build Instructions <a name="build"></a>

The following requirements need to be met to be able to build this project:

- SDL 2.0+ Development Libraries

### Windows

At the moment the build process has only ever been tested on Windows with Microsoft Visual Studio 2017 Community Edition. Follow these steps to generate a Visual Studio project:

- extract **SDL2** to `dependencies/SDL2`
	- make sure to copy *include*, *lib* and *bin* directories
- run **cmake**
	- on Windows simply execute `m.bat`
- Open the solution at `build/YoAudio.snl`
- build by hitting `ctrl + shift + b`
- the output will be built to `bin/Debug` or `bin/Release`
	- YoAudio.dll for runtime
	- YoAudio.pdb for debug symbols 

### Linux & Mac OS

If you build this project on these platforms, let me know how it went or submit a pull request ;)

## Playing a sound <a name="sound"></a>

To see how sounds are being played back check out **main.cpp**.

Here is the basic idea:

	#include "YoAudio.h"

	int main()
	{
		// initialize YO audio
		YOA_Init();
		
		float volume = 1.0f;
		float pitch = 1.0f;
		
		// Play Oneshot
		YOA_PlaySound("door_open_01.wav", false, volume, pitch);
		
		// Play loop and store VoiceID
		uint16_t ambLoop = PlaySound("ambience.wav", true, volume, pitch);

		// Stop loop using VoiceID
		YOA_StopVoice(ambLoop);

		// quit YO audio
		YOA_Quit();
		
		return 0;
	}

## TODO <a name="todo"></a>

These are things I'd like to explore with this library / tool in the future (in no particular order):

- multithreading
	- create MessageQueue
	- create AudioRenderer (filling AudioFrame queue for the callback to copy out of)
- create SampleGenerator interface to generalize audio sources (sample playback, synthesis, streaming)
	- GenerateSamples()
	- SamplesRemaining()
- create Mixer graph	
	- AudioListener
		- MixerGroup
		- Voice
			- SamplePlayer
			- Synth
	- AudioEffect
		- Clipper
		- Filter
			- HighPass
			- LowPass
		- RingModulator
- Voice statemachine
	- add a small fade-out when stopping, pausing, virtualizing
	- fade-in on un-pausing / de-virtualizing
- resample from WAV spec to device spec
- instead of hard coded 16bit int
	- convert from WAV spec format to float
	- convert from float to Device spec format