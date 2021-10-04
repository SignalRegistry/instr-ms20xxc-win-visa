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
#define VISA_READ_MAX_BUFFER_SIZE 1000000
static char visaReadBuffer[VISA_READ_MAX_BUFFER_SIZE]; // read buffer

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
    visaStatus = viSetAttribute(visaInstr, VI_ATTR_TMO_VALUE, 10000);

    printf("Status : ");
    strcpy(visaWriteBuffer, "*IDN?");
    visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
    if (visaStatus < VI_SUCCESS)
    {
      printf("Not supported.\n");
      visaStatus = viFindNext(visaFoundInstrList, visaFoundInstrBuffer); /* find next desriptor */
      continue;
    }
    visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
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
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  json_object_set_new(obj, "id", json_string(visaReadBuffer));
  return 1;
}

int instr_conf(json_t *obj)
{
  json_t *jstrbuff = NULL;
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
        // Measure
        else if (strcmp(key, "CALC:FORM") == 0)
        {
          sprintf(visaWriteBuffer, ":CALCulate%d:FORMat %s", active_trace, json_string_value(value));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "CALC:PAR") == 0)
        {
          sprintf(visaWriteBuffer, ":SENSe:TRACe%d:SPARams %s", active_trace, json_string_value(value));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "CALC:PAR:COUN") == 0)
        {
          sprintf(visaWriteBuffer, ":SENSe:TRACe:TOTal %d", atol(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "CALC:PAR:SEL") == 0)
        {
          sprintf(visaWriteBuffer, ":SENSe:TRACe%d:SELect", atol(json_string_value(value)));
          active_trace = atoi(json_string_value(value));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "SENS:SMO:APER") == 0)
        {
          sprintf(visaWriteBuffer, ":CALCulate%d:SMOothing:APERture %d", active_trace, atol(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        // Measure
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:PDIV") == 0)
        {
          sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:Y:PDIVision %f", active_trace, atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:RLEV") == 0)
        {
          sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:Y:RLEVel %f", active_trace, atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:RPOS") == 0)
        {
          sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:Y:RPOSition %f", active_trace, atof(json_string_value(value)));
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:AUTO") == 0)
        {
          sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:RLEVel IMMediate", active_trace);
          visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
        }
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
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:STAR", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:STOP?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:STOP", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:CENTer?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:CENT", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  strcpy(visaWriteBuffer, ":SENSe:FREQuency:SPAN?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:FREQ:SPAN", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);

  // Sweep
  strcpy(visaWriteBuffer, ":SENSe:SWEep:POINts?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:SWE:POIN", json_integer(atol(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  strcpy(visaWriteBuffer, ":SENSe:SWEep:IFBW?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:BAND:RES", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);

  // Measure
  sprintf(visaWriteBuffer, ":CALCulate%d:FORMat?\0", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "CALC:FORM", jstrbuff);
  sprintf(visaWriteBuffer, ":SENSe:TRACe%d:SPARams?\0", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "CALC:PAR", jstrbuff);
  sprintf(visaWriteBuffer, ":SENSe:TRACe:TOT?\0");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "CALC:PAR:COUN", json_integer(atol(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  sprintf(visaWriteBuffer, ":SENSe:TRACe:SELect?\0");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  active_trace = atoi(json_string_value(jstrbuff) + 2);
  json_object_set_new(obj, "CALC:PAR:SEL", json_integer(atol(json_string_value(jstrbuff) + 2)));
  json_decref(jstrbuff);
  sprintf(visaWriteBuffer, ":CALCulate%d:SMOothing:APERture?\0", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "SENS:SMO:APER", json_integer(atol(json_string_value(jstrbuff))));
  json_decref(jstrbuff);

  // Scale
  sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:Y:PDIVision?\0", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:PDIV", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:Y:RLEVel?\0", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:RLEV", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);
  sprintf(visaWriteBuffer, ":DISPlay:WINDow:TRACe%d:Y:RPOSition?\0", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  jstrbuff = json_stringn(visaReadBuffer, visaRetCount);
  json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:RPOS", json_real(atof(json_string_value(jstrbuff))));
  json_decref(jstrbuff);

  json_decref(jstrbuff);

  // UI
  json_t *cat, *subcat, *cats, *subcats, *options, *parameters;
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

  // Measure
  cat = json_object();
  json_object_set_new(cat, "name", json_string("Measure"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Graph Type"));
  json_object_set_new(subcat, "scpi", json_string("CALC:FORM"));
  options = json_array();
  json_array_append_new(options, json_string("LMAG"));
  json_array_append_new(options, json_string("PHAS"));
  json_array_append_new(options, json_string("GDEL"));
  json_array_append_new(options, json_string("SWR"));
  json_array_append_new(options, json_string("LM/2"));
  json_array_append_new(options, json_string("REAL"));
  json_array_append_new(options, json_string("IMAG"));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("S Parameter"));
  json_object_set_new(subcat, "scpi", json_string("CALC:PAR"));
  options = json_array();
  json_array_append_new(options, json_string("S11"));
  json_array_append_new(options, json_string("S21"));
  json_array_append_new(options, json_string("S12"));
  json_array_append_new(options, json_string("S22"));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Number of Traces"));
  json_object_set_new(subcat, "scpi", json_string("CALC:PAR:COUN"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(4));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Active Trace"));
  json_object_set_new(subcat, "scpi", json_string("CALC:PAR:SEL"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(4));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Smoothing %"));
  json_object_set_new(subcat, "scpi", json_string("SENS:SMO:APER"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(20));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);

  // Scale
  cat = json_object();
  json_object_set_new(cat, "name", json_string("Scale"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Scale"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:PDIV"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Ref Value"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:RLEV"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Ref Position"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:RPOS"));
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
  char *token;
  char data[VISA_READ_MAX_BUFFER_SIZE];

  timestamp();

  strcpy(visaWriteBuffer, ":SENSe1:FREQuency:DATA?");
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  if (visaStatus < VI_SUCCESS)
  {
    viStatusDesc(visaInstr, visaStatus, visaStatusErrorString);
    json_object_set_new(obj, "err", json_string(visaStatusErrorString));
    return 0;
  }
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  if (visaStatus < VI_SUCCESS)
  {
    viStatusDesc(visaInstr, visaStatus, visaStatusErrorString);
    json_object_set_new(obj, "err", json_string(visaStatusErrorString));
    return 0;
  }
  // printf("%s\n", visaReadBuffer);
  // printf("%d\n", visaRetCount);
  strncpy(data, visaReadBuffer, visaRetCount);
  data[visaRetCount] = '\0';
  token = strtok(data + 2 + (data[1] - '0'), ",");
  while (token != NULL)
  {
    json_array_append_new(x, json_real(atof(token)));
    token = strtok(NULL, ",");
  }

  sprintf(visaWriteBuffer, ":CALCulate%d:DATA? FDATA", active_trace);
  visaStatus = viWrite(visaInstr, (ViBuf)visaWriteBuffer, (ViUInt32)strlen(visaWriteBuffer), &visaWriteCount);
  if (visaStatus < VI_SUCCESS)
  {
    viStatusDesc(visaInstr, visaStatus, visaStatusErrorString);
    json_object_set_new(obj, "err", json_string(visaStatusErrorString));
    return 0;
  }
  visaStatus = viRead(visaInstr, visaReadBuffer, VISA_READ_MAX_BUFFER_SIZE, &visaRetCount);
  if (visaStatus < VI_SUCCESS)
  {
    viStatusDesc(visaInstr, visaStatus, visaStatusErrorString);
    json_object_set_new(obj, "err", json_string(visaStatusErrorString));
    return 0;
  }
  // printf("%s\n", visaReadBuffer);
  // printf("%d\n", visaRetCount);

  strncpy(data, visaReadBuffer, visaRetCount);
  data[visaRetCount] = '\0';
  token = strtok(data + 2 + (data[1] - '0'), ",");
  while (token != NULL)
  {
    json_array_append_new(y1, json_real(atof(token)));
    token = strtok(NULL, ",");
  }

  json_object_set_new(trace, "x", x);
  json_object_set_new(trace, "y1", y1);
  json_array_append_new(traces, trace);
  json_array_append_new(channels, traces);
  json_object_set_new(obj, "channels", channels);
  return 1;
}