
// Marcel Timm, RhinoDevel, 2024may12

// *****************************************************************************
// *** Kenbak-1 state machine                                                ***
// *****************************************************************************
//
// Page 25: Common and manual operations
// 
// Manual operations and automatic operations which are common to one and two
// byte instructions.
//
// SL, SN, SS, SV, SY, SZ
// -> SA
//    Set R = 3.
//    SA will not be left, until CM.
//    -CM-> SB
// 		 Set P = P + INC.
// 		 Set W = P.
// 		 
// 		 Next state depends on ED:
// 		 
// 		 (a) -^ED-> SC
// 					Set R = W.
// 					SC will not be left, until CM.
// 					-CM-> SD
// 						  Set I = MR.
// 						  
// 						  Next state depends on I3 and I2:
// 						  
// 						  (a)(a) ^I3 and ^I2 -> SU (on page 27).
// 						  
// 						  (a)(b)  I3 or I2   -> SE (on page 26).
// 
// 		 (b) -ED-> QC
// 				   - idle -
// 				   QC will not be left, until GO or X5.
// 				   
// 				   Next state depends on GO and X5:
// 				   
// 				   (b)(a) -GO-> QB
// 								- wait -
// 								QB will not be left, until ^GO.
// 								-^GO-> SA (see above).
// 
// 				   (b)(b) -X5-> QD
// 								Set R = W.
// 								QD will not be left, until CM.
// 								-CM-> QE
// 									  - execute manual operation -
// 									  -> QF
// 										 - wait -
// 										 QF will not be left, until ^X5.
// 										 -^X5-> QC (see above).
//
// Page 26: Two byte instructions
//
// ...
//
// Page 27: One byte instructions
//
// SD (on page 25)
// - ^I3 and ^I2 -> SU
//                  Set R to 0 (A register) or to 1 (B register).
//                  SU will not be left, until CM.
//                  -CM-> SV
//                        Set W = MR.
//
//                        Next state depends on I0:
//
//                        (a) -^I0-> SA (on page 25)
//                         
//                        (b) - I0-> SW
//                                   - do shift -
//                                   -> SX
//                                      Set R to 0 (A register) or to 1 (B
//                                      register).
//                                      SX will not be left, until CM.
//                                      -CM-> SY
//                                            Set WD = W.
//                                            -> SA (on page 25).
//
// *****************************************************************************

#ifndef KENBAK_STATE
#define KENBAK_STATE

#define KENBAK_STATE_TYPE_SHIFT 5 // bits
#define KENBAK_STATE_TYPE_Q 17 // Q is the 17th letter in the alphabet.
#define KENBAK_STATE_TYPE_S 19 // S is the 19th letter in the alphabet.

#define KENBAK_STATE_TYPE_SHIFTED_Q \
    (KENBAK_STATE_TYPE_Q << KENBAK_STATE_TYPE_SHIFT)
#define KENBAK_STATE_TYPE_SHIFTED_S \
    (KENBAK_STATE_TYPE_S << KENBAK_STATE_TYPE_SHIFT)

enum kenbak_state
{
    // *********************************************
    // *** States added, not from the manual(-s) ***
    // *********************************************

    kenbak_state_power_off = -1,
    kenbak_state_unknown = 0,

    // ************************************************
    // *** Q? = States pertain to manual operations ***
    // ************************************************

    // QB = State to wait for start button to be released. (09)
    kenbak_state_qb = KENBAK_STATE_TYPE_SHIFTED_Q | 2,

    // QC = Idle state for manual operations. (09)
    kenbak_state_qc = KENBAK_STATE_TYPE_SHIFTED_Q | 3,

    // QD = State to seek address comparison for manual operations. (09)
    kenbak_state_qd = KENBAK_STATE_TYPE_SHIFTED_Q | 4,

    // QE = State to execute manual operations. (09)
    kenbak_state_qe = KENBAK_STATE_TYPE_SHIFTED_Q | 5,

    // QF = State to wait for control buttons to be released. (09)
    kenbak_state_qf = KENBAK_STATE_TYPE_SHIFTED_Q | 6,

    // *******************************************************************
    // *** S? = States pertain to automatic operations of the computer ***
    // ***      or run mode                                            ***
    // *******************************************************************

    // SA = State to seek P register. (09)
    kenbak_state_sa = KENBAK_STATE_TYPE_SHIFTED_S | 1,

    // SB = State to read and update P register. (09)
    kenbak_state_sb = KENBAK_STATE_TYPE_SHIFTED_S | 2,

    // SC = State to seek instruction. (09)
    kenbak_state_sc = KENBAK_STATE_TYPE_SHIFTED_S | 3,

    // SD = State to read first byte of instruction. (09)
    kenbak_state_sd = KENBAK_STATE_TYPE_SHIFTED_S | 4,

    // SE = State to read second byte of instruction. (09)
    kenbak_state_se = KENBAK_STATE_TYPE_SHIFTED_S | 5,

    // SF = State to seek indirect address. (09)
    kenbak_state_sf = KENBAK_STATE_TYPE_SHIFTED_S | 6,

    // SG = State to read indirect address. (09)
    kenbak_state_sg = KENBAK_STATE_TYPE_SHIFTED_S | 7,

    // SH = State to seek X register. (09).
    kenbak_state_sh = KENBAK_STATE_TYPE_SHIFTED_S | 8,

    // (there is no state SI)

    // SJ = State to do index address computation. (09)
    kenbak_state_sj = KENBAK_STATE_TYPE_SHIFTED_S | 10,

    // SK = State to seek operand. (09)
    kenbak_state_sk = KENBAK_STATE_TYPE_SHIFTED_S | 11,

    // SL = State to read operand & execute bit manipulation instructions. (09)
    kenbak_state_sl = KENBAK_STATE_TYPE_SHIFTED_S | 12,

    // SM = State to seek A, B or X register. (09)
    kenbak_state_sm = KENBAK_STATE_TYPE_SHIFTED_S | 13,

    // SN = State to execute change to A, B, or X (also P) register. (09)
    kenbak_state_sn = KENBAK_STATE_TYPE_SHIFTED_S | 14,

    // (there is no state SO)

    // SP = State to load I with A, B or X register. (09)
    kenbak_state_sp = KENBAK_STATE_TYPE_SHIFTED_S | 16,

    // SQ = State to modify P in jump and mark instructions. (09)
    kenbak_state_sq = KENBAK_STATE_TYPE_SHIFTED_S | 17,

    // SR = State to seek address for storing in memory. (09)
    kenbak_state_sr = KENBAK_STATE_TYPE_SHIFTED_S | 18,

    // SS = State to store data in memory. (09)
    kenbak_state_ss = KENBAK_STATE_TYPE_SHIFTED_S | 19,

    // ST = State to seek P register. (09)
    kenbak_state_st = KENBAK_STATE_TYPE_SHIFTED_S | 20,

    // SU = State to seek A or B register. (09)
    kenbak_state_su = KENBAK_STATE_TYPE_SHIFTED_S | 21,

    // SV = State to read A or B register. (09)
    kenbak_state_sv = KENBAK_STATE_TYPE_SHIFTED_S | 22,

    // SW = State to perform shift/rotates in W register. (09)
    kenbak_state_sw = KENBAK_STATE_TYPE_SHIFTED_S | 23,

    // SX = State to seek A or B register. (09)
    kenbak_state_sx = KENBAK_STATE_TYPE_SHIFTED_S | 24,

    // SY = State to store W register in A or B register. (09)
    kenbak_state_sy = KENBAK_STATE_TYPE_SHIFTED_S | 25,

    // SZ = State to evaluate jump conditions. (09)
    kenbak_state_sz = KENBAK_STATE_TYPE_SHIFTED_S | 26
};

#endif //KENBAK_STATE
