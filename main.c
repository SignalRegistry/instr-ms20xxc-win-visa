

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "visa.h"

static char stringinput[512];
static unsigned char buffer[100];
static ViUInt32 retCount;
static ViUInt32 writeCount;

int main()
{
  printf("Step: 0\n");
  ViStatus status; // Error checking
  ViSession defaultRM, instr;
  ViUInt32 numInstrs; // Return count from string I/O
  ViFindList fList;
  ViChar desc[VI_FIND_BUFLEN];

  printf("Step: Initialize VISA\n");
  status = viOpenDefaultRM(&defaultRM);

  if (status < VI_SUCCESS)
  {
    printf("Can't initialize VISA\n");
    return -1;
  }
  else
    printf("VISA initialized.\n");

  status = viFindRsrc(defaultRM, "?*", &fList, &numInstrs, desc);
  // if (status < VI_SUCCESS)
  // {
  //   printf("Can't initialize find\n");
  //   return -1;
  // }
  if (status == VI_ERROR_INV_OBJECT)
    printf("Invalid object\n");
  else if (status == VI_ERROR_NSUP_OPER)
    printf("Session does not support operation\n");
  else if (status == VI_ERROR_INV_EXPR)
    printf("Invalid expression\n");
  else if (status == VI_ERROR_RSRC_NFOUND)
    printf("Resource not found\n");
  else
  {
    printf("%s \n", desc);
    status = viOpen(defaultRM, desc, VI_NULL, VI_NULL, &instr);
    if (status < VI_SUCCESS)
      printf("An error occurred opening a session to %s\n", desc);
    else
    {
      printf("Connected\n");
      status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
      strcpy(stringinput, "*IDN?");
      status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
      if (status < VI_SUCCESS)
      {
        printf("Error writing to the device\n");
      }

      /*
     * Now we will attempt to read back a response from the device to
     * the identification query that was sent.  We will use the viRead
     * function to acquire the data.  We will try to read back 100 bytes.
     * After the data has been read the response is displayed.
     */
      status = viRead(instr, buffer, 100, &retCount);
      if (status < VI_SUCCESS)
      {
        printf("Error reading a response from the device\n");
      }
      else
      {
        printf("Data read: %*s\n", retCount, buffer);
      }
      status = viClose(instr);
    }
    for (unsigned i = 0; i < numInstrs - 1; --i)
    {
      status = viFindNext(fList, desc); /* find next desriptor */
      status = viOpen(defaultRM, desc, VI_NULL, VI_NULL, &instr);
      if (status < VI_SUCCESS)
        printf("An error occurred opening a session to %s\n", desc);
      else
      {
        printf("Connected\n");
        status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
        strcpy(stringinput, "*IDN?");
        status = viWrite(instr, (ViBuf)stringinput, (ViUInt32)strlen(stringinput), &writeCount);
        if (status < VI_SUCCESS)
        {
          printf("Error writing to the device\n");
        }

        /*
     * Now we will attempt to read back a response from the device to
     * the identification query that was sent.  We will use the viRead
     * function to acquire the data.  We will try to read back 100 bytes.
     * After the data has been read the response is displayed.
     */
        status = viRead(instr, buffer, 100, &retCount);
        if (status < VI_SUCCESS)
        {
          printf("Error reading a response from the device\n");
        }
        else
        {
          printf("Data read: %*s\n", retCount, buffer);
        }
        status = viClose(instr);
      }
    }
    // {
    //   /* stay in this loop until we find all instruments */
    //   status = viFindNext(findList, instrDescriptor); /* find next desriptor */
    //   if (status < VI_SUCCESS)
    //   { /* did we find the next resource? */
    //     printf("An error occurred finding the next resource.\nHit enter to continue.");
    //     fflush(stdin);
    //     getchar();
    //     viClose(defaultRM);
    //     return status;
    //   }
    //   printf("%s \n", instrDescriptor);

    //   /* Now we will open a session to the instrument we just found */
    //   status = viOpen(defaultRM, instrDescriptor, VI_NULL, VI_NULL, &instr);
    //   if (status < VI_SUCCESS)
    //     printf("An error occurred opening a session to %s\n", instrDescriptor);
    //   else
    // } /* end while */
  }
  printf("Found instruments: %d\n", numInstrs);

  status = viClose(fList);
  status = viClose(defaultRM);

  return 0;
}