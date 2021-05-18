

#include <windows.h>
#include <stdio.h>

#include "visa.h"

int main()
{
  printf("Step: 0\n");
  ViStatus status; // Error checking
  ViSession defaultRM;
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
  if (status < VI_SUCCESS)
  {
    printf("Can't initialize find\n");
    return -1;
  }
  else
  {
    printf("%s \n", desc);
    for (int i = 0; i < numInstrs - 1; --i)
    {
      status = viFindNext(fList, desc); /* find next desriptor */
      printf("%s \n", desc);
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

  status = viClose(defaultRM);

  return 0;
}