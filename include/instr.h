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

#define INSTR_MAX_QUERY_SIZE 50
static json_t *instr_queries[INSTR_MAX_QUERY_SIZE];
static int active_channel = 1;
static int channel_count = 1;
static int active_trace = 1;
static int trace_count = 1;

int instr_connect(json_t *obj);
int instr_info(json_t *obj);
int instr_conf(json_t *obj);
// int instr_data(json_t *obj);
void instr_add_query(json_t *obj);
// void instr_disconnect();

#endif