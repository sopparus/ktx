/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: maps.c,v 1.11 2006/05/17 20:57:12 oldmanuk Exp $
 */

#include "g_local.h"

char *maps_list[] =
{
	// episode 1
	"e1m1",
	"e1m2",
	"e1m3",
	"e1m4",
	"e1m5",
	"e1m6",
	"e1m7",
	"e1m8",

	// episode 2
	"e2m1",
	"e2m2",
	"e2m3",
	"e2m4",
	"e2m5",
	"e2m6",
	"e2m7",

	// episode 3
	"e3m1",
	"e3m2",
	"e3m3",
	"e3m4",
	"e3m5",
	"e3m6",
	"e3m7",

	// episode 4
	"e4m1",
	"e4m2",
	"e4m3",
	"e4m4",
	"e4m5",
	"e4m6",
	"e4m7",
	"e4m8",

	// 
	"start",
	"end",

	// DM maps
	"dm1",
	"dm2",
	"dm3",
	"dm4",
	"dm5",
	"dm6"
};

int maps_cnt = sizeof ( maps_list ) / sizeof ( maps_list[0] );


void StuffCustomMaps()
{
	float f1, f2, f3;
	char *s2=NULL;
	float dt = StuffDeltaTime( iKey( PROG_TO_EDICT( self->s.v.owner ), "ss" ) );

	if(self->cnt == -1)
		self->cnt = 0;

	f1 = self->cnt;
	f3 = f1 + STUFFCMDS_PER_PORTION;
	f2 = LOCALINFO_MAPS_LIST_START + self->cnt;

	stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "\n"); // FIXME: mb remove this line?
	
	for( ; f1 <= f3 && f2 >= LOCALINFO_MAPS_LIST_START
				    && f2 <= LOCALINFO_MAPS_LIST_END; f2++, f1++ ) {

		s2 = ezinfokey(world, va("%d", (int)f2));

		if ( strnull( s2 ) )
			break;

		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias %s cmd cm %d\n", s2, (int)f1 + 1);
	}

	if( strnull ( s2 ) || f1 <= f3 /* i.e. out of reserved range */ )
	{
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "echo Maps downloaded\n");

        // Tell the world we already have stuffed the commands and map aliases.
		PROG_TO_EDICT( self->s.v.owner )->k_stuff = 1;

		// no more maps in localinfo so setup for removing entity and return.
		self->s.v.think = ( func_t ) SUB_Remove;
		self->s.v.nextthink = g_globalvars.time + 0.1;

		return;
	}

	// next time around we'll be starting from the f1 variable.
	self->cnt = f1;
	
	// 'dt' sec delay before next stuffing.
	self->s.v.nextthink = g_globalvars.time + dt;
	
	return;
}

void StuffMainMaps()
{
	int i, limit;
	float dt = StuffDeltaTime( iKey( PROG_TO_EDICT( self->s.v.owner ), "ss" ) );

	if(self->cnt == -1)
		self->cnt = 0;

	i = self->cnt;
	limit = i + STUFFCMDS_PER_PORTION;
	
	for( ; i <= limit && ( i >=0 && i < maps_cnt ); i++ )
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias %s cmd cm %d\n",
															maps_list[i], -(i + 1));

	if( i <= limit /* done */ )
	{
		// no more maps in maps_list[], so setup for stuffing custom maps.
		self->cnt = -1;
		self->s.v.think = ( func_t ) StuffCustomMaps;
		self->s.v.nextthink = g_globalvars.time + dt;
		return;
	}

	// next time around we'll be starting from the i variable.
	self->cnt = i;
	
	// 'dt' sec delay before next stuffing.
	self->s.v.nextthink = g_globalvars.time + dt;
	
	return;
}


char *GetMapName(int imp)
{
	int i;

	if ( imp < 0 ) { // potential from maps_list[]
		i = -imp - 1;

		if ( i >= 0 && i < maps_cnt )
			return maps_list[i];
	}
	else { // potential custom map
		i = LOCALINFO_MAPS_LIST_START + imp - 1;

	 	if ( i >= LOCALINFO_MAPS_LIST_START && i <= LOCALINFO_MAPS_LIST_END )
			return ezinfokey(world, va("%d", i));
	}

	return "";
}

void SelectMap()
{
	char     *m;
	gedict_t *p = world;
	int 	 till;
	qboolean isVoted = false;

	if( (till = Q_rint( ( k_matchLess ? 15: 7 ) - g_globalvars.time)) > 0  ) {
		G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
		return;
	}

	if ( k_matchLess ) {
		if ( cvar("k_no_vote_map") ) {
			G_sprint(self, 2, "Voting map is %s allowed\n", redtext("not"));
			return;
		}

		if ( match_in_progress != 2 )
			return; // u can select map in matchLess mode, but not in countdown
	}
	else if ( match_in_progress )
		return;

	if ( self->k_spectator && self->k_admin != 2 ) // only admined specs can select map
		return;

	if ( strnull( m = GetMapName( self->cmd_selectMap ) ) )
		return;

	if( ( cvar( "k_lockmap" ) || cvar( "k_master" ) )
			&& self->k_admin != 2 
      ) {
		G_sprint(self, 2, "MAP IS LOCKED!\n"
						  "You are NOT allowed to change!\n");
		return;
	}

	if ( self->v.map == self->cmd_selectMap ) {
		G_sprint(self, 2, "--- your vote is still good ---\n");
		return;
	}

	while ( (p = find(p , FOFCLSN, "player")) )
		if ( p->v.map == self->cmd_selectMap ) {
			isVoted = true;
			break;
		}

	if( !get_votes( OV_MAP ) ) {
		G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext("suggests map"),m);
	}
	else if ( isVoted ) {
		G_bprint(2, "%s %s %s %s %s\n", self->s.v.netname, redtext("agrees"),
			(CountPlayers() < 3 ? redtext("to") : redtext("on") ), redtext("map"), m);
	}
	else
		G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext("would rather play on"), m);

	self->v.map = k_lastvotedmap = self->cmd_selectMap;

	vote_check_map ();
}

