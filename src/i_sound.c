// I_SOUND.C

#include <h2def.h>
#include <i_sound.h>

/*
 *
 *                           SOUND HEADER & DATA
 *
 *
 */

int snd_Channels;
int snd_MaxVolume, // maximum volume for sound
	snd_MusicVolume; // maximum volume for music

void I_PauseSong(int handle)
{
}

void I_ResumeSong(int handle)
{
}

void I_SetMusicVolume(int volume)
{
}

void I_SetSfxVolume(int volume)
{
	snd_MaxVolume = volume; // THROW AWAY?
}

/*
 *
 *                              SONG API
 *
 */

int I_RegisterSong(void *data)
{
	return 0;
}

void I_UnRegisterSong(int handle)
{
}

int I_QrySongPlaying(int handle)
{
	return 0;
}

// Stops a song.  MUST be called before I_UnregisterSong().

void I_StopSong(int handle)
{
}

void I_PlaySong(int handle, boolean looping)
{
}

/*
 *
 *                                 SOUND FX API
 *
 */

// Gets lump nums of the named sound.  Returns pointer which will be
// passed to I_StartSound() when you want to start an SFX.  Must be
// sure to pass this to UngetSoundEffect() so that they can be
// freed!

int I_GetSfxLumpNum(sfxinfo_t *sound)
{
	return W_GetNumForName(sound->lumpname);
}

int I_StartSound(int id, void *data, int vol, int sep, int pitch, int priority)
{
	return 0;
}

void I_StopSound(int handle)
{
}

int I_SoundIsPlaying(int handle)
{
	return 0;
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
}

// inits all sound stuff

void I_StartupSound(void)
{
}

// shuts down all sound stuff

void I_ShutdownSound(void)
{
}

void I_SetChannels(int channels)
{
}
