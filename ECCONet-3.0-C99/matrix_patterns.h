/**
  ******************************************************************************
  * @file    		matrix_patterns.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		May 2017
  * @brief   		ESG Matrix expression region definitions and enumerations.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  *	OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __MATRIX_EXPRESSION_REGIONS_H
#define __MATRIX_EXPRESSION_REGIONS_H

#include <stdint.h>
#include <stdbool.h>


//	PLEASE READ:
//	================================================================================
//	Bits 9~12 of the 16-bit expression enumeration identify the enumeration region. The
//	lightbar region is 4096 bytes and all others are 512 bytes.
//
//	The enumeration region may be used by input devices to know what sequencers are
//	online.
//
//	The first 25% of each region is reserved for nondescript expressions that are
//	typically created by GUI.
//
//	Pattern enumeration zero (0) is reserved to stop a sequencer.
//


//	This region is for non-named indexed lightbar expressions.
//	Index zero is always stops the token sequencer.
//
#define Region_Base__Lightbar_Indexed_Patterns				    					    1
#define Region_Size__Lightbar_Indexed_Patterns					  					 1023

//	This region is for named lightbar expressions.
//
#define Region_Base__Lightbar_Named_Patterns					    					 1024
#define Region_Size__Lightbar_Named_Patterns										  	 3072


//	This region is for non-named indexed Safety Director expressions.
//	Index zero is always stops the token sequencer.
//
#define Region_Base__SafetyDir_Indexed_Patterns				    					 4096
#define Region_Size__SafetyDir_Indexed_Patterns					  					  128

//	This region is for named Safety Director expressions.
//
#define Region_Base__SafetyDir_Named_Patterns					    					 4224
#define Region_Size__SafetyDir_Named_Patterns										  	  384


//	This region is for non-named indexed Sound expressions.
//	Index zero is always stops the token sequencer.
//
#define Region_Base__Sound_Indexed_Patterns				    							 4608
#define Region_Size__Sound_Indexed_Patterns							  					  128

//	This region is for named Sound expressions.
//
#define Region_Base__Sound_Named_Patterns					 			   					 4736
#define Region_Size__Sound_Named_Patterns												  	  384


//	This region is reserved.
//
#define Region_Base__Reserved_Patterns						    							 5120
#define Region_Size__Reserved_Patterns							  					  	 2560


//	This region is for non-named indexed misc. expressions.
//	Index zero is always stops the token sequencer.
//
#define Region_Base__Misc_Indexed_Patterns						    					 7680
#define Region_Size__Misc_Indexed_Patterns							  					  128

//	This region is for named misc. expressions.
//
#define Region_Base__Misc_Named_Patterns					  		  					 7808
#define Region_Size__Misc_Named_Patterns												  	  384



//	PLEASE READ:
//	================================================================================
//	Bits 9~12 of the 16-bit expression enumeration identify the enumeration region. The
//	lightbar region is 4096 bytes and all others are 512 bytes.
//
//	The enumeration region may be used by input devices to know what sequencers are
//	online.
//
//	The first 25% of each region is reserved for nondescript expressions that are
//	typically created by GUI.
//
//	Pattern enumeration zero (0) is reserved to stop a sequencer.
//
typedef enum
{
	ExpressionCategoryLightbar,
	ExpressionCategorySafetyDir,
	ExpressionCategorySound,
	ExpressionCategoryMisc,
	
} EXPRESSION_CATEGORY;


//	ESG 3.0 Expression Enumerations.
typedef enum
{
	//	expression enumeration zero is token sequencer stop 
	Pattern_Stop = 0,
	
	//	first indexed lightbar expressions
	Lightbar_Indexed = Region_Base__Lightbar_Indexed_Patterns,
	
	//	named ECCO lightbar expressions
	Lightbar_CycleAll = Region_Base__Lightbar_Named_Patterns,
	Lightbar_Random,
	Lightbar_Steady_PWM_50,
	Lightbar_Double_75_FPM,
	Lightbar_Title_13_Quad_65_FPM,
	Lightbar_Title_13_Double_65_FPM,
	Lightbar_Quint_Hold_75_FPM,
	Lightbar_Pulse_8_Burst_75_FPM,
	Lightbar_Reg_65_Single_120_FPM,
	Lightbar_Reg_65_Double_120_FPM,
	Lightbar_Reg_65_Triple_120_FPM,
	Lightbar_Reg_65_Quad_120_FPM,
	Lightbar_Reg_65_Burst_120_FPM,
	Lightbar_Reg_65_Single_S_S_120_FPM,
	Lightbar_Reg_65_Double_S_S_120_FPM,
	Lightbar_Reg_65_Triple_S_S_120_FPM,
	Lightbar_Reg_65_Quad_S_S_120_FPM,
	Lightbar_Reg_65_Burst_S_S_120_FPM,
	Lightbar_Quad_Alternate_S_S_150_FPM,
	Lightbar_Quad_Cross_Alternate_150_FPM,
	Lightbar_Double_Alternate_S_S_150_FPM,
	Lightbar_Double_Cross_Alternate_150_FPM,
	Lightbar_Quint_Hold_Alternate_S_S_150_FPM,
	Lightbar_Quint_Hold_Cross_Alternate,
	Lightbar_Quad_Alternate_S_S_150_FPM_Front,
	Lightbar_Quad_Alternate_S_S_150_FPM_Rear,
	Lightbar_Double_Alternate_S_S_150_FPM_Front,
	Lightbar_Double_Alternate_S_S_150_FPM_Rear,
	Lightbar_Quint_Hold_Alternate_Side_to_Side_Front,
	Lightbar_Quint_Hold_Alternate_Side_to_Side_Rear,
	Lightbar_Reg_65_Single_S_S_120_FPM_Center_Pulse,
	Lightbar_Reg_65_Double_S_S_120_FPM_Center_Pulse,
	Lightbar_Reg_65_Triple_S_S_120_FPM_Center_Pulse,
	Lightbar_Reg_65_Quad_S_S_120_FPM_Center_Pulse,
	Lightbar_Reg_65_Burst_S_S_120_FPM_Center_Pulse,
	Lightbar_Quad_Alternate_S_S_150_FPM_Center_Pulse,
	Lightbar_Double_Alternate_S_S_150_FPM_Center_Pulse,
	Lightbar_Quint_Hold_Alternate_S_S_150_FPM_Center_Pulse,
	Lightbar_Quad_Alternate_S_S_150_FPM_Front_Center_Pulse,
	Lightbar_Quad_Alternate_S_S_150_FPM_Rear_Center_Pulse,
	Lightbar_Double_Alternate_S_S_150_FPM_Front_Center_Pulse,
	Lightbar_Double_Alternate_S_S_150_FPM_Rear_Center_Pulse,
	Lightbar_Quint_Hold_Alternate_Side_to_Side_Front_Center_Pulse,
	Lightbar_Quint_Hold_Alternate_Side_to_Side_Rear_Center_Pulse,
	Lightbar_Quad_Alternate_S_S_Middle_75_FPM,
	Lightbar_Quad_Alternate_S_S_Middle_75_FPM_Center_Pulse,
	Lightbar_Fast_Rotate,
	Lightbar_Wave_Rotate,
	Lightbar_Rotate_Quad,
	Lightbar_Fast_Quad,	

	//	named Code3 lightbar expressions
	Lightbar_Single_Flash_75,
	Lightbar_Single_Flash_150,
	Lightbar_Single_Flash_250,
	Lightbar_Single_Flash_375,
	Lightbar_Double_Flash_75,
	Lightbar_Double_Flash_115,
	Lightbar_Double_Flash_150,
	Lightbar_Triple_Flash_60,
	Lightbar_Triple_Flash_115,
	Lightbar_Triple_Flash_150,
	Lightbar_Quad_Flash_75_NFPA,
	Lightbar_Quad_Flash_115,
	Lightbar_Quad_Flash_150,
	Lightbar_Five_Flash_115,
	Lightbar_Five_Flash_150,
	Lightbar_Six_Flash_60,
	Lightbar_Six_Flash_80,
	
	Lightbar_CA_Steady_Burn,
	Lightbar_Cruise_Low,
	Lightbar_Cruise_High,
	Lightbar_Flicker_Cruise_Low,
	Lightbar_Flicker_Cruise_High,
	
	Lightbar_360_Combo_1,
	Lightbar_Intersection,
	Lightbar_All_Bar_Rotate,
	Lightbar_Dual_End_Rotate,
	Lightbar_Pursuit,
	Lightbar_Sweep,
	Lightbar_Hyperflash_1,
	Lightbar_Steady_Burn,
	Lightbar_Variable_Flash,
	Lightbar_Cycle_Flash,

	
	
	//	first indexed Safety Director expression
	SafetyDir_Indexed = Region_Base__SafetyDir_Indexed_Patterns,
	
	//	named ECCO Safety Director expressions
	SafetyDir_Left = Region_Base__SafetyDir_Named_Patterns,
  SafetyDir_Left_Solid,
  SafetyDir_Right,
  SafetyDir_Right_Solid,
  SafetyDir_Center_Out,
  SafetyDir_Center_Out_Solid,
  SafetyDir_Wig_Wag,
  SafetyDir_Alternating,
  SafetyDir_Quad_Flash,
	SafetyDir_Alternating_Center_Pulse,
	SafetyDir_Quad_Flash_Center_Pulse,
	SafetyDir_LeftFillAndChase,
	SafetyDir_RightFillAndChase,
	SafetyDir_CenterFillAndChase,
	SafetyDir_Indexed_Front_Pattern,
	SafetyDir_Indexed_Rear_Pattern,


	//	named Code3 arrow stik expressions
	ArrowStik_Left_Build_Slow,
	ArrowStik_Left_Build_Medium,
	ArrowStik_Left_Build_Fast,
	ArrowStik_Left_Build_Collapse_Slow,
	ArrowStik_Left_Build_Collapse_Medium,
	ArrowStik_Left_Build_Collapse_Fast,
	ArrowStik_Left_Build_3_Flash_Slow,
	ArrowStik_Left_Build_3_Flash_Medium,
	ArrowStik_Left_Build_3_Flash_Fast,
	ArrowStik_Left_Traveling_Ball_3_Flash_Slow,
	ArrowStik_Left_Traveling_Ball_3_Flash_Medium,
	ArrowStik_Left_Traveling_Ball_3_Flash_Fast,

	ArrowStik_Right_Build_Slow,
	ArrowStik_Right_Build_Medium,
	ArrowStik_Right_Build_Fast,
	ArrowStik_Right_Build_Collapse_Slow,
	ArrowStik_Right_Build_Collapse_Medium,
	ArrowStik_Right_Build_Collapse_Fast,
	ArrowStik_Right_Build_3_Flash_Slow,
	ArrowStik_Right_Build_3_Flash_Medium,
	ArrowStik_Right_Build_3_Flash_Fast,
	ArrowStik_Right_Traveling_Ball_3_Flash_Slow,
	ArrowStik_Right_Traveling_Ball_3_Flash_Medium,
	ArrowStik_Right_Traveling_Ball_3_Flash_Fast,

	ArrowStik_Center_Build_Slow,
	ArrowStik_Center_Build_Medium,
	ArrowStik_Center_Build_Fast,
	ArrowStik_Center_Build_Collapse_Slow,
	ArrowStik_Center_Build_Collapse_Medium,
	ArrowStik_Center_Build_Collapse_Fast,
	ArrowStik_Center_Build_3_Flash_Slow,
	ArrowStik_Center_Build_3_Flash_Medium,
	ArrowStik_Center_Build_3_Flash_Fast,
	ArrowStik_Center_Traveling_Ball_3_Flash_Slow,
	ArrowStik_Center_Traveling_Ball_3_Flash_Medium,
	ArrowStik_Center_Traveling_Ball_3_Flash_Fast,

} EXPRESSION_ENUMERATIONS;

/**
  * @brief  Pattern modes.
  */
typedef enum
{
	//	token key prefixes
	PatternMode_MultiKey																= 0x00,
	PatternMode_StepDictionaryKey												= 0x20,
	PatternMode_LedMatrixKey														= 0x40,
	
} PatternMode;

/**
  * @brief  Pattern entry prefixes.
  */
typedef enum
{
	PatternPrefix_PatternWithRepeats										= 0xA0,
	PatternPrefix_PatternStepWithPeriod									= 0xB0,
	PatternPrefix_PatternStepWithRepeatsOfNestedPattern	= 0xC0,
	PatternPrefix_PatternStepWithAllOff									= 0xD0,
	PatternPrefix_PatternSectionStartWithRepeats				= 0xE0,
	PatternPrefix_PatternSectionEnd											= 0xF0,
	PatternPrefix_Mask																	= 0xF0,

} PatternPrefixes;	
#define PatternEnumPrefixMask	0xE0


#endif  //  __MATRIX_EXPRESSION_REGIONS_H


