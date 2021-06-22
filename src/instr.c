#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#pragma warning(disable : 4996)

#include "instr.h"

#include "jansson.h"

// Visa Parameters
static ViStatus visaStatus;                         // Error checking
static ViSession visaDefault, visaInstr;            // Instrument handler
static ViUInt32 visaNumInstrs;                      // Number of found instruments
static ViFindList visaFoundInstrList;               // Found instruments
static ViChar visaFoundInstrBuffer[VI_FIND_BUFLEN]; // find buffer
static char visaWriteBuffer[512];                   // write buffer
static ViUInt32 visaWriteCount;                     // write count
static ViUInt32 visaRetCount;                       // read return count,
static unsigned char visaReadBuffer[100000];        // read buffer

int instr_connect(json_t *obj)
{
  visaStatus = viOpenDefaultRM(&visaDefault);
  if (visaStatus < VI_SUCCESS)
    return 0;

  visaStatus = viFindRsrc(visaDefault, "?*", &visaFoundInstrList, &visaNumInstrs, visaFoundInstrBuffer);
  if (visaStatus == VI_ERROR_RSRC_NFOUND)
  {
    json_object_set_new(obj, "instr", json_integer(INSTR_DEV_ERROR));
    json_object_set_new(obj, "err", json_string("VI_ERROR_RSRC_NFOUND"));
    printf("Resource not found\n");
    return 0;
  }
  else if (visaStatus < VI_SUCCESS)
  {
    json_object_set_new(obj, "instr", json_integer(INSTR_DEV_ERROR));
    json_object_set_new(obj, "err", json_string("VI_ERROR"));
    return 0;
  }

  for (unsigned i = 0; i < visaNumInstrs; i++)
  {
    printf("Instrument : %d - %s ... ", i, visaFoundInstrBuffer);
    visaStatus = viOpen(visaDefault, visaFoundInstrBuffer, VI_NULL, VI_NULL, &visaInstr);
    if (visaStatus < VI_SUCCESS)
    {
      printf("Not avaliable.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    printf("Connected.\n");
    visaStatus = viSetAttribute(visaInstr, VI_ATTR_TMO_VALUE, 5000);

    printf("Status : ");
    strcpy(visaWriteBuffer, "*IDN?");
    visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
    if (visaStatus < VI_SUCCESS)
    {
      printf("Not supported.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    visaStatus = viRead(visaInstr, visaReadBuffer, 100000, &visaRetCount);
    if (visaStatus < VI_SUCCESS)
    {
      printf("Not supported.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    if (strstr(visaReadBuffer, "MS20") == NULL)
    {
      printf("Not supported.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    printf("Supported - %s.\n", visaReadBuffer);
    break;
  }
  // tolower
  return 1;
}

void instr_disconnect()
{
  visaStatus = viClose(visaFoundInstrList);
  visaStatus = viClose(visaInstr);
  visaStatus = viClose(visaDefault);
}

int instr_info(json_t *obj)
{
  json_object_set_new(obj, "instr", json_integer(INSTR_DEV_INFO));
  json_object_set_new(obj, "mode", json_integer(INSTR_MODE_VNA));
  strcpy(visaWriteBuffer, "*IDN?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, 100000, &visaRetCount);
  json_object_set_new(obj, "id", json_string(visaReadBuffer));
  json_object_set_new(obj, "ui", json_string(instr_ui));
  return 1;
}

// int instr_conf(json_t *obj)
// {
//   // SET
//   for (int i = 0; i < INSTR_MAX_QUERY_SIZE; i++)
//   {
//     if (instr_queries[i] != NULL)
//     {
//       const char *key;
//       json_t *value;
//       json_object_foreach(instr_queries[i], key, value)
//       {
//         // Stimulus
//         if (strcmp(key, "SENS:FREQ:STAR") == 0)
//           pNWA->SCPI->SENSe[active_channel]->FREQuency->STARt = atof(json_string_value(value));
//         else if (strcmp(key, "SENS:FREQ:STOP") == 0)
//           pNWA->SCPI->SENSe[active_channel]->FREQuency->STOP = atof(json_string_value(value));
//         else if (strcmp(key, "SENS:SWE:POIN") == 0)
//           pNWA->SCPI->SENSe[active_channel]->SWEep->POINts = atol(json_string_value(value));
//         // Response
//         else if (strcmp(key, "CALC:FORM") == 0)
//           pNWA->SCPI->CALCulate[active_channel]->SELected->FORMat = json_string_value(value);
//         else if (strcmp(key, "CALC:PAR:DEF") == 0)
//           pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->DEFine = json_string_value(value);
//         // Scale
//         else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:PDIV") == 0)
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->PDIVision = atof(json_string_value(value));
//         else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:RLEV") == 0)
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RLEVel = atof(json_string_value(value));
//         else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:RPOS") == 0)
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RPOSition = atol(json_string_value(value));
//         else if (strcmp(key, "DISP:WIND:Y:SCAL:DIV") == 0)
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->Y->SCALe->DIVisions = atol(json_string_value(value));
//         else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:AUTO") == 0)
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->AUTO();
//         // Channels
//         else if (strcmp(key, "CALC") == 0)
//         {
//           if (atol(json_string_value(value)) == 3)
//             pNWA->SCPI->DISPlay->SPLit = 4;
//           else if (atol(json_string_value(value)) == 4)
//             pNWA->SCPI->DISPlay->SPLit = 6;
//           else if (atol(json_string_value(value)) == 6)
//             pNWA->SCPI->DISPlay->SPLit = 8;
//           else if (atol(json_string_value(value)) == 8)
//             pNWA->SCPI->DISPlay->SPLit = 9;
//           else if (atol(json_string_value(value)) == 9)
//             pNWA->SCPI->DISPlay->SPLit = 10;
//           else
//             pNWA->SCPI->DISPlay->SPLit = atol(json_string_value(value));
//           // check active channel
//           if (active_channel > atol(json_string_value(value)))
//             active_channel = atol(json_string_value(value));
//           // check active trace
//           if (active_trace > pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt)
//           {
//             active_trace = pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt;
//             pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->SELect();
//             pNWA->SCPI->DISPlay->WINDow[active_channel]->MAXimize;
//           }
//         }
//         else if (strcmp(key, "CALC:ACT") == 0)
//         {
//           if (atol(json_string_value(value)) <= channel_count)
//             active_channel = atol(json_string_value(value));
//           else
//             active_channel = channel_count;
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->ACTivate();
//           pNWA->SCPI->DISPlay->MAXimize;
//           // check active trace
//           if (active_trace > pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt)
//           {
//             active_trace = 1;
//             pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->SELect();
//             pNWA->SCPI->DISPlay->WINDow[active_channel]->MAXimize;
//           }
//         }
//         else if (strcmp(key, "CALC:PAR:COUN") == 0)
//           pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt = atol(json_string_value(value));
//         else if (strcmp(key, "CALC:PAR:SEL") == 0)
//         {
//           if (atol(json_string_value(value)) <= trace_count)
//             active_trace = atol(json_string_value(value));
//           else
//             active_trace = trace_count;
//           pNWA->SCPI->CALCulate[active_channel]->PARameter[atol(json_string_value(value))]->SELect();
//           pNWA->SCPI->DISPlay->WINDow[active_channel]->MAXimize;
//         }
//         // printf("%s: %lu\n", key, atol(json_string_value(value)));
//       }
//       json_decref(instr_queries[i]);
//       instr_queries[i] = NULL;
//     }
//     else
//       break;
//   }

//   // GET
//   json_object_set_new(obj, "instr", json_integer(INSTR_DEV_CONF));
//   // Stimulus
//   json_object_set_new(obj, "SENS:FREQ:STAR", json_real(pNWA->SCPI->SENSe[active_channel]->FREQuency->STARt));
//   json_object_set_new(obj, "SENS:FREQ:STOP", json_real(pNWA->SCPI->SENSe[active_channel]->FREQuency->STOP));
//   json_object_set_new(obj, "SENS:SWE:POIN", json_integer(pNWA->SCPI->SENSe[active_channel]->SWEep->POINts));
//   // Response
//   json_object_set_new(obj, "CALC:FORM", json_string(pNWA->SCPI->CALCulate[active_channel]->SELected->FORMat));
//   json_object_set_new(obj, "CALC:PAR:DEF", json_string(pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->DEFine));
//   // Scale
//   json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:PDIV", json_real(pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->PDIVision));
//   json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:RLEV", json_real(pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RLEVel));
//   json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:RPOS", json_integer(pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RPOSition));
//   json_object_set_new(obj, "DISP:WIND:Y:SCAL:DIV", json_integer(pNWA->SCPI->DISPlay->WINDow[active_channel]->Y->SCALe->DIVisions));
//   // Channels
//   long split_mode = pNWA->SCPI->DISPlay->SPLit;
//   if (split_mode == 4)
//     channel_count = 3;
//   else if (split_mode == 6)
//     channel_count = 4;
//   else if (split_mode == 8)
//     channel_count = 6;
//   else if (split_mode == 9)
//     channel_count = 8;
//   else if (split_mode == 10)
//     channel_count = 9;
//   else
//     channel_count = split_mode;
//   json_object_set_new(obj, "CALC", json_integer(channel_count));
//   active_channel = pNWA->SCPI->SERVice->CHANnel[1]->ACTive;
//   json_object_set_new(obj, "CALC:ACT", json_integer(active_channel));
//   // Traces
//   trace_count = pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt;
//   json_object_set_new(obj, "CALC:PAR:COUN", json_integer(trace_count));
//   active_trace = pNWA->SCPI->SERVice->CHANnel[active_channel]->TRACe->ACTive;
//   json_object_set_new(obj, "CALC:PAR:SEL", json_integer(active_trace));
//   return 1;
// }

// void instr_add_query(json_t *obj)
// {
//   for (int i = 0; i < INSTR_MAX_QUERY_SIZE; ++i)
//   {
//     if (instr_queries[i] == NULL)
//     {
//       instr_queries[i] = obj;
//       break;
//     }
//   }
// }

// int instr_data(json_t *obj)
// {
//   json_t *channels = NULL, *traces = NULL, *trace = NULL, *x = NULL, *y1 = NULL, *y2 = NULL;
//   json_object_set_new(obj, "instr", json_integer(INSTR_DEV_DATA));

//   channels = json_array();
//   traces = json_array();
//   trace = json_object();

//   Data = pNWA->SCPI->CALCulate[active_channel]->SELected->DATA->FDATa;
//   Freq = pNWA->SCPI->SENSe[active_channel]->FREQuency->DATA;
//   CComSafeArray<double> mSafeArray;
//   CComSafeArray<double> FreqArray;
//   if (FreqArray.Attach(Freq.parray) == S_OK && mSafeArray.Attach(Data.parray) == S_OK)
//   {
//     x = json_array();
//     y1 = json_array();
//     for (unsigned int n = 0; n < FreqArray.GetCount(); n++)
//     {
//       json_array_append_new(x, json_real(FreqArray.GetAt(n)));
//       json_array_append_new(y1, json_real(mSafeArray.GetAt(2 * n)));
//     }
//     mSafeArray.Detach();
//     FreqArray.Detach();
//     json_object_set_new(trace, "x", x);
//     json_object_set_new(trace, "y1", y1);
//   }
//   json_array_append_new(traces, trace);
//   json_array_append_new(channels, traces);
//   json_object_set_new(obj, "channels", channels);
//   return 1;
// }