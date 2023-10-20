
//**************************************************************************
//**
//** I_IBM.C
//**
//**************************************************************************

#include <h2def.h>
#include <i_system.h>

int main(int argc, char **argv)
{
	myargc = argc;
	myargv = argv;
	H2_Main();

	return 0;
}

/*
============================================================================

						NETWORKING

============================================================================
*/

/* // FUCKED LINES
typedef struct
{
	char    priv[508];
} doomdata_t;
*/ // FUCKED LINES

#define DOOMCOM_ID 0x12345678l

/* // FUCKED LINES
typedef struct
{
	long    id;
	short   intnum;                 // DOOM executes an int to execute commands

// communication between DOOM and the driver
	short   command;                // CMD_SEND or CMD_GET
	short   remotenode;             // dest for send, set by get (-1 = no packet)
	short   datalength;             // bytes in doomdata to be sent

// info common to all nodes
	short   numnodes;               // console is allways node 0
	short   ticdup;                 // 1 = no duplication, 2-5 = dup for slow nets
	short   extratics;              // 1 = send a backup tic in every packet
	short   deathmatch;             // 1 = deathmatch
	short   savegame;               // -1 = new game, 0-5 = load savegame
	short   episode;                // 1-3
	short   map;                    // 1-9
	short   skill;                  // 1-5

// info specific to this node
	short   consoleplayer;
	short   numplayers;
	short   angleoffset;    // 1 = left, 0 = center, -1 = right
	short   drone;                  // 1 = drone

// packet data to be sent
	doomdata_t      data;
} doomcom_t;
*/ // FUCKED LINES

extern doomcom_t *doomcom;

/*
====================
=
= I_InitNetwork
=
====================
*/

void I_InitNetwork(void)
{
	int i;

	i = M_CheckParm("-net");
	if (!i) {
		//
		// single player game
		//
		doomcom = malloc(sizeof(*doomcom));
		memset(doomcom, 0, sizeof(*doomcom));
		netgame = false;
		doomcom->id = DOOMCOM_ID;
		doomcom->numplayers = doomcom->numnodes = 1;
		doomcom->deathmatch = false;
		doomcom->consoleplayer = 0;
		doomcom->ticdup = 1;
		doomcom->extratics = 0;
		return;
	}
	/*
	netgame = true;
	doomcom = (doomcom_t *)atoi(myargv[i + 1]);
	//DEBUG
	doomcom->skill = startskill;
	doomcom->episode = startepisode;
	doomcom->map = startmap;
	doomcom->deathmatch = deathmatch;
*/
}

void I_NetCmd(void)
{
	if (!netgame)
		I_Error("I_NetCmd when not in netgame");
}
