#ifndef __SOUND__
#define __SOUND__

#define SND_TICRATE 140 // tic rate for updating sound
#define SND_MAXSONGS 40 // max number of songs in game
#define SND_SAMPLERATE 11025 // sample rate of sound effects

void I_PauseSong(int handle);
void I_ResumeSong(int handle);
void I_SetMusicVolume(int volume);
void I_SetSfxVolume(int volume);
int I_RegisterSong(void *data);
void I_UnRegisterSong(int handle);
int I_QrySongPlaying(int handle);
void I_StopSong(int handle);
void I_PlaySong(int handle, boolean looping);
int I_GetSfxLumpNum(sfxinfo_t *sound);
int I_StartSound(int id, void *data, int vol, int sep, int pitch, int priority);
void I_StopSound(int handle);
int I_SoundIsPlaying(int handle);
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch);
void I_sndArbitrateCards(void);
void I_StartupSound(void);
void I_ShutdownSound(void);
void I_SetChannels(int channels);

#endif
