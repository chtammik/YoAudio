#include "YoManager.h"
#include "Defs.h"
#include <algorithm>

std::unique_ptr<YoManager> YoManager::sInstance = nullptr;
bool YoManager::sInitialized = false;

YoManager * YoManager::GetInstance()
{
	if (sInstance == nullptr)
	{
		sInstance = std::make_unique<YoManager>();
	}

	return sInstance.get();
}

void YoManager::Release(bool quitSDL)
{
	if(sInstance == nullptr)
		return;
	
	sInstance = nullptr;

	if (quitSDL == true)
	{
		// quit SDL
		SDL_Quit();
	}
	else
	{
		// only quit SDL Audio system
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}

bool YoManager::IsInitialized()
{
	return sInitialized;
}

uint16_t YoManager::PlayWavFile(const std::string & filename, bool loop, float volume, float pitch)
{
	printf("Playing: %s\n", filename.c_str());

	if (m_device == nullptr)
	{
		printf("NULL Error: no Device present!");
		return 0;
	}

	std::shared_ptr<Voice> newVoice = m_resources->GetVoice();
	newVoice->Sound = m_resources->GetSound(filename);
	
	if (newVoice->Sound == nullptr)
	{
		newVoice->State = Stopped;
		
		printf("Could not load sound at path: %s", filename);
		return 0;
	}

	newVoice->Sound->Spec.userdata = this;

	newVoice->PlayHead = newVoice->Sound->Buffer;
	newVoice->LengthRemaining = newVoice->Sound->Length;
	newVoice->Volume = volume;
	newVoice->Pitch = pitch;
	newVoice->IsLooping = loop;

	this->QueueVoice(newVoice);

	printf("%s got ID: %i\n", filename.c_str(), newVoice->ID);
	return newVoice->ID;
}

bool YoManager::StopVoice(uint16_t id) noexcept
{
	if (m_device == nullptr)
	{
		printf("NULL Error: no Device present!");
		return false;
	}

	if (id == 0 || id > m_resources->GetVoiceCount())
	{
		printf("Invadid ID: %i\nCan't stop specified Voice as it does not exist!\n\n", id);
		return false;
	}

	for (auto sound : m_playingAudio)
	{
		if (sound->ID != id)
			continue;

		if (sound->State != Stopped)
		{
			SDL_LockAudioDevice(m_device->DeviceID);
			sound->State = Stopping;
			SDL_UnlockAudioDevice(m_device->DeviceID);

			printf("Stopped Voice: %i\n", id);
		}

		return true;
	}

	return false;
}

void YoManager::Pause(bool pause) noexcept
{
	if (m_device == nullptr)
	{
		printf("NULL Error: no Device present!");
		return;
	}

	if (!m_Paused)
	{
		if(pause)
		{
			printf("Yo Audio pausing\n");
			SDL_PauseAudioDevice(m_device->DeviceID, 1);
			m_Paused = true;
		}
		else
		{
			printf("Yo Audio already paused\n");
		}
	}
	else
	{
		if (m_Paused)
		{
			printf("Yo Audio resuming\n");
			SDL_PauseAudioDevice(m_device->DeviceID, 0);
			m_Paused = false;
		}
		else
		{
			printf("Yo Audio already playing\n");
		}
	}
}

bool YoManager::IsPaused() noexcept
{
	return m_Paused;
}

void YoManager::Run()
{
	printf("Thread started!\n");

	std::unique_ptr<Timer> time = std::make_unique<Timer>();
	m_resources = std::make_unique<ResourceManager>();
	m_device = std::make_unique<AudioDevice>();

	SDL_memset(&(m_device->SpecWanted), 0, sizeof(m_device->SpecWanted));

	(m_device->SpecWanted).freq = AUDIO_FREQUENCY;
	(m_device->SpecWanted).format = AUDIO_FORMAT;
	(m_device->SpecWanted).channels = AUDIO_CHANNELS;
	(m_device->SpecWanted).samples = AUDIO_SAMPLES;
	(m_device->SpecWanted).callback = AudioCallback;
	(m_device->SpecWanted).userdata = this;

	if ((m_device->DeviceID = SDL_OpenAudioDevice(nullptr, 0, &(m_device->SpecWanted), &(m_device->SpecObtained), ALLOWED_CHANGES)) == 0)
	{
		fprintf(stderr, "[%s:\t%d]Warning: failed to open audio device: %s\n\n", __FILE__, __LINE__, SDL_GetError());
		m_Quit = true;
	}

	// unpause SDL audio callback
	this->Pause(false);

	// main thread loop
	while (m_Quit == false)
	{
		if (m_ThreadRunning == false)
		{
			continue;
		}

		// increment deltaTime
		time->Update();
		if (time->DeltaTime() >= 1.0f / UPDATE_RATE)
		{
			//printf("Audio Thread DeltaTime: %f\n", mTimer->DeltaTime());
			this->Update();
			time->Reset();
		}
	}

	this->Pause(true);

	// close SDL audio
	SDL_CloseAudioDevice(m_device->DeviceID);

	m_resources = nullptr;
	m_device = nullptr;

	printf("Thread finished!\n");
}

void YoManager::Update() noexcept
{
	// system updates

	if (m_Paused)
		return;

	// loop throu sound channels
	// interpolate values
	// update statemachines
}

YoManager::YoManager() noexcept
{
	if (SDL_WasInit(SDL_INIT_AUDIO) != 0)
	{
		printf("Audio is already initialized.\n");
	}
	else
	{
		// initialize SDL audio
		if (SDL_Init(SDL_INIT_AUDIO) < 0)
		{
			fprintf(stderr, "[%s:\t%d]Warning: failed to initilize SDL!\n\n", __FILE__, __LINE__);
			return;
		}
	}

	printf("Yo Audio Init\n");

	if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
	{
		fprintf(stderr, "[%s:\t%d]\nError: SDL_INIT_AUDIO not initialized\n\n", __FILE__, __LINE__);
		return;
	}

	m_Quit = false;
	m_ThreadRunning = true;
	m_Thread = std::thread([this]() { this->Run(); });
	// alterate: std::thread t{&Class::Run, this}

	// TODO: check SpecWanted against SpecObtained
}

YoManager::~YoManager() noexcept
{
	// stop the SDL AudioCallback
	this->Pause(true);

	m_Quit = true;
	// wait for thread to finish
	m_Thread.join();

	printf("Yo Audio Quit\n");

	if (m_device == nullptr)
	{
		fprintf(stderr, "[%s:\t%d]\nError: no audio device initialized!\n\n", __FILE__, __LINE__);
		return;
	}

	m_device = nullptr;
}

void YoManager::QueueVoice(std::shared_ptr<Voice> newVoice)
{
	// avoid duplicate Voices
	this->StopVoice(newVoice->ID); // returns true because we probably set the state to ToPlay
	// set voice state
	newVoice->State = ToPlay;

	SDL_LockAudioDevice(m_device->DeviceID);
	// add voice to playing voices vector
	this->m_playingAudio.push_back(newVoice);
	SDL_UnlockAudioDevice(m_device->DeviceID);
}

inline void YoManager::AudioCallback(void * userdata, uint8_t * stream, int len)
{
	YoManager* instance = ((YoManager*)userdata);
	const uint32_t streamLen = (uint32_t) len / 2;
	
	
	std::vector<float>* fstream = &(instance->m_stream);
	
	fstream->reserve(streamLen);
	float* floatStream = fstream->begin()._Ptr;

	for (float f : *fstream)
	{
		f = 0.0f;
	}

	for (auto voice : instance->m_playingAudio)
	{
		//Voice * voice = *it._Ptr;

		if (voice->State == ToPlay)
		{
			voice->State = Playing;
		}

		if (voice->State == Playing)
		{
			// s16 to normalized float
			const float sampleFactor = 1 / 32768.0f;
			float volumeFactor = voice->Volume;
			float pitch = voice->Pitch;

			uint32_t length = (uint32_t)((streamLen > voice->LengthRemaining / 2) ? voice->LengthRemaining / 2 : streamLen);

			const Sint16* samples = (Sint16*)voice->PlayHead;
			float sampleIndex = 0;

			for (uint32_t i = 0; i < length; i++)
			{
				if (i == length - 1)
				{
					voice = voice;
				}

				floatStream[i] = (samples[(int)sampleIndex] * 1.0f) * sampleFactor * volumeFactor;

				// TODO: implement interpolating pitching & resampling
				// non-interpolating pitching
				sampleIndex += pitch;
			}

			voice->PlayHead += length * 2;
			voice->LengthRemaining -= length * 2;

			if (voice->LengthRemaining <= 0)
			{
				if (voice->IsLooping == true)
				{
					voice->PlayHead = voice->Sound->Buffer;
					voice->LengthRemaining = voice->Sound->Length;

					length = streamLen - length;
				}
				else
				{
					// Non looping sound has no more mixable samples
					voice->State = Stopped;
				}
			}
		}
	}

	Sint16* current = (Sint16*)stream;
	for (uint32_t i = 0; i < streamLen; i++)
	{
		float val = floatStream[i];

		// clipping
		if (val > 1.0f)
			val = 1.0f;
		else if (val < -1.0f)
			val = -1.0f;

		// convert float back to s16
		current[i] = (Sint16)(val * 32767);
	}
}
