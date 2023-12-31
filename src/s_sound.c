//**************************************************************************
//**
//** S_SOUND.C
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <SDL2/SDL_stdinc.h>
#include <h2def.h>
#include <i_sound.h>
#include <r_local.h>
#include <p_local.h>
#include <i_system.h>
#include <m_random.h>

// MACROS ------------------------------------------------------------------

#define PRIORITY_MAX_ADJUST 10
#define DIST_ADJUST (MAX_SND_DIST / PRIORITY_MAX_ADJUST)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static boolean S_StopSoundID(int sound_id, int priority);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern sfxinfo_t S_sfx[];
extern musicinfo_t S_music[];

extern int snd_MaxVolume;
extern int snd_MusicVolume;
extern int snd_Channels;

extern void **lumpcache;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static channel_t Channel[MAX_CHANNELS];
static int RegisteredSong; //the current registered song.

static boolean MusicPaused;
static int Mus_Song = -1;
static int Mus_LumpNum;
static void *Mus_SndPtr;
static byte *SoundCurve;

static char ArchivePath[128];

// CODE --------------------------------------------------------------------

//==========================================================================
//
// S_Start
//
//==========================================================================

void S_Start(void)
{
	S_StopAllSound();
	S_StartSong(gamemap, true);
}

//==========================================================================
//
// S_StartSong
//
//==========================================================================

void S_StartSong(int song, boolean loop)
{
	char *songLump;
	int track;

	if (song == Mus_Song) { // don't replay an old song
		return;
	}
	if (RegisteredSong) {
		I_StopSong(RegisteredSong);
		I_UnRegisterSong(RegisteredSong);

		Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);

		RegisteredSong = 0;
	}
	songLump = P_GetMapSongLump(song);
	if (!songLump) {
		return;
	}

	Mus_LumpNum = W_GetNumForName(songLump);
	Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);

	RegisteredSong = I_RegisterSong(Mus_SndPtr);
	I_PlaySong(RegisteredSong,
		   loop); // 'true' denotes endless looping.
	Mus_Song = song;
}

//==========================================================================
//
// S_StartSongName
//
//==========================================================================

void S_StartSongName(char *songLump, boolean loop)
{
	if (!songLump) {
		return;
	}

	if (RegisteredSong) {
		I_StopSong(RegisteredSong);
		I_UnRegisterSong(RegisteredSong);

		Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);

		RegisteredSong = 0;
	}

	Mus_LumpNum = W_GetNumForName(songLump);
	Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);

	RegisteredSong = I_RegisterSong(Mus_SndPtr);
	I_PlaySong(RegisteredSong,
		   loop); // 'true' denotes endless looping.
	Mus_Song = -1;
}

//==========================================================================
//
// S_GetSoundID
//
//==========================================================================

int S_GetSoundID(char *name)
{
	int i;

	for (i = 0; i < NUMSFX; i++) {
		if (!strcmp(S_sfx[i].tagName, name)) {
			return i;
		}
	}
	return 0;
}

//==========================================================================
//
// S_StartSound
//
//==========================================================================

void S_StartSound(mobj_t *origin, int sound_id)
{
	S_StartSoundAtVolume(origin, sound_id, 127);
}

//==========================================================================
//
// S_StartSoundAtVolume
//
//==========================================================================

void S_StartSoundAtVolume(mobj_t *origin, int sound_id, int volume)
{
	int dist, vol;
	int i;
	int priority;
	int sep;
	int angle;
	int absx = 0;
	int absy = 0;

	static int sndcount = 0;
	int chan;

	if (sound_id == 0 || snd_MaxVolume == 0)
		return;
	if (origin == NULL) {
		origin = players[displayplayer].mo;
	}
	if (volume == 0) {
		return;
	}

	if (origin) {
		// calculate the distance before other stuff so that we can throw out
		// sounds that are beyond the hearing range.
		absx = abs(origin->x - players[displayplayer].mo->x);
		absy = abs(origin->y - players[displayplayer].mo->y);
	}

	dist = absx + absy - (absx > absy ? absy >> 1 : absx >> 1);
	dist >>= FRACBITS;
	if (dist >= MAX_SND_DIST) {
		return; // sound is beyond the hearing range...
	}
	if (dist < 0) {
		dist = 0;
	}
	priority = S_sfx[sound_id].priority;
	priority *= (PRIORITY_MAX_ADJUST - (dist / DIST_ADJUST));
	if (!S_StopSoundID(sound_id, priority)) {
		return; // other sounds have greater priority
	}
	for (i = 0; i < snd_Channels; i++) {
		if (!origin || origin->player) {
			i = snd_Channels;
			break; // let the player have more than one sound.
		}
		if (origin ==
		    Channel[i].mo) { // only allow other mobjs one sound
			S_StopSound(Channel[i].mo);
			break;
		}
	}
	if (i >= snd_Channels) {
		for (i = 0; i < snd_Channels; i++) {
			if (Channel[i].mo == NULL) {
				break;
			}
		}
		if (i >= snd_Channels) {
			// look for a lower priority sound to replace.
			sndcount++;
			if (sndcount >= snd_Channels) {
				sndcount = 0;
			}
			for (chan = 0; chan < snd_Channels; chan++) {
				i = (sndcount + chan) % snd_Channels;
				if (priority >= Channel[i].priority) {
					chan = -1; //denote that sound should be replaced.
					break;
				}
			}
			if (chan != -1) {
				return; //no free channels.
			} else //replace the lower priority sound.
			{
				if (Channel[i].handle) {
					if (I_SoundIsPlaying(
						    Channel[i].handle)) {
						I_StopSound(Channel[i].handle);
					}
					if (S_sfx[Channel[i].sound_id]
						    .usefulness > 0) {
						S_sfx[Channel[i].sound_id]
							.usefulness--;
					}
				}
			}
		}
	}
	if (S_sfx[sound_id].lumpnum == 0) {
		S_sfx[sound_id].lumpnum = I_GetSfxLumpNum(&S_sfx[sound_id]);
	}
	if (S_sfx[sound_id].snd_ptr == NULL) {
		S_sfx[sound_id].snd_ptr =
			W_CacheLumpNum(S_sfx[sound_id].lumpnum, PU_SOUND);
	}

	Channel[i].mo = origin;

	vol = (SoundCurve[dist] * (snd_MaxVolume * 8) * volume) >> 14;
	if (origin == players[displayplayer].mo) {
		sep = 128;
		//              vol = (volume*(snd_MaxVolume+1)*8)>>7;
	} else {
		angle = R_PointToAngle2(players[displayplayer].mo->x,
					players[displayplayer].mo->y,
					Channel[i].mo->x, Channel[i].mo->y);
		angle = (angle - viewangle) >> 24;
		sep = angle * 2 - 128;
		if (sep < 64)
			sep = -sep;
		if (sep > 192)
			sep = 512 - sep;
		//              vol = SoundCurve[dist];
	}

	if (S_sfx[sound_id].changePitch) {
		Channel[i].pitch =
			(byte)(127 + (M_Random() & 7) - (M_Random() & 7));
	} else {
		Channel[i].pitch = 127;
	}
	Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr, vol,
					 sep, Channel[i].pitch, 0);
	Channel[i].mo = origin;
	Channel[i].sound_id = sound_id;
	Channel[i].priority = priority;
	Channel[i].volume = volume;
	if (S_sfx[sound_id].usefulness < 0) {
		S_sfx[sound_id].usefulness = 1;
	} else {
		S_sfx[sound_id].usefulness++;
	}
}

//==========================================================================
//
// S_StopSoundID
//
//==========================================================================

static boolean S_StopSoundID(int sound_id, int priority)
{
	int i;
	int lp; //least priority
	int found;

	if (S_sfx[sound_id].numchannels == -1) {
		return (true);
	}
	lp = -1; //denote the argument sound_id
	found = 0;
	for (i = 0; i < snd_Channels; i++) {
		if (Channel[i].sound_id == sound_id && Channel[i].mo) {
			found++; //found one.  Now, should we replace it??
			if (priority >=
			    Channel[i].priority) { // if we're gonna kill one, then this'll be it
				lp = i;
				priority = Channel[i].priority;
			}
		}
	}
	if (found < S_sfx[sound_id].numchannels) {
		return (true);
	} else if (lp == -1) {
		return (false); // don't replace any sounds
	}
	if (Channel[lp].handle) {
		if (I_SoundIsPlaying(Channel[lp].handle)) {
			I_StopSound(Channel[lp].handle);
		}
		if (S_sfx[Channel[lp].sound_id].usefulness > 0) {
			S_sfx[Channel[lp].sound_id].usefulness--;
		}
		Channel[lp].mo = NULL;
	}
	return (true);
}

//==========================================================================
//
// S_StopSound
//
//==========================================================================

void S_StopSound(mobj_t *origin)
{
	int i;

	for (i = 0; i < snd_Channels; i++) {
		if (Channel[i].mo == origin) {
			I_StopSound(Channel[i].handle);
			if (S_sfx[Channel[i].sound_id].usefulness > 0) {
				S_sfx[Channel[i].sound_id].usefulness--;
			}
			Channel[i].handle = 0;
			Channel[i].mo = NULL;
		}
	}
}

//==========================================================================
//
// S_StopAllSound
//
//==========================================================================

void S_StopAllSound(void)
{
	int i;

	//stop all sounds
	for (i = 0; i < snd_Channels; i++) {
		if (Channel[i].handle) {
			S_StopSound(Channel[i].mo);
		}
	}
	memset(Channel, 0, 8 * sizeof(channel_t));
}

//==========================================================================
//
// S_SoundLink
//
//==========================================================================

void S_SoundLink(mobj_t *oldactor, mobj_t *newactor)
{
	int i;

	for (i = 0; i < snd_Channels; i++) {
		if (Channel[i].mo == oldactor)
			Channel[i].mo = newactor;
	}
}

//==========================================================================
//
// S_PauseSound
//
//==========================================================================

void S_PauseSound(void)
{
	I_PauseSong(RegisteredSong);
}

//==========================================================================
//
// S_ResumeSound
//
//==========================================================================

void S_ResumeSound(void)
{
	I_ResumeSong(RegisteredSong);
}

//==========================================================================
//
// S_UpdateSounds
//
//==========================================================================

void S_UpdateSounds(mobj_t *listener)
{
	int i, dist, vol;
	int angle;
	int sep;
	int priority;
	int absx;
	int absy;

	if (snd_MaxVolume == 0) {
		return;
	}

	// Update any Sequences
	SN_UpdateActiveSequences();

	for (i = 0; i < snd_Channels; i++) {
		if (!Channel[i].handle ||
		    S_sfx[Channel[i].sound_id].usefulness == -1) {
			continue;
		}
		if (!I_SoundIsPlaying(Channel[i].handle)) {
			if (S_sfx[Channel[i].sound_id].usefulness > 0) {
				S_sfx[Channel[i].sound_id].usefulness--;
			}
			Channel[i].handle = 0;
			Channel[i].mo = NULL;
			Channel[i].sound_id = 0;
		}
		if (Channel[i].mo == NULL || Channel[i].sound_id == 0 ||
		    Channel[i].mo == listener || listener == NULL) {
			continue;
		} else {
			absx = abs(Channel[i].mo->x - listener->x);
			absy = abs(Channel[i].mo->y - listener->y);
			dist = absx + absy -
			       (absx > absy ? absy >> 1 : absx >> 1);
			dist >>= FRACBITS;

			if (dist >= MAX_SND_DIST) {
				S_StopSound(Channel[i].mo);
				continue;
			}
			if (dist < 0) {
				dist = 0;
			}
			//vol = SoundCurve[dist];
			vol = (SoundCurve[dist] * (snd_MaxVolume * 8) *
			       Channel[i].volume) >>
			      14;
			if (Channel[i].mo == listener) {
				sep = 128;
			} else {
				angle = R_PointToAngle2(listener->x,
							listener->y,
							Channel[i].mo->x,
							Channel[i].mo->y);
				angle = (angle - viewangle) >> 24;
				sep = angle * 2 - 128;
				if (sep < 64)
					sep = -sep;
				if (sep > 192)
					sep = 512 - sep;
			}
			I_UpdateSoundParams(Channel[i].handle, vol, sep,
					    Channel[i].pitch);
			priority = S_sfx[Channel[i].sound_id].priority;
			priority *= PRIORITY_MAX_ADJUST - (dist / DIST_ADJUST);
			Channel[i].priority = priority;
		}
	}
}

//==========================================================================
//
// S_Init
//
//==========================================================================

void S_Init(void)
{
	SoundCurve = W_CacheLumpName("SNDCURVE", PU_STATIC);

	I_StartupSound();
	if (snd_Channels > 8) {
		snd_Channels = 8;
	}
	I_SetChannels(snd_Channels);
	I_SetMusicVolume(snd_MusicVolume);
}

//==========================================================================
//
// S_GetChannelInfo
//
//==========================================================================

void S_GetChannelInfo(SoundInfo_t *s)
{
	int i;
	ChanInfo_t *c;

	s->channelCount = snd_Channels;
	s->musicVolume = snd_MusicVolume;
	s->soundVolume = snd_MaxVolume;
	for (i = 0; i < snd_Channels; i++) {
		c = &s->chan[i];
		c->id = Channel[i].sound_id;
		c->priority = Channel[i].priority;
		c->name = S_sfx[c->id].lumpname;
		c->mo = Channel[i].mo;
		c->distance =
			P_AproxDistance(c->mo->x - viewx, c->mo->y - viewy) >>
			FRACBITS;
	}
}

//==========================================================================
//
// S_GetSoundPlayingInfo
//
//==========================================================================

boolean S_GetSoundPlayingInfo(mobj_t *mobj, int sound_id)
{
	int i;

	for (i = 0; i < snd_Channels; i++) {
		if (Channel[i].sound_id == sound_id && Channel[i].mo == mobj) {
			if (I_SoundIsPlaying(Channel[i].handle)) {
				return true;
			}
		}
	}
	return false;
}

//==========================================================================
//
// S_SetMusicVolume
//
//==========================================================================

void S_SetMusicVolume(void)
{
	I_SetMusicVolume(snd_MusicVolume);

	if (snd_MusicVolume == 0) {
		I_PauseSong(RegisteredSong);

		MusicPaused = true;
	} else if (MusicPaused) {
		I_ResumeSong(RegisteredSong);

		MusicPaused = false;
	}
}

//==========================================================================
//
// S_ShutDown
//
//==========================================================================

void S_ShutDown(void)
{
	I_StopSong(RegisteredSong);
	I_UnRegisterSong(RegisteredSong);
	I_ShutdownSound();
}

//==========================================================================
//
// S_InitScript
//
//==========================================================================

void S_InitScript(void)
{
	int i;

	SC_OpenLump("sndinfo");

	while (SC_GetString()) {
		if (*sc_String == '$') {
			if (!strcasecmp(sc_String, "$ARCHIVEPATH")) {
				SC_MustGetString();
				SDL_strlcpy(ArchivePath, sc_String,
					    sizeof(ArchivePath));
			} else if (!strcasecmp(sc_String, "$MAP")) {
				SC_MustGetNumber();
				SC_MustGetString();
				if (sc_Number) {
					P_PutMapSongLump(sc_Number, sc_String);
				}
			}
			continue;
		} else {
			for (i = 0; i < NUMSFX; i++) {
				if (!strcmp(S_sfx[i].tagName, sc_String)) {
					SC_MustGetString();
					if (*sc_String != '?') {
						SDL_strlcpy(
							S_sfx[i].lumpname,
							sc_String,
							sizeof(S_sfx[i].lumpname));
					} else {
						SDL_strlcpy(
							S_sfx[i].lumpname,
							"default",
							sizeof(S_sfx[i].lumpname));
					}
					break;
				}
			}
			if (i == NUMSFX) {
				SC_MustGetString();
			}
		}
	}
	SC_Close();

	for (i = 0; i < NUMSFX; i++) {
		if (!strcmp(S_sfx[i].lumpname, "")) {
			SDL_strlcpy(S_sfx[i].lumpname, "default",
				    sizeof(S_sfx[i].lumpname));
		}
	}
}
