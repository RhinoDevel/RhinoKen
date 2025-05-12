
// Marcel Timm, RhinoDevel, 2025may12

#include <assert.h>
#include <stdbool.h>

#include "kenbak_state.h"

char const * kenbak_state_get_str(enum kenbak_state const state)
{
	switch(state)
	{
		case kenbak_state_power_off:	{ return "po"; }
		case kenbak_state_unknown:		{ return "un"; }

		case kenbak_state_qb:			{ return "QB"; }
		case kenbak_state_qc:			{ return "QC"; }
		case kenbak_state_qd:			{ return "QD"; }
		case kenbak_state_qe:			{ return "QE"; }
		case kenbak_state_qf:			{ return "QF"; }

		case kenbak_state_sa:			{ return "SA"; }
		case kenbak_state_sb:			{ return "SB"; }
		case kenbak_state_sc:			{ return "SC"; }
		case kenbak_state_sd:			{ return "SD"; }
		case kenbak_state_se:			{ return "SE"; }
		case kenbak_state_sf:		    { return "SF"; }
		case kenbak_state_sg:		    { return "SG"; }
		case kenbak_state_sh:		    { return "SH"; }
		case kenbak_state_sj:		    { return "SJ"; }
		case kenbak_state_sk:		    { return "SK"; }
		case kenbak_state_sl:		    { return "SL"; }
		case kenbak_state_sm:		    { return "SM"; }
		case kenbak_state_sn:		    { return "SN"; }
		case kenbak_state_sp:		    { return "SP"; }
		case kenbak_state_sq:		    { return "SQ"; }
		case kenbak_state_sr:		    { return "SR"; }
		case kenbak_state_ss:		    { return "SS"; }
		case kenbak_state_st:		    { return "ST"; }
		case kenbak_state_su:		    { return "SU"; }
		case kenbak_state_sv:		    { return "SV"; }
		case kenbak_state_sw:		    { return "SW"; }
		case kenbak_state_sx:		    { return "SX"; }
		case kenbak_state_sy:		    { return "SY"; }
		case kenbak_state_sz:		    { return "SZ"; }

		default:
		{
			assert(false); // Must not get here.
			return NULL;
		}
	}
}
