#ifndef H_INSTR
#define H_INSTR

#include "visa.h"
#include "jansson.h"

enum INSTR_MSG_TYPE
{
  BLANK,
  INSTR_CONN_READY,
  INSTR_DEV_READY,
  INSTR_DEV_INFO,
  INSTR_DEV_CONF,
  INSTR_DEV_DATA,
  INSTR_DEV_ERROR
};

enum INSTR_MODE
{
  INSTR_MODE_VNA,
};

static const char *instr_ui = "[{\"name\":\"STIMILUS\",\"items\":[{\"name\":\"Start\",\"scpi\":\"SENS:FREQ:STAR\"},{\"name\":\"Stop\",\"scpi\":\"SENS:FREQ:STOP\"},{\"name\":\"Points\",\"scpi\":\"SENS:SWE:POIN\"}]},{\"name\":\"RESPONSE\",\"items\":[{\"name\":\"Measurement\",\"scpi\":\"CALC:PAR:DEF\",\"options\":[\"S11\",\"S21\",\"A\",\"B\",\"R\"]},{\"name\":\"Format\",\"scpi\":\"CALC:FORM\",\"options\":[\"MLOG\",\"PHAS\",\"GDEL\",\"MLIN\",\"SWR\",\"REAL\",\"IMAG\",\"UPH\"]}]},{\"name\":\"SCALE\",\"items\":[{\"name\":\"Scale\",\"scpi\":\"DISP:WIND:TRAC:Y:SCAL:PDIV\"},{\"name\":\"Ref Value\",\"scpi\":\"DISP:WIND:TRAC:Y:SCAL:RLEV\"},{\"name\":\"Ref Position\",\"scpi\":\"DISP:WIND:TRAC:Y:SCAL:RPOS\"},{\"name\":\"Divisions\",\"scpi\":\"DISP:WIND:Y:SCAL:DIV\",\"number\":{\"min\":4,\"max\":20,\"step\":2}},{\"name\":\"Auto Scale\",\"scpi\":\"DISP:WIND:TRAC:Y:SCAL:AUTO\",\"button\":true}]},{\"name\":\"CHANNELS\",\"items\":[{\"name\":\"Channel Count\",\"scpi\":\"CALC\",\"options\":[1,2,3,4,6,8,9]},{\"name\":\"Active Channel\",\"scpi\":\"CALC:ACT\",\"number\":{\"min\":1,\"max\":9,\"step\":1}}]},{\"name\":\"TRACES\",\"items\":[{\"name\":\"Trace Count\",\"scpi\":\"CALC:PAR:COUN\",\"number\":{\"min\":1,\"max\":8,\"step\":1}},{\"name\":\"Active Trace\",\"scpi\":\"CALC:PAR:SEL\",\"number\":{\"min\":1,\"max\":8,\"step\":1}}]}]";
#define INSTR_MAX_QUERY_SIZE 50
static json_t *instr_queries[INSTR_MAX_QUERY_SIZE];
static int active_channel = 1;
static int channel_count = 1;
static int active_trace = 1;
static int trace_count = 1;

int instr_connect(json_t *obj);
int instr_info(json_t *obj);
// int instr_conf(json_t *obj);
// int instr_data(json_t *obj);
// void instr_add_query(json_t *obj);
// void instr_disconnect();

#endif