/*****************************************************************************
 *
 *
 * Charge_CANCOM.c
 * Definitions for CAN communication
 *
 *Also includes value calibration, charge status, and all CAN status communications
 *
 *****************************************************************************/


//#include "DSP280x_Device.h"		// DSP280x Headerfile Include File
#include "Shared.h"
#include "ChargeCan.h"

#define LQ10_VAC_SAM_SYSA_LIMDN         ((INT32)512)
#define LQ10_VAC_SAM_SYSA_LIMUP         ((INT32)1536)
#define LQ10_VAC_SAM_SYSB_LIMDN         (((INT32)(-120)) << 10)
#define LQ10_VAC_SAM_SYSB_LIMUP         ((INT32)120 << 10)

#define LQ12_VDC_SAM_SYSA_LIMDN         ((INT32)3277)
#define LQ12_VDC_SAM_SYSA_LIMUP         ((INT32)8192)
#define LQ12_VDC_SAM_SYSB_LIMDN         (((INT32)(-300)) << 12)
#define LQ12_VDC_SAM_SYSB_LIMUP         ((INT32)300 << 12)

#define LQ12_VDC_CON_SYSA_LIMDN         ((INT32)2457)
#define LQ12_VDC_CON_SYSA_LIMUP         ((INT32)8192)
#define LQ12_VDC_CON_SYSB_LIMDN         (((INT32)(-300)) << 12)
#define LQ12_VDC_CON_SYSB_LIMUP         ((INT32)300 << 12)


#define LQ10_IDC_SAM_SYSA_LIMDN         ((INT32)512)
#define LQ10_IDC_SAM_SYSA_LIMUP         ((INT32)1536)
#define LQ10_IDC_SAM_SYSB_LIMDN         (((INT32)(-20)) << 10)
#define LQ10_IDC_SAM_SYSB_LIMUP         ((INT32)20 << 10)

#define LQ10_IDC_CON_SYSA_LIMDN         ((INT32)512)
#define LQ10_IDC_CON_SYSA_LIMUP         ((INT32)1536)
#define LQ10_IDC_CON_SYSB_LIMDN         (((INT32)(-800)) << 10)
#define LQ10_IDC_CON_SYSB_LIMUP         ((INT32)800 << 10)

#define LQ10_POW_CON_SYSA_LIMDN         ((INT32)512)
#define LQ10_POW_CON_SYSA_LIMUP         ((INT32)1536)
#define LQ10_POW_CON_SYSB_LIMDN         (((INT32)(-500)) << 10)
#define LQ10_POW_CON_SYSB_LIMUP         ((INT32)500 << 10)


#define RECOK       0xf0
#define NOVCOM      0xf2

UINT16	g_u16MdlShortProtectFlg = 0;
UINT16	u16DCANTimer = 0;
UINT16	u16WarnStatus = 0;
UINT16	u16WarnStatusLast = 0;
UINT16	u16FrameCnt;

ubitfloat bitTemp;
struct CALIBRATION      // Set values for calibration. Can be called by Cree Monitor/BMS
{
    long lq10K;
    long lq10B;
    long lq10X1;
    long lq10Y1;
    long lq10X2;
    long lq10Y2;
    long lq10SysA;
    long lq10SysB;
    Uint16 u16FunctionCode; //Function Code
    Uint16 u16Step;
    Uint16 u16ErrCode;
}Calibrate;

extern UINT16 u16WrFloatDataThree(UINT16 u16Address, ubitfloat fTemp);

// GSeibt
// Added extern for Global Heartbeat counter.
extern bool g_BoolGotHeartbeat;
extern bool g_BoolChargerIsActive;
// Local Global
Uint32  g_uint32Counter1 = 0;
Uint32  g_uint32Counter2 = 0;
Uint32  g_uint32Counter3 = 0;
Uint32  g_uint32Counter4 = 0;
Uint32  g_uint32Counter5 = 0;



void Charge_main(void)
{
	u16FrameCnt = 0;
//===========loop back function============
	{
	    if(0 == (CanaRegs.CAN_TXRQ_21 & 0x0008))//TA允许发送时，发送一帧 Mbx3
	    {
	    	Charge_sCanTxISR3();//reset TA and send 1 frame if buffer is not empty
	    }

		 if(CanaRegs.CAN_NDAT_21 & 0x0004) //MBX2
	    {
			 Charge_sCanRxISR2();
	    }
	}
//===========loop back function============
	while(Charge_sCanRead(&Charge_g_stCanRxData) != CAN_RXBUF_EMPTY)//从Buffer中读取数据
	{
		Charge_sDecodeCanRxFrame(Charge_g_stCanRxData);

		g_u16CanFailTime = 0;
		g_u16MdlStatus.bit.NOCONTROLLER = 0;

 		if(0 != Charge_g_stCanTxData.id.all)
		{
			Charge_sCanWrite3(&Charge_g_stCanTxData);
			if(CHARGE_READ_LIMIT_A8 == Charge_g_stCanRxData.id.CHG_Bits.CMD)
			{//本协议需要2条回复
				u16FrameCnt += 2;
			}
			else
			{
				u16FrameCnt++;
			}
		}
	}

	u16WarnStatus = vStatusTrans();
	if((u16WarnStatusLast != u16WarnStatus) || (g_u16RunFlag.bit.Tx3SFlag))
	{
		//B2帧处理
		if(g_u16RunFlag.bit.Tx3SFlag)
		{
			sEncodeTxData_B2_01_3S();
			Charge_sCanWrite3(&Charge_g_stCanTxData);
			u16FrameCnt++;
			g_u16RunFlag.bit.Tx3SFlag = 0;

			if(++u16DCANTimer >= 10)//30s
			{
				u16DCANTimer = 0;//重新计时
				sEncodeTxData_B2_02_30S();
				Charge_sCanWrite3(&Charge_g_stCanTxData);
				u16FrameCnt++;
			}
		}
		u16WarnStatusLast = u16WarnStatus;
		sEncodeTxData_B3_3S_Inner(u16WarnStatus);
		Charge_sCanWrite3(&Charge_g_stCanTxData);
		u16FrameCnt++;
	}

	Charge_sCanSendTrigger3(u16FrameCnt);
}

void Charge_sDecodeCanRxFrame(struct Charge_MESSAGE stRxFrameTemp)
{//MDL Processed first
	static ubitfloat lq10CanDataTemp;


	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.all = 0uL;//清空上次赋值（设置ErrByte=0）
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.all = 0uL;
	Charge_g_stCanTxData.id.all = 0;
	switch(stRxFrameTemp.id.CHG_Bits.CMD)
	{
		case CHARGE_SET_V_I_ON_A3:
			Charge_g_stCanTxData.id.all = 0;//No Reply

			lq10CanDataTemp.lData = 0;

			// GSeibt
			// Set flag that we received a Charger Control message (On/Off)same used for Heart beat.
			g_BoolGotHeartbeat = true;
			g_BoolChargerIsActive = 1;  // See void vModeSelect(void) in OBC_start.C

			if(0 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0) //set as rec
			{

			    g_uint32Counter1++;
			    // If this is set to 0 it means switch on the Charger.
                // Hmm - check if already on and then step over this code....
				if (/* (OI) (0 == g_BoolChargerIsActive) &&*/ (0 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE1))
				{
				    g_uint32Counter2++;
				    if((1 == g_u16MdlCtrl.bit.OFFCTRL) || (1 == g_u16MdlCtrl.bit.SHORTRELAY))
					{
				        g_uint32Counter3++;
				        if ( !(g_u16MdlStatus.bit.HVSDLOCK || g_u16MdlStatus.bit.PFCOV
                               || g_u16MdlStatus.bit.PFCUV || g_u16MdlStatus.bit.ACOV
                               || g_u16MdlStatus.bit.ACUV || g_u16MdlStatus.bit.PhaseUnlock
                               || g_u16MdlStatus.bit.AMBIOT
                               || g_u16MdlStatusH.bit.DCOT || g_u16MdlStatusH.bit.PFCOT
                               || g_u16MdlStatusH.bit.SROT|| g_u16MdlStatus.bit.DCOV
                               ||g_u16MdlStatusH.bit.DCHWOV ))	// rectifier on if no fault
                        {
                            if(MODE_INV == CputoClaVar.u16WorkMode)
                            {  //no response for INV
                                g_u16MdlCtrl.bit.SHORTRELAY = 0;
                                g_u16MdlCtrl.bit.OFFCTRL = 0;
                            }
                            else if(	(MODE_REC == CputoClaVar.u16WorkMode) ||
                                    ( (g_lq10AcVolt.lData >= VAC_80V)	&& (g_lq10PfcVolt.lData >= VPFC_80V)
                                && ( (CputoClaVar._uiOverAcVolt == 0) && (g_u16MdlStatus.bit.ACOV == 0)
                                ) 		 )
                                )
                            {
                                g_u16MdlCtrl.bit.SHORTRELAY = 0;//Pri ON
                                g_u16MdlCtrl.bit.OFFCTRL = 0; //Rec status OK

                                // GSeibt Set ChargerIsActive Flag
                                g_BoolChargerIsActive = 1;

                                lq10CanDataTemp.lData = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);//数制转换

                                g_lq10SetVolt.lData =
                                        __IQsat(lq10CanDataTemp.lData + 1, VDC_455V, VDC_250V);


                                lq10CanDataTemp.lData = _IQ10mpy(7153L,stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);//数制转换167772 = 1024/10 * 1/20 * 2^15 ---电流转限流点
                                g_lq10SetLimit.lData = __IQsat(lq10CanDataTemp.lData + 1, LQ10_CURRLIMITMAX, 102);
                            }
                        }
					}
					else if(MODE_REC == CputoClaVar.u16WorkMode)
					{
					    g_uint32Counter4++;
						lq10CanDataTemp.lData = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);//数制转换

						g_lq10SetVolt.lData =
								__IQsat(lq10CanDataTemp.lData + 1, VDC_455V, VDC_250V);


						lq10CanDataTemp.lData = _IQ10mpy(7153L,stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);//数制转换167772 = 1024/10 * 1/20 * 2^15 ---电流转限流点
						g_lq10SetLimit.lData = __IQsat(lq10CanDataTemp.lData + 1, LQ10_CURRLIMITMAX, 102);
					}
				}
				// Switch the Charger off......
				else
				{
				    g_uint32Counter5++;
	                // (OI) if ((1 == g_BoolChargerIsActive) && (1 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE1))
	                //{
	                    g_u16MdlCtrl.bit.SHORTRELAY = 1; //PriSide/Sec side OFF
	                    g_BoolChargerIsActive = 0;
	                //}
				}
			}
			else//set as INV
			{//only ON/OFF, no pri ON_Sec OFF

				if (0 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE1)
				{
					if((1 == g_u16MdlCtrl.bit.OFFCTRL) || (1 == g_u16MdlCtrl.bit.SHORTRELAY))
					{
						if ( !(g_u16MdlStatus.bit.HVSDLOCK || g_u16MdlStatus.bit.PFCOV
							   || g_u16MdlStatus.bit.PFCUV || g_u16MdlStatus.bit.ACOV
							   || g_u16MdlStatus.bit.ACUV || g_u16MdlStatus.bit.PhaseUnlock
							   || g_u16MdlStatus.bit.AMBIOT
							   || g_u16MdlStatusH.bit.DCOT || g_u16MdlStatusH.bit.PFCOT
							   || g_u16MdlStatusH.bit.SROT|| g_u16MdlStatus.bit.DCOV
							   ||g_u16MdlStatusH.bit.DCHWOV ))
						{
							if(MODE_REC == CputoClaVar.u16WorkMode)
							{
                                g_u16MdlCtrl.bit.SHORTRELAY = 0;
                                g_u16MdlCtrl.bit.OFFCTRL = 0;
							}//no response for REC mode
							else if(	(MODE_INV == CputoClaVar.u16WorkMode) ||
								( (g_lq10MdlVolt.lData >= VDC_318V) && (g_lq10PfcVolt.lData <= VPFC_60V)
								 && (g_u16MdlStatus.bit.DCOV == 0) && (g_u16MdlCtrl.bit.OFFCTRL == 0)
								 && (g_u16MdlStatus.bit.HVSDLOCK == 0) && (g_u16MdlStatusH.bit.DCHWOV  == 0) )
								)
							{
								g_u16MdlCtrl.bit.SHORTRELAY = 0;//Pri ON
								g_u16MdlCtrl.bit.OFFCTRL = 0; //Rec status OK

								lq10CanDataTemp.lData = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);//数制转换

								g_lq10SetVolt.lData =
										__IQsat(lq10CanDataTemp.lData + 1, VDC_455V, VDC_250V);


								lq10CanDataTemp.lData = _IQ10mpy(7153L,stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);//数制转换167772 = 1024/10 * 1/20 * 2^15 ---电流转限流点
								g_lq10SetLimit.lData = __IQsat(lq10CanDataTemp.lData + 1, LQ10_CURRLIMITMAX, 102);
							}
						}
					}
					else if(MODE_INV == CputoClaVar.u16WorkMode)
					{
						lq10CanDataTemp.lData = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);//数制转换

						g_lq10SetVolt.lData =
								__IQsat(lq10CanDataTemp.lData + 1, VDC_455V, VDC_250V);


						lq10CanDataTemp.lData = _IQ10mpy(7153L,stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);//数制转换167772 = 1024/10 * 1/20 * 2^15 ---电流转限流点
						g_lq10SetLimit.lData = __IQsat(lq10CanDataTemp.lData + 1, LQ10_CURRLIMITMAX, 102);
					}
				}
				else //if(1 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE1)//PRI OFF
				{
					g_u16MdlCtrl.bit.SHORTRELAY = 1; //PriSide OFF --- 隐藏Sec side OFF
				}
			}
			break;
		case CHARGE_SET_V_I_OFF_A4:
			Charge_g_stCanTxData.id.all = 0;//No Reply
			{
				g_u16MdlCtrl.bit.SHORTRELAY = 1;//g_u16MdlCtrl.bit.OFFCTRL = 1;

				lq10CanDataTemp.lData = 0;
				lq10CanDataTemp.lData = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);//数制转换

				g_lq10SetVolt.lData =
						__IQsat(lq10CanDataTemp.lData + 1, VDC_380V, VDC_250V);//VDC_455V


				lq10CanDataTemp.lData = _IQ10mpy(7153L,stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);//数制转换167772 = 1024/10 * 1/20 * 2^15 ---电流转限流点
				g_lq10SetLimit.lData = __IQsat(lq10CanDataTemp.lData + 1, LQ10_CURRLIMITMAX, 102);
			}
			break;
        case CHARGE_Calibrate_AB://Calibrate
            //Rx:Byte0:Function Code  Byte1:Step Code  WordLL:Vac  WordHH:Vdc WordHL:Idc
            //Tx:Byte0:Function Code  Byte1:Err Code  WordLL:FFFF  WordHH:X WordHL:Y

            //------------------------------------
                                Charge_g_stCanTxData.id.all = REPLYID_READ_LIMIT_BB;

            Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = 0xFFFF;
            Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = 0xFFFF;
            Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = 0xFFFF;
            //-------------------------------------------------------

                                Calibrate.u16ErrCode = 0x00;//err

            //if(1 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
            //{//1==BYTE0 vac calibration

                if(1 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE1)
                {
                    Calibrate.u16FunctionCode = stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0;//Record as Vac Calibration. Step 1
                    Calibrate.u16Step = 1;
                    Calibrate.u16ErrCode = 0xA0;//Step1  Normal Code

                    Calibrate.lq10K = 1024;
                    Calibrate.lq10B = 0;

                    if(1 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//Vac sample
                        Calibrate.lq10X1 = g_lq10AcVolt.lData;
                        Calibrate.lq10Y1 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDL.word.LOW_WORD);

                        g_lq10AcVrmsSampSysa.lData = 1L<<10;
                        g_lq10AcVrmsSampSysb.lData = 0;
                    }
                    else if(2 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//Vdc sample
                        Calibrate.lq10X1 = g_lq10MdlVolt.lData;
                        Calibrate.lq10Y1 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);

                        g_lq12VoltSampSysa.lData = 1L<<12;
                        g_lq12VoltSampSysb.lData = 0;
                    }
                    else if(3 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//Idc sample
                        Calibrate.lq10X1 = g_lq10MdlCurr.lData;
                        Calibrate.lq10Y1 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);

                        g_lq10CurrSampSysa.lData = 1L<<10;
                        g_lq10CurrSampSysb.lData = 0;
                    }
                    else if(4 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//dc  Vset
                        Calibrate.lq10X1 = g_lq10SetVolt.lData;
                        Calibrate.lq10Y1 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);

                        g_lq12VoltConSysa.lData = 1L<<12;
                        g_lq12VoltConSysb.lData = 0;
                    }
                    else if((5 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0))
                    {//Cur limit
                        g_lq10CurrConSysa.lData = 1L<<10;
                        g_lq10CurrConSysb.lData = 0;
                        if(2 == IsrVars._u16ChoiseCon)
                        {
                            Calibrate.lq10X1 = IsrVars.l32IdcdcSys<<5;//Q5
                            Calibrate.lq10Y1 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);
                            Calibrate.lq10B = _IQ5mpy(_IQ5div(457*g_lq10MdlLimit.lData, Calibrate.lq10Y1)-(1L<<10), Calibrate.lq10X1)
                                    + g_lq10CurrConSysb.lData;//18L<<10---29*1291=18.2A_Q11
                            Calibrate.lq10K = Calibrate.lq10SysA = 1024;
                            Calibrate.lq10SysB = __IQsat(g_lq10CurrConSysb.lData + Calibrate.lq10B, 600L<<10,-10L<<10);
                            Calibrate.lq10B = 0;
                        }
                        else
                        {
                            Calibrate.lq10SysA = 1024;
                            Calibrate.lq10SysB = 0;
                            Calibrate.u16ErrCode = 0xF5;
                        }
                    }
                    else if((6 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)/*  &&  (1 == IsrVars._u16ChoiseCon)*/)
                    {//Pwr limit
                        g_lq10PowerConSysa.lData = 1L<<10;
                        g_lq10PowerConSysb.lData = 0;

                        if(1 == IsrVars._u16ChoiseCon)
                        {
                            Calibrate.lq10X1 = (long)IsrVars._i16PowerLimMax<<10;
                            Calibrate.lq10Y1 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);//临时使用 Vdc
                            Calibrate.lq10Y2 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);//临时使用 Idc
                            //功率IQ10
                            Calibrate.lq10Y1 = __lmax(_IQ10mpy(Calibrate.lq10Y1, Calibrate.lq10Y2), 10L<<10);
                            Calibrate.lq10B = _IQ10mpy(_IQ10div(6600*g_lq10SetPowerWant.lData, Calibrate.lq10Y1) - (1L<<10), Calibrate.lq10X1)
                                    + g_lq10PowerConSysb.lData;//6600L<<10
                            //下限为零   上限步进为10
                            Calibrate.lq10SysA = Calibrate.lq10K = 1024;
                            Calibrate.lq10SysB = __IQsat(g_lq10PowerConSysb.lData + Calibrate.lq10B, 200L<<10, -10L<<10);
                            Calibrate.lq10B = 0;
                        }
                        else
                        {
                            Calibrate.lq10SysA = 1024;
                            Calibrate.lq10SysB = 0;
                            Calibrate.u16ErrCode = 0xF6;
                        }
                    }
                    else
                    {
                        Calibrate.u16ErrCode = 0xFF;//out of range
                        Calibrate.u16FunctionCode = 0x00; //Clear Step
                    }

                    if((5 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)||(6 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0))
                    {
                        Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = (Calibrate.lq10SysA*10L)>>10;
                        Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = (Calibrate.lq10SysB*10L)>>10;
                    }
                    else
                    {
                        Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = (Calibrate.lq10X1*10L)>>10;
                        Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = (Calibrate.lq10Y1*10L)>>10;
                    }
                }
                //--------------------step2--------------------------
                else if((2 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE1) && (1 == Calibrate.u16Step)
                        && (stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0 == Calibrate.u16FunctionCode))//已经完成Step1的前提下
                {
                    Calibrate.u16ErrCode = 0xFF;//0xB0-----Step 2 Normal Code
                    Calibrate.u16Step = 2;
                    if(1 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//Vac sample
                        Calibrate.lq10X2 = g_lq10AcVolt.lData;
                        Calibrate.lq10Y2 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDL.word.LOW_WORD);

                        //--------------------Calculation Based on (X1,Y1),(X2,Y2)
                        if(labs(Calibrate.lq10X2-Calibrate.lq10X1) > (20L<<10))//避免分母过小
                        {
                            Calibrate.lq10K = _IQ10div(Calibrate.lq10Y2-Calibrate.lq10Y1,
                                                       Calibrate.lq10X2-Calibrate.lq10X1);
                            Calibrate.lq10B = _IQ10div(_IQ10mpy(Calibrate.lq10Y1,Calibrate.lq10X2)-_IQ10mpy(Calibrate.lq10Y2,Calibrate.lq10X1),
                                                       Calibrate.lq10X2-Calibrate.lq10X1);

                            Calibrate.lq10SysA = _IQ10mpy(g_lq10AcVrmsSampSysa.lData, Calibrate.lq10K);
                            Calibrate.lq10SysB = _IQ10mpy(g_lq10AcVrmsSampSysb.lData, Calibrate.lq10K) + Calibrate.lq10B;

                            if(    (Calibrate.lq10SysA >= LQ10_VAC_SAM_SYSA_LIMDN )
                                && (Calibrate.lq10SysA <= LQ10_VAC_SAM_SYSA_LIMUP)
                                && (Calibrate.lq10SysB >= LQ10_VAC_SAM_SYSB_LIMDN )
                                && (Calibrate.lq10SysB <= LQ10_VAC_SAM_SYSB_LIMUP)
                              )
                            {
                                bitTemp.lData = Calibrate.lq10SysA;
                                Calibrate.u16ErrCode = u16WrFloatDataThree(40, bitTemp);
                                bitTemp.lData = Calibrate.lq10SysB;
                                Calibrate.u16ErrCode = Calibrate.u16ErrCode
                                              | u16WrFloatDataThree(44, bitTemp);
                                if (Calibrate.u16ErrCode == RECOK)
                                {
                                    g_lq10AcVrmsSampSysa.lData = Calibrate.lq10SysA;
                                    g_lq10AcVrmsSampSysb.lData = Calibrate.lq10SysB;

                                    Calibrate.u16ErrCode = 0xB0;
                                }
                            }
                        }
                    }
                    else if(2 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//Vdc sample
                        Calibrate.lq10X2 = g_lq10MdlVolt.lData;
                        Calibrate.lq10Y2 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);

                        //--------------------Calculation Based on (X1,Y1),(X2,Y2)
                        if(labs(Calibrate.lq10X2-Calibrate.lq10X1) > (20L<<10))//避免分母过小
                        {
                            Calibrate.lq10K = _IQ10div(Calibrate.lq10Y2-Calibrate.lq10Y1,
                                                       Calibrate.lq10X2-Calibrate.lq10X1);
                            Calibrate.lq10B = _IQ10div(_IQ10mpy(Calibrate.lq10Y1,Calibrate.lq10X2)-_IQ10mpy(Calibrate.lq10Y2,Calibrate.lq10X1),
                                                       Calibrate.lq10X2-Calibrate.lq10X1);

                            Calibrate.lq10SysA = _IQ10mpy(g_lq12VoltSampSysa.lData, Calibrate.lq10K);
                            Calibrate.lq10SysB = _IQ10mpy(g_lq12VoltSampSysb.lData, Calibrate.lq10K) + Calibrate.lq10B;

                            if(    (Calibrate.lq10SysA >= LQ12_VDC_SAM_SYSA_LIMDN )
                                && (Calibrate.lq10SysA <= LQ12_VDC_SAM_SYSA_LIMUP)
                                && (Calibrate.lq10SysB >= LQ12_VDC_SAM_SYSB_LIMDN )
                                && (Calibrate.lq10SysB <= LQ12_VDC_SAM_SYSB_LIMUP)
                              )
                            {
                                bitTemp.lData = Calibrate.lq10SysA<<2;//Q12
                                Calibrate.u16ErrCode = u16WrFloatDataThree(4, bitTemp);
                                bitTemp.lData = Calibrate.lq10SysB<<2;//Q12
                                Calibrate.u16ErrCode = Calibrate.u16ErrCode
                                              | u16WrFloatDataThree(8, bitTemp);
                                if (Calibrate.u16ErrCode == RECOK)
                                {
                                    g_lq12VoltSampSysa.lData = Calibrate.lq10SysA;
                                    g_lq12VoltSampSysb.lData = Calibrate.lq10SysB;

                                    Calibrate.u16ErrCode = 0xB0;
                                }
                            }
                        }
                    }
                    else if(3 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//Idc sample
                        Calibrate.lq10X2 = g_lq10MdlCurr.lData;
                        Calibrate.lq10Y2 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.LOW_WORD);
                        //--------------------Calculation Based on (X1,Y1),(X2,Y2)
                         if(labs(Calibrate.lq10X2-Calibrate.lq10X1) > (5L<<10))//避免分母过小
                         {
                             Calibrate.lq10K = _IQ10div(Calibrate.lq10Y2-Calibrate.lq10Y1,
                                                        Calibrate.lq10X2-Calibrate.lq10X1);
                             Calibrate.lq10B = _IQ10div(_IQ10mpy(Calibrate.lq10Y1,Calibrate.lq10X2)-_IQ10mpy(Calibrate.lq10Y2,Calibrate.lq10X1),
                                                        Calibrate.lq10X2-Calibrate.lq10X1);

                             Calibrate.lq10SysA = _IQ10mpy(g_lq10CurrSampSysa.lData, Calibrate.lq10K);
                             Calibrate.lq10SysB = _IQ10mpy(g_lq10CurrSampSysb.lData, Calibrate.lq10K) + Calibrate.lq10B;

                             if(    (Calibrate.lq10SysA >= LQ10_IDC_SAM_SYSA_LIMDN )
                                 && (Calibrate.lq10SysA <= LQ10_IDC_SAM_SYSA_LIMUP)
                                 && (Calibrate.lq10SysB >= LQ10_IDC_SAM_SYSB_LIMDN )
                                 && (Calibrate.lq10SysB <= LQ10_IDC_SAM_SYSB_LIMUP)
                               )
                             {
                                 bitTemp.lData = Calibrate.lq10SysA;
                                 Calibrate.u16ErrCode = u16WrFloatDataThree(20, bitTemp);
                                 bitTemp.lData = Calibrate.lq10SysB;
                                 Calibrate.u16ErrCode = Calibrate.u16ErrCode
                                               | u16WrFloatDataThree(24, bitTemp);

                                 if (Calibrate.u16ErrCode == RECOK)
                                 {
                                     g_lq10CurrSampSysa.lData = Calibrate.lq10SysA;
                                     g_lq10CurrSampSysb.lData = Calibrate.lq10SysB;

                                     Calibrate.u16ErrCode = 0xB0;
                                 }
                             }
                         }
                    }
                    else if(4 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)
                    {//dc  Vset
                        Calibrate.lq10X2 = g_lq10SetVolt.lData;
                        Calibrate.lq10Y2 = _IQ5mpy(3277L, stRxFrameTemp.DataAll.MBoxDataBits.MDH.word.HI_WORD);


                        //--------------------Calculation Based on (X1,Y1),(X2,Y2)
                         if(labs(Calibrate.lq10X2-Calibrate.lq10X1) > (20L<<10))//避免分母过小
                         {
                             Calibrate.lq10K = _IQ10div(Calibrate.lq10Y2-Calibrate.lq10Y1,
                                                        Calibrate.lq10X2-Calibrate.lq10X1);
                             Calibrate.lq10B = _IQ10div(_IQ10mpy(Calibrate.lq10Y1,Calibrate.lq10X2)-_IQ10mpy(Calibrate.lq10Y2,Calibrate.lq10X1),
                                                        Calibrate.lq10X2-Calibrate.lq10X1);

                             Calibrate.lq10SysA = _IQ10mpy(g_lq12VoltConSysa.lData, Calibrate.lq10K);
                             Calibrate.lq10SysB = _IQ10mpy(g_lq12VoltConSysb.lData, Calibrate.lq10K) + Calibrate.lq10B;

                             if(    (Calibrate.lq10SysA >= LQ12_VDC_CON_SYSA_LIMDN )
                                 && (Calibrate.lq10SysA <= LQ12_VDC_CON_SYSA_LIMUP)
                                 && (Calibrate.lq10SysB >= LQ12_VDC_CON_SYSB_LIMDN )
                                 && (Calibrate.lq10SysB <= LQ12_VDC_CON_SYSB_LIMUP)
                               )
                             {
                                 bitTemp.lData = Calibrate.lq10SysA<<2;
                                 Calibrate.u16ErrCode = u16WrFloatDataThree(12, bitTemp);
                                 bitTemp.lData = Calibrate.lq10SysB<<2;
                                 Calibrate.u16ErrCode = Calibrate.u16ErrCode
                                               | u16WrFloatDataThree(16, bitTemp);

                                 if (Calibrate.u16ErrCode == RECOK)
                                 {
                                     g_lq12VoltConSysa.lData = Calibrate.lq10SysA;
                                     g_lq12VoltConSysb.lData = Calibrate.lq10SysB;

                                     Calibrate.u16ErrCode = 0xB0;
                                 }
                             }
                         }
                    }
                    else if((5 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0) && (0 == IsrVars._u16ChoiseCon))
                    {//Cur limit
                        //--------------------Calculation Based on (X1,Y1),(X2,Y2)
                         {
                             if(    (Calibrate.lq10SysA >= LQ10_IDC_CON_SYSA_LIMDN )
                                 && (Calibrate.lq10SysA <= LQ10_IDC_CON_SYSA_LIMUP)
                                 && (Calibrate.lq10SysB >= LQ10_IDC_CON_SYSB_LIMDN )
                                 && (Calibrate.lq10SysB <= LQ10_IDC_CON_SYSB_LIMUP)
                               )
                             {
                                 bitTemp.lData = Calibrate.lq10SysA = 1024;
                                 Calibrate.u16ErrCode = u16WrFloatDataThree(28, bitTemp);
                                 bitTemp.lData = Calibrate.lq10SysB;
                                 Calibrate.u16ErrCode = Calibrate.u16ErrCode
                                               | u16WrFloatDataThree(32, bitTemp);

                                 if (Calibrate.u16ErrCode == RECOK)
                                 {
                                     g_lq10CurrConSysa.lData = Calibrate.lq10SysA;
                                     g_lq10CurrConSysb.lData = Calibrate.lq10SysB;

                                     Calibrate.u16ErrCode = 0xB0;
                                 }
                             }
                         }
                    }
                    else if((6 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0) && (0 == IsrVars._u16ChoiseCon))
                    {//Pwr limit
                        {
                            if(    (Calibrate.lq10SysA >= LQ10_POW_CON_SYSA_LIMDN )
                                && (Calibrate.lq10SysA <= LQ10_POW_CON_SYSA_LIMUP)
                                && (Calibrate.lq10SysB >= LQ10_POW_CON_SYSB_LIMDN )
                                && (Calibrate.lq10SysB <= LQ10_POW_CON_SYSB_LIMUP)
                              )
                            {
                                bitTemp.lData = Calibrate.lq10SysA = 1024;
                                Calibrate.u16ErrCode = u16WrFloatDataThree(64, bitTemp);
                                bitTemp.lData = Calibrate.lq10SysB;
                                Calibrate.u16ErrCode = Calibrate.u16ErrCode
                                              | u16WrFloatDataThree(68, bitTemp);
                            }

                            if (Calibrate.u16ErrCode == RECOK)
                            {
                                g_lq10PowerConSysa.lData = Calibrate.lq10SysA;
                                g_lq10PowerConSysb.lData = Calibrate.lq10SysB;

                                Calibrate.u16ErrCode = 0xB0;
                            }

                        }
                    }
                    else
                    {
                        Calibrate.u16ErrCode = 0x0D;//Function Code error
                        Calibrate.u16FunctionCode = 0x00; //Clear Step
                    }

                            Calibrate.lq10K = 1024;
                            Calibrate.lq10B = 0;


                            if((5 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0)||(6 == stRxFrameTemp.DataAll.MBoxDataBits.MDL.byte.BYTE0))
                            {
                                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = (Calibrate.lq10SysA*10L)>>10;
                                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = (Calibrate.lq10SysB*10L)>>10;
                            }
                            else
                            {
                                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = (Calibrate.lq10X2*10L)>>10;
                                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD =(Calibrate.lq10Y2*10L)>>10;
                            }
                }
                else//Step Code error
                {//Need Reset X1
                    Calibrate.u16ErrCode = 0x0C;//Error
                    Calibrate.lq10K = 1024;
                    Calibrate.lq10B = 0;
                    Calibrate.u16FunctionCode = 0x0;
                    Calibrate.u16Step = 0;
                }



//                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.HI_WORD = 0x064;
                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.byte.BYTE0 = Calibrate.u16FunctionCode;
                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.byte.BYTE1 = Calibrate.u16ErrCode;
                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = 0xFFFF;
//                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = Calibrate.lq10SysA;
//                Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = Calibrate.lq10SysB;

                Charge_g_stCanTxData.id.all |= 0xE0000000;
                Charge_sCanWrite3(&Charge_g_stCanTxData);
            break;
		case CHARGE_READ_LIMIT_A8://查询模块限值
			//-------第一帧-----------------------
			Charge_g_stCanTxData.id.all = REPLYID_READ_LIMIT_B8;

			//通信协议版本号为V100
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.HI_WORD = 0x064;
			//最小充电电流值2A ---- 20
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = 0x014;
			//最大充电电流值  18A  ---- 180
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = 0x0B4;
			//最大放电电流值10A ---100
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = 0x064;

			Charge_g_stCanTxData.id.all |= 0xE0000000;
			Charge_sCanWrite3(&Charge_g_stCanTxData);
			//-----------第二帧----------------
			Charge_g_stCanTxData.id.all = REPLYID_READ_LIMIT_B9;

			//DCSW Ver  *100
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.HI_WORD = g_u16VersionNoSw;

			//最低输出电压250V----2500
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = 0x09C4;
			//最高输出电压450V----4500
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = 0x1194;
			//输出恒功率最低电压/恒流最高电压366V ---- 3660
			Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = 0x0E4C;
			break;
		default:
			Charge_g_stCanTxData.id.all = 0;//禁止回复
			break;
	}

	if(0 != Charge_g_stCanTxData.id.all)
	{
		Charge_g_stCanTxData.id.all |= 0xE0000000;
	}
}

void sEncodeTxData_B2_01_3S(void)
{
	Charge_g_stCanTxData.id.all = REPLYID_3S_B2_1;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.HI_WORD = (g_lq10MdlVolt.lData * 10L) >>10;//366L<<10
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = (g_lq10MdlCurr.lData * 10L) >>10;//10L<<10
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = (g_lq10PfcVoltDisp.lData * 10L) >>10;//400L<<10
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = (g_lq10AcVolt.lData * 10L) >>10;//220L<<10
	Charge_g_stCanTxData.id.all |= 0xE0000000;
}

void sEncodeTxData_B2_02_30S(void)
{
	Charge_g_stCanTxData.id.all = REPLYID_30S_B2_2;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.HI_WORD = _IQ10mpy(g_lq10TempAmbiDisp.lData + TEMP_50C, 10);//0x01F0;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = _IQ10mpy(g_lq10TempPFCDisp.lData + TEMP_50C, 10);//0x2233;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = _IQ10mpy(g_lq10TempDCDisp.lData + TEMP_50C, 10);//0x4455;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = IsrVars._u16ChoiseCon;//0xFFFF;//0x66FF;
	Charge_g_stCanTxData.id.all |= 0xE0000000;
}

void sEncodeTxData_B3_3S_Inner(UINT16 u16WarnStatus)
{
//	UINT16 u16Temp = 0;
	Charge_g_stCanTxData.id.all = REPLYID_STATUS_B3;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.HI_WORD = u16WarnStatus;
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDL.word.LOW_WORD = __f32toui16r((55.0f*PI_2_VALUE + ePLL.f32deltaW_Est)*10.0f);//2*pi*freq
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.HI_WORD = __f32toui16r((ePLL.f32Vamp_Est)*10.0f);
	Charge_g_stCanTxData.DataAll.MBoxDataBits.MDH.word.LOW_WORD = (g_lq10BatCurr.lData * 10L) >>10;
	Charge_g_stCanTxData.id.all |= 0xE0000000;
}

UINT16 vStatusTrans(void)   // Responsible for storing data from CAN messages from Cree Monitor/BMS
{
	UINT16 u16Tmp = 0;


	if(g_u16MdlCtrl.bit.Rec_Inv)
	{
		u16Tmp = u16Tmp | 0x8000;		//REC_INV
	}
	if(g_u16MdlStatus.bit.PhaseUnlock)
	{
		u16Tmp = u16Tmp | 0x4000;		// PLL ERROR
	}
	if(IsrVars._u16DcdcOCP)         // DCDC OCP == Over Current Protection?
	{                               // Also responsible for calling mGreenLedOn if true
		u16Tmp = u16Tmp | 0x2000;
	}
	if(0 == g_u16MdlStatus.bit.SrOn)
	{
		u16Tmp = u16Tmp | 0x1000;//SR OFF
	}
	//------------------2------------------
	if(g_u16MdlStatus.bit.OFFSTAT) //输出侧是否关机
	{
		u16Tmp = u16Tmp | 0x0800;		// rectifier onoff state
	}

	if(g_u16MdlStatus.bit.ACOV || g_u16MdlCtrl.bit.SHORTRELAY)
	{
		u16Tmp = u16Tmp | 0x0400;		// AC IN Relay onoff
	}

	if(mMainRelayCloseCheck())
	{
		;//do nothing
	}
	else
	{
		u16Tmp = u16Tmp | 0x0400;		//AC过压/脱离
		u16Tmp = u16Tmp | 0x0100;		//继电器吸合/断开标志
	}

	if(UNVALID == CputoClaVar.u16WorkMode)
	{
		u16Tmp = u16Tmp | 0x0200;		//workmode unvalid?
	}

	//------------------3------------------
	if(g_u16MdlStatus.bit.DCOV || g_u16MdlStatusH.bit.DCHWOV|| g_u16MdlStatus.bit.HVSDLOCK)
	{
		u16Tmp = u16Tmp | 0x0080;		//OVP
	}

	if(g_u16MdlStatus.bit.PFCOV) 		// PFC fault Add PFCFAIL
	{
		u16Tmp = u16Tmp | 0x0040;
	}

	if(g_u16MdlStatus.bit.ACOV)			// AC over volt
	{
		u16Tmp = u16Tmp | 0x0020;
	}

	if(g_u16MdlStatus.bit.AMBIOT || g_u16MdlStatusH.bit.DCOT || g_u16MdlStatusH.bit.PFCOT
	    ||g_u16MdlStatusH.bit.SROT )
	{
		u16Tmp = u16Tmp | 0x0010;//OTP
	}

	//------------------4------------------
	if(g_u16LimitStatus & POW_LIM_AC)
	{
		u16Tmp = u16Tmp | 0x0008;		// power limit status for AC
	}

	if(g_u16LimitStatus & POW_LIM_TEMP)
	{
		u16Tmp = u16Tmp | 0x0004;		// power limit status for temp
	}

	if(g_u16LimitStatus & POW_LIM)
	{
		u16Tmp = u16Tmp | 0x0002;		// power limit warm
	}

	if((CanaRegs.CAN_ERRC.bit.REC > 10) || (CanaRegs.CAN_ERRC.bit.TEC > 10))
	{
		u16Tmp = u16Tmp | 0x0001;		// CAN error
	}

	return u16Tmp;
}

