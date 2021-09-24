#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#pragma warning(disable : 4996)
#include <time.h>
void timestamp()
{
  time_t ltime;       /* calendar time */
  ltime = time(NULL); /* get current cal time */
  printf("%s\n", asctime(localtime(&ltime)));
}

#include "instr.h"

#include "jansson.h"

// Visa Parameters
static ViStatus visaStatus;                         // Error checking
static ViSession visaDefault, visaInstr;            // Instrument handler
static ViUInt32 visaNumInstrs;                      // Number of found instruments
static ViFindList visaFoundInstrList;               // Found instruments
static ViChar visaFoundInstrBuffer[VI_FIND_BUFLEN]; // find buffer
static ViChar visaStatusErrorString[512];           // find buffer
static char visaWriteBuffer[512];                   // write buffer
static ViUInt32 visaWriteCount;                     // write count
static ViUInt32 visaRetCount;                       // read return count,
static unsigned visaReadBufferSize = 1000000;
static unsigned char visaReadBuffer[1000000]; // read buffer

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

  unsigned int foundSupported = 0;
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
    visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
    if (visaStatus < VI_SUCCESS)
    {
      printf("Not supported.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    if (strstr(visaReadBuffer, "MS20") == NULL && strstr(visaReadBuffer, "ms20") == NULL)
    {
      printf("Not supported.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    else
    {
      foundSupported = 1;
      printf("Supported - %s.\n", visaReadBuffer);
      break;
    }
  }
  if (foundSupported == 0)
  {
    json_object_set_new(obj, "instr", json_integer(INSTR_DEV_ERROR));
    json_object_set_new(obj, "err", json_string("Device could not be fetched or model is not supported."));
  }
  return foundSupported;
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
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "id", json_string(visaReadBuffer));
  return 1;
}

int instr_conf(json_t *obj)
{
  // SET
  for (int i = 0; i < INSTR_MAX_QUERY_SIZE; i++)
  {
    if (instr_queries[i] != NULL)
    {
      const char *key;
      json_t *value;
      json_object_foreach(instr_queries[i], key, value)
      {
        // Freq/Time/Dist
        if (strcmp(key, "SENS:FREQ:STAR") == 0)
        {
          sprintf(visaWriteBuffer, "%s %f", ":SENSe:FREQuency:STARt", atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "SENS:FREQ:STOP") == 0)
        {
          sprintf(visaWriteBuffer, "%s %f", ":SENSe:FREQuency:STOP", atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "SENS:FREQ:CENT") == 0)
        {
          sprintf(visaWriteBuffer, "%s %f", ":SENSe:FREQuency:CENTer", atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "SENS:FREQ:SPAN") == 0)
        {
          sprintf(visaWriteBuffer, "%s %f", ":SENSe:FREQuency:SPAN", atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        // Sweep
        else if (strcmp(key, "SENS:SWE:POIN") == 0)
        {
          sprintf(visaWriteBuffer, "%s %d", ":SENSe:SWEep:POINts", atol(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "SENS:BAND:RES") == 0)
        {
          sprintf(visaWriteBuffer, "%s %d", ":SENSe:SWEep:IFBW", atol(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
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
      }
      json_decref(instr_queries[i]);
      instr_queries[i] = NULL;
    }
    else
      break;
  }

  // GET
  json_object_set_new(obj, "instr", json_integer(INSTR_DEV_CONF));
  // Frequency/Time/Distance
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:STARt?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:STAR", json_real(atof(json_string_value(json_stringn(visaReadBuffer, visaRetCount)))));
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:STOP?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:STOP", json_real(atof(json_string_value(json_stringn(visaReadBuffer, visaRetCount)))));
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:CENTer?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:CENT", json_real(atof(json_string_value(json_stringn(visaReadBuffer, visaRetCount)))));
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:SPAN?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:SPAN", json_real(atof(json_string_value(json_stringn(visaReadBuffer, visaRetCount)))));

  // Sweep
  strcpy(visaWriteBuffer, ":SENSe:SWEep:POINts?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "SENS:SWE:POIN", json_integer(atol(json_string_value(json_stringn(visaReadBuffer, visaRetCount)))));
  strcpy(visaWriteBuffer, ":SENSe:SWEep:IFBW?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  json_object_set_new(obj, "SENS:BAND:RES", json_real(atof(json_string_value(json_stringn(visaReadBuffer, visaRetCount)))));

  // json_object_set_new(obj, "ui", json_string(instr_ui));
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

  // UI
  json_t *cat, *subcat, *cats, *subcats, *options;
  cats = json_array();
  // Freq/Time/Dist
  cat = json_object();
  json_object_set_new(cat, "name", json_string("Freq/Time/Dist"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Start"));
  json_object_set_new(subcat, "scpi", json_string("SENS:FREQ:STAR"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Stop"));
  json_object_set_new(subcat, "scpi", json_string("SENS:FREQ:STOP"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Center"));
  json_object_set_new(subcat, "scpi", json_string("SENS:FREQ:CENT"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Span"));
  json_object_set_new(subcat, "scpi", json_string("SENS:FREQ:SPAN"));
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);

  // Freq/Time/Dist
  cat = json_object();
  json_object_set_new(cat, "name", json_string("Sweep"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Data Points"));
  json_object_set_new(subcat, "scpi", json_string("SENS:SWE:POIN"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("IF Bandwidth"));
  json_object_set_new(subcat, "scpi", json_string("SENS:BAND:RES"));
  options = json_array();
  json_array_append_new(options, json_string("100 kHz"));
  json_array_append_new(options, json_string("50 kHz"));
  json_array_append_new(options, json_string("20 kHz"));
  json_array_append_new(options, json_string("10 kHz"));
  json_array_append_new(options, json_string("5 kHz"));
  json_array_append_new(options, json_string("2 kHz"));
  json_array_append_new(options, json_string("1 kHz"));
  json_array_append_new(options, json_string("500 Hz"));
  json_array_append_new(options, json_string("200 Hz"));
  json_array_append_new(options, json_string("100 Hz"));
  json_array_append_new(options, json_string("50 Hz"));
  json_array_append_new(options, json_string("20 Hz"));
  json_array_append_new(options, json_string("10 Hz"));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);

  json_object_set_new(obj, "ui", cats);

  return 1;
}

void instr_add_query(json_t *obj)
{
  for (int i = 0; i < INSTR_MAX_QUERY_SIZE; ++i)
  {
    if (instr_queries[i] == NULL)
    {
      instr_queries[i] = obj;
      break;
    }
  }
}

int instr_data(json_t *obj)
{
  json_t *channels = NULL, *traces = NULL, *trace = NULL, *x = NULL, *y1 = NULL, *y2 = NULL;
  json_object_set_new(obj, "instr", json_integer(INSTR_DEV_DATA));

  channels = json_array();
  traces = json_array();
  trace = json_object();
  x = json_array();
  y1 = json_array();
  char *data, *token;
  timestamp();

  visaStatus = viClose(visaInstr);
  visaStatus = viOpen(visaDefault, visaFoundInstrBuffer, VI_NULL, VI_NULL, &visaInstr);

  strcpy(visaWriteBuffer, ":SENSe1:FREQuency:DATA?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  data = json_string_value(json_stringn(visaReadBuffer, visaRetCount));
  token = strtok(data + 2 + (data[1] - '0'), ",");
  while (token != NULL)
  {
    json_array_append_new(x, json_real(atof(token)));
    token = strtok(NULL, ",");
  }
  free(data);

  strcpy(visaWriteBuffer, ":CALCulate1:DATA? FDATA");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  if (visaStatus < VI_SUCCESS)
    printf("Write success.\n");
  visaStatus = viRead(visaInstr, visaReadBuffer, visaReadBufferSize, &visaRetCount);
  if (visaStatus < VI_SUCCESS)
  {
    viStatusDesc(visaInstr, visaStatus, visaStatusErrorString);
    json_object_set_new(obj, "err", json_string(visaStatusErrorString));
    return 0;
  }
  data = json_string_value(json_stringn(visaReadBuffer, visaRetCount));
  token = strtok(data + 2 + (data[1] - '0'), ",");
  while (token != NULL)
  {
    json_array_append_new(y1, json_real(atof(token)));
    token = strtok(NULL, ",");
  }
  free(data);

  json_object_set_new(trace, "x", x);
  json_object_set_new(trace, "y1", y1);
  json_array_append_new(traces, trace);
  json_array_append_new(channels, traces);
  json_object_set_new(obj, "channels", channels);
  return 1;
}