#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrimorph.h"
#include "mri_conform.h"
#include "utils.h"
#include "timer.h"

int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;

char *Progname ;
static char *log_fname = NULL ;
static void usage_exit(int code) ;

static int quiet = 0 ;
static int all_flag = 0 ;

int
main(int argc, char *argv[])
{
  char   **av ;
  int    ac, nargs, lno, nshared, nvox1, nvox2, total_nvox1, total_nvox2,
         total_nshared, nlabels, i ;
  int          msec, minutes, seconds/*, wrong, total, correct*/ ;
  struct timeb start ;
  MRI    *mri1, *mri2 ;
  FILE   *log_fp ;
  float  nvox_mean ;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  TimerStart(&start) ;

  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 3)
    usage_exit(1) ;


  mri1 = MRIread(argv[1]) ;
  if (!mri1)
    ErrorExit(ERROR_NOFILE, "%s: could not read volume from %s",Progname,
              argv[1]) ;
  mri2 = MRIread(argv[2]) ;
  if (!mri2)
    ErrorExit(ERROR_NOFILE, "%s: could not read volume from %s",Progname,
              argv[2]) ;

  if (log_fname)
  {
    log_fp = fopen(log_fname, "a+") ;
    if (!log_fp)
      ErrorExit(ERROR_BADFILE, "%s: could not open %s for writing",
                Progname, log_fname) ;
  }
  else
    log_fp = NULL ;

  if (all_flag)
  {
    MRI *mri1_label = NULL, *mri2_label = NULL ;

    mri1_label = MRIclone(mri1, NULL) ;
    mri2_label = MRIclone(mri2, NULL) ;
    for (lno = 0 ; lno < 1000 ; lno++)
    {
#if 1
      nvox1 = MRIvoxelsInLabel(mri1, lno) ;
      nvox2 = MRIvoxelsInLabel(mri2, lno) ;
      if (!nvox1 && !nvox2)
        continue ;
      nvox_mean = (float)(nvox1+nvox2)/2.0f ;
      nshared = MRIlabelOverlap(mri1, mri2, lno) ;
      
      printf("volume diff = |(%d - %d)| / %2.1f = %2.2f\n",
             nvox1, nvox2, nvox_mean,100.0f*(float)abs(nvox1-nvox2)/nvox_mean);
      printf("volume overlap = %d / %2.1f = %2.2f\n",
             nshared, nvox_mean, 100.0f*(float)nshared/nvox_mean) ;
      if (log_fp)
      {
        fprintf(log_fp, "%d  %2.2f  %2.2f\n", lno,
                100.0f*(float)abs(nvox1-nvox2)/nvox_mean,
                100.0f*(float)nshared/nvox_mean) ;
      }
#else
      nvox1 = MRIcopyLabel(mri1, mri1_label, lno) ;
      nvox2 = MRIcopyLabel(mri2, mri1_label, lno) ;
      if (!nvox1 && !nvox2)
        continue ;
      correct = MRIlabelOverlap(mri1, mri2, lno) ;
      total = (nvox1+nvox2)/2 ;
      wrong = total-correct ;
      printf("label %03d: %d of %d voxels correctly labeled - %2.2f%%\n",
             lno, correct, total, 100.0f*(float)correct/(float)total) ;
      if (log_fp)
      {
        fprintf(log_fp,"%d  %d  %d  %2.4f\n", lno, correct, total, 
                100.0f*(float)correct/(float)total) ;
        fclose(log_fp) ;
      }
#endif
    }
    if (log_fp)
      fclose(log_fp) ;
  }
  else
  {
    nlabels = total_nvox1 = total_nvox2 = total_nshared = 0 ;
    for (i = 3 ; i < argc ; i++)
    {
      lno = atoi(argv[i]) ;
      nvox1 = MRIvoxelsInLabel(mri1, lno) ;
      nvox2 = MRIvoxelsInLabel(mri2, lno) ;
      nvox_mean = (float)(nvox1+nvox2)/2 ;
      nshared = MRIlabelOverlap(mri1, mri2, lno) ;
      
      if (!quiet)
      {
        printf("label %d: volume diff = |(%d - %d)| / %2.1f = %2.2f\n",
               lno,nvox1,nvox2,nvox_mean,
               100.0f*(float)abs(nvox1-nvox2)/nvox_mean);
        printf("label %d: volume overlap = %d / %2.1f = %2.2f\n",
               lno, nshared, nvox_mean, 100.0f*(float)nshared/nvox_mean) ;
      }
      if (log_fp)
      {
        fprintf(log_fp, "%2.2f  %2.2f\n", 
                100.0f*(float)abs(nvox1-nvox2)/nvox_mean,
                100.0f*(float)nshared/nvox_mean) ;
        fclose(log_fp) ;
      }
      total_nvox1 += nvox1 ;
      total_nvox2 += nvox2 ;
      total_nshared += nshared ;
      nlabels++ ;
    }      
    if (nlabels > 1)
    {
      nvox_mean = (total_nvox1 + total_nvox2) / 2 ;
      printf("total: volume diff = |(%d - %d)| / %2.1f = %2.2f\n",
             total_nvox1,total_nvox2,nvox_mean,
             100.0f*(float)abs(total_nvox1-total_nvox2)/nvox_mean);
      printf("total: volume overlap = %d / %2.1f = %2.2f\n",
             total_nshared, nvox_mean, 100.0f*(float)total_nshared/nvox_mean) ;
    }
  }

  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;

  if (DIAG_VERBOSE_ON)
    fprintf(stderr, "overlap calculation took %d minutes and %d seconds.\n", 
            minutes, seconds) ;

  exit(0) ;
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  switch (toupper(*option))
  {
  case 'Q':
    quiet = 1 ;
    break ;
  case 'A':
    all_flag = 1 ;
    break ;
  case 'L':
    log_fname = argv[2] ;
    nargs = 1 ;
    fprintf(stderr, "logging results to %s\n", log_fname) ;
    break ;
  case '?':
  case 'U':
    usage_exit(0) ;
    break ;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
usage_exit(int code)
{
  printf("usage: %s [options] <volume 1> <volume 2>\n", Progname) ;
  printf(
         "\ta           - compute overlap of all lables\n"
         );
  exit(code) ;
}
