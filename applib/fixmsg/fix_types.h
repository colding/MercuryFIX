/*
 *    Copyright (C) 2013, Jules Colding <jcolding@gmail.com>.
 *
 *    All Rights Reserved.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     (1) Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer.
 *
 *     (2) Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *     (3) Neither the name of the copyright holder nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <inttypes.h>

/*
 * The CUSTOM version will allow a custom messaging protocol to be
 * build on top of the Mercury FIX engine. A precondition for CUSTOM
 * is that tags 52 and 122 are of the fixed format
 * "YYYYMMDD-HH:MM:SS.sss".
 */
enum FIX_Version : unsigned int
{
	CUSTOM = 0,
        FIX_4_0,
        FIX_4_1,
        FIX_4_2,
        FIX_4_3,
        FIX_4_4,
        FIX_5_0,
        FIX_5_0_SP1,
        FIX_5_0_SP2,
        FIXT_1_1,
        FIX_VERSION_TYPES_COUNT
};

const char fix_version_string[FIX_VERSION_TYPES_COUNT][16] =
{
	"CUSTOM",
        "FIX.4.0",
        "FIX.4.1",
        "FIX.4.2",
        "FIX.4.3",
        "FIX.4.4",
        "FIX.5.0",
        "FIX.5.0.SP1",
        "FIX.5.0.SP2",
        "FIXT.1.1",
};

enum FIX_Type : unsigned int
{
        ft_int = 0,
        ft_Length,
        ft_TagNum,
        ft_SeqNum,
        ft_NumInGroup,
        ft_DayOfMonth,
        ft_float,
        ft_Qty,
        ft_Price,
        ft_PriceOffset,
        ft_Amt,
        ft_Percentage,
        ft_char,
        ft_Boolean,
        ft_String,
        ft_MultipleCharValue,
        ft_MultipleStringValue,
        ft_Country,
        ft_Currency,
        ft_Exchange,
        ft_MonthYear,
        ft_UTCTimestamp, // formerly "time"
        ft_UTCTimeOnly,
        ft_UTCDateOnly,
        ft_LocalMktDate, // formerly "date"
        ft_TZTimeOnly,
        ft_TZTimestamp,
        ft_data,
        ft_Pattern,
        ft_Tenor,
        ft_Reserved100Plus,
        ft_Reserved1000Plus,
        ft_Reserved4000Plus,
        ft_XMLData,
        ft_Language,
};

struct FIX_Tag {
        unsigned int tag;
        FIX_Type type;
};

/*
 * Standard tags and their types
 */
extern const struct FIX_Tag fix40_std_tags[];
extern const struct FIX_Tag fix41_std_tags[];
extern const struct FIX_Tag fix42_std_tags[];
extern const struct FIX_Tag fix43_std_tags[];
extern const struct FIX_Tag fix44_std_tags[];
extern const struct FIX_Tag fix50_std_tags[];
extern const struct FIX_Tag fix50sp1_std_tags[];
extern const struct FIX_Tag fix50sp2_std_tags[];
extern const struct FIX_Tag fixt11_std_tags[];

/*
 * Standard data tags
 */
extern const struct FIX_Tag fix40_std_data_tags[];
extern const struct FIX_Tag fix41_std_data_tags[];
extern const struct FIX_Tag fix42_std_data_tags[];
extern const struct FIX_Tag fix43_std_data_tags[];
extern const struct FIX_Tag fix44_std_data_tags[];
extern const struct FIX_Tag fix50_std_data_tags[];
extern const struct FIX_Tag fix50sp1_std_data_tags[];
extern const struct FIX_Tag fix50sp2_std_data_tags[];
extern const struct FIX_Tag fixt11_std_data_tags[];

enum FIX_MsgType : uint_fast32_t
{
        fmt_CustomMsg = 0, // hmm, maybe I really don't need this?
        fmt_Heartbeat,
        fmt_TestRequest,
        fmt_ResendRequest,
        fmt_Reject,
        fmt_SequenceReset,
        fmt_Logout,
        fmt_IOI,
        fmt_Advertisement,
        fmt_ExecutionReport,
        fmt_OrderCancelReject,
        fmt_Logon,
        fmt_DerivativeSecurityList,
        fmt_NewOrderMultileg,
        fmt_MultilegOrderCancelReplace,
        fmt_TradeCaptureReportRequest,
        fmt_TradeCaptureReport,
        fmt_OrderMassStatusRequest,
        fmt_QuoteRequestReject,
        fmt_RFQRequest,
        fmt_QuoteStatusReport,
        fmt_QuoteResponse,
        fmt_Confirmation,
        fmt_PositionMaintenanceRequest,
        fmt_PositionMaintenanceReport,
        fmt_RequestForPositions,
        fmt_RequestForPositionsAck,
        fmt_PositionReport,
        fmt_TradeCaptureReportRequestAck,
        fmt_TradeCaptureReportAck,
        fmt_AllocationReport,
        fmt_AllocationReportAck,
        fmt_ConfirmationAck,
        fmt_SettlementInstructionRequest,
        fmt_AssignmentReport,
        fmt_CollateralRequest,
        fmt_CollateralAssignment,
        fmt_CollateralResponse,
        fmt_News,
        fmt_CollateralReport,
        fmt_CollateralInquiry,
        fmt_NetworkCounterpartySystemStatusRequest,
        fmt_NetworkCounterpartySystemStatusResponse,
        fmt_UserRequest,
        fmt_UserResponse,
        fmt_CollateralInquiryAck,
        fmt_ConfirmationRequest,
        fmt_TradingSessionListRequest,
        fmt_TradingSessionList,
        fmt_SecurityListUpdateReport,
        fmt_AdjustedPositionReport,
        fmt_AllocationInstructionAlert,
        fmt_ExecutionAcknowledgement,
        fmt_ContraryIntentionReport,
        fmt_SecurityDefinitionUpdateReport,
        fmt_SettlementObligationReport,
        fmt_DerivativeSecurityListUpdateReport,
        fmt_TradingSessionListUpdateReport,
        fmt_MarketDefinitionRequest,
        fmt_MarketDefinition,
        fmt_MarketDefinitionUpdateReport,
        fmt_ApplicationMessageRequest,
        fmt_ApplicationMessageRequestAck,
        fmt_ApplicationMessageReport,
        fmt_OrderMassActionReport,
        fmt_Email,
        fmt_OrderMassActionRequest,
        fmt_UserNotification,
        fmt_StreamAssignmentRequest,
        fmt_StreamAssignmentReport,
        fmt_StreamAssignmentReportACK,
        fmt_NewOrderSingle,
        fmt_NewOrderList,
        fmt_OrderCancelRequest,
        fmt_OrderCancelReplaceRequest,
        fmt_OrderStatusRequest,
        fmt_AllocationInstruction,
        fmt_ListCancelRequest,
        fmt_ListExecute,
        fmt_ListStatusRequest,
        fmt_ListStatus,
        fmt_AllocationInstructionAck,
        fmt_DontKnowTrade,
        fmt_QuoteRequest,
        fmt_Quote,
        fmt_SettlementInstructions,
        fmt_MarketDataRequest,
        fmt_MarketDataSnapshotFullRefresh,
        fmt_MarketDataIncrementalRefresh,
        fmt_MarketDataRequestReject,
        fmt_QuoteCancel,
        fmt_QuoteStatusRequest,
        fmt_MassQuoteAcknowledgement,
        fmt_SecurityDefinitionRequest,
        fmt_SecurityDefinition,
        fmt_SecurityStatusRequest,
        fmt_SecurityStatus,
        fmt_TradingSessionStatusRequest,
        fmt_TradingSessionStatus,
        fmt_MassQuote,
        fmt_BusinessMessageReject,
        fmt_BidRequest,
        fmt_BidResponse,
        fmt_ListStrikePrice,
        fmt_XMLnonFIX,
        fmt_RegistrationInstructions,
        fmt_RegistrationInstructionsResponse,
        fmt_OrderMassCancelRequest,
        fmt_OrderMassCancelReport,
        fmt_NewOrderCross,
        fmt_CrossOrderCancelReplaceRequest,
        fmt_CrossOrderCancelRequest,
        fmt_SecurityTypeRequest,
        fmt_SecurityTypes,
        fmt_SecurityListRequest,
        fmt_SecurityList,
        fmt_DerivativeSecurityListRequest,
        FIX_MSGTYPES_COUNT
};

/*
 * It is absolutely *REQUIRED* that this array is in the same order as
 * FIX_MsgType above.
 */
const char fix_msgtype_string[FIX_MSGTYPES_COUNT][sizeof(uint_fast32_t)+1] =
{
        "",   /* CustomMsg */
        "0",  /* Heartbeat */
        "1",  /* TestRequest */
        "2",  /* ResendRequest */
        "3",  /* Reject */
        "4",  /* SequenceReset */
        "5",  /* Logout */
        "6",  /* IOI */
        "7",  /* Advertisement */
        "8",  /* ExecutionReport */
        "9",  /* OrderCancelReject */
        "A",  /* Logon */
        "AA", /* DerivativeSecurityList */
        "AB", /* NewOrderMultileg */
        "AC", /* MultilegOrderCancelReplace */
        "AD", /* TradeCaptureReportRequest */
        "AE", /* TradeCaptureReport */
        "AF", /* OrderMassStatusRequest */
        "AG", /* QuoteRequestReject */
        "AH", /* RFQRequest */
        "AI", /* QuoteStatusReport */
        "AJ", /* QuoteResponse */
        "AK", /* Confirmation */
        "AL", /* PositionMaintenanceRequest */
        "AM", /* PositionMaintenanceReport */
        "AN", /* RequestForPositions */
        "AO", /* RequestForPositionsAck */
        "AP", /* PositionReport */
        "AQ", /* TradeCaptureReportRequestAck */
        "AR", /* TradeCaptureReportAck */
        "AS", /* AllocationReport */
        "AT", /* AllocationReportAck */
        "AU", /* ConfirmationAck */
        "AV", /* SettlementInstructionRequest */
        "AW", /* AssignmentReport */
        "AX", /* CollateralRequest */
        "AY", /* CollateralAssignment */
        "AZ", /* CollateralResponse */
        "B",  /* News */
        "BA", /* CollateralReport */
        "BB", /* CollateralInquiry */
        "BC", /* NetworkCounterpartySystemStatusRequest */
        "BD", /* NetworkCounterpartySystemStatusResponse */
        "BE", /* UserRequest */
        "BF", /* UserResponse */
        "BG", /* CollateralInquiryAck */
        "BH", /* ConfirmationRequest */
        "BI", /* TradingSessionListRequest */
        "BJ", /* TradingSessionList */
        "BK", /* SecurityListUpdateReport */
        "BL", /* AdjustedPositionReport */
        "BM", /* AllocationInstructionAlert */
        "BN", /* ExecutionAcknowledgement */
        "BO", /* ContraryIntentionReport */
        "BP", /* SecurityDefinitionUpdateReport */
        "BQ", /* SettlementObligationReport */
        "BR", /* DerivativeSecurityListUpdateReport */
        "BS", /* TradingSessionListUpdateReport */
        "BT", /* MarketDefinitionRequest */
        "BU", /* MarketDefinition */
        "BV", /* MarketDefinitionUpdateReport */
        "BW", /* ApplicationMessageRequest */
        "BX", /* ApplicationMessageRequestAck */
        "BY", /* ApplicationMessageReport */
        "BZ", /* OrderMassActionReport */
        "C",  /* Email */
        "CA", /* OrderMassActionRequest */
        "CB", /* UserNotification */
        "CC", /* StreamAssignmentRequest */
        "CD", /* StreamAssignmentReport */
        "CE", /* StreamAssignmentReportACK */
        "D",  /* NewOrderSingle */
        "E",  /* NewOrderList */
        "F",  /* OrderCancelRequest */
        "G",  /* OrderCancelReplaceRequest */
        "H",  /* OrderStatusRequest */
        "J",  /* AllocationInstruction */
        "K",  /* ListCancelRequest */
        "L",  /* ListExecute */
        "M",  /* ListStatusRequest */
        "N",  /* ListStatus */
        "P",  /* AllocationInstructionAck */
        "Q",  /* DontKnowTrade */
        "R",  /* QuoteRequest */
        "S",  /* Quote */
        "T",  /* SettlementInstructions */
        "V",  /* MarketDataRequest */
        "W",  /* MarketDataSnapshotFullRefresh */
        "X",  /* MarketDataIncrementalRefresh */
        "Y",  /* MarketDataRequestReject */
        "Z",  /* QuoteCancel */
        "a",  /* QuoteStatusRequest */
        "b",  /* MassQuoteAcknowledgement */
        "c",  /* SecurityDefinitionRequest */
        "d",  /* SecurityDefinition */
        "e",  /* SecurityStatusRequest */
        "f",  /* SecurityStatus */
        "g",  /* TradingSessionStatusRequest */
        "h",  /* TradingSessionStatus */
        "i",  /* MassQuote */
        "j",  /* BusinessMessageReject */
        "k",  /* BidRequest */
        "l",  /* BidResponse */
        "m",  /* ListStrikePrice */
        "n",  /* XMLnonFIX */
        "o",  /* RegistrationInstructions */
        "p",  /* RegistrationInstructionsResponse */
        "q",  /* OrderMassCancelRequest */
        "r",  /* OrderMassCancelReport */
        "s",  /* NewOrderCross */
        "t",  /* CrossOrderCancelReplaceRequest */
        "u",  /* CrossOrderCancelRequest */
        "v",  /* SecurityTypeRequest */
        "w",  /* SecurityTypes */
        "x",  /* SecurityListRequest */
        "y",  /* SecurityList */
        "z",  /* DerivativeSecurityListRequest */
};
extern FIX_MsgType get_fix_msgtype(const char soh,
                                   const char * const str); // first byte in message type value field

const FIX_MsgType fix_session_message_types[]
{
        fmt_Heartbeat,
        fmt_TestRequest,
        fmt_ResendRequest,
        fmt_Reject,
        fmt_SequenceReset,
        fmt_Logout,
        fmt_Logon,
	fmt_XMLnonFIX,
	fmt_CustomMsg // terminating the array
};
#define FIX_SESSION_MSGTYPES_COUNT (sizeof(fix_session_message_types)/sizeof(FIX_MsgType))

const char fix_session_msgtype_string[FIX_SESSION_MSGTYPES_COUNT][sizeof(uint_fast32_t)+1] =
{
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "A",
        "n"
};

extern int is_session_message(const FIX_MsgType type);

extern int is_session_message(const char soh,
                              const char * const str); // first byte in message type value field
