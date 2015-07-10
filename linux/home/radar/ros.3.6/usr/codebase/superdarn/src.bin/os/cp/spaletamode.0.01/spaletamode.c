/* spaletamode.c
   ============
*/

/*
 LICENSE AND DISCLAIMER
 
 Copyright (c) 2012 The Johns Hopkins University/Applied Physics Laboratory
 
 This file is part of the Radar Software Toolkit (RST).
 
 RST is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.

 RST is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with RST.  If not, see <http://www.gnu.org/licenses/>.
 
 
  
*/
/* Includes provided by the OS environment */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <argtable2.h>
#include <zlib.h>
#include <fcntl.h>

/* Includes provided by the RST */ 
#include "rtypes.h"
#include "rtime.h"
#include "dmap.h"
#include "limit.h"
#include "radar.h"
#include "rprm.h"
#include "iq.h"
#include "rawdata.h"
#include "fitblk.h"
#include "fitdata.h"
#include "fitacf.h"
#include "errlog.h"
#include "tcpipmsg.h"
#include "rmsg.h"
#include "rmsgsnd.h"
#include "build.h"
#include "global.h"
#include "reopen.h"
#include "setup.h"
#include "sync.h"
#include "site.h"
#include "sitebuild.h"

/* sorry, included for checking sanity checking pcode sequences with --test (JTK)*/
#include "tsg.h" 
#include "maketsg.h"

/* Argtable define for argument error parsing */
#define ARG_MAXERRORS 30

int main(int argc,char *argv[]) {
  char progid[80]={"spaletamode"};
  char progname[256]="spaletamode";
  char modestr[32];

  char *roshost=NULL;
  char *droshost={"127.0.0.1"};

  char *ststr=NULL;

  char *libstr=NULL;
  char *verstr=NULL;

  int status=0,n,i,j,index;
  int nerrors=0;
  int exitpoll=0;

  /* Variables need for interprocess communications */
  char logtxt[1024];
  void *tmpbuf;
  size_t tmpsze;
/*
  struct TCPIPMsgHost shell={"127.0.0.1",44101,-1};
*/
  int tnum=4;      
  struct TCPIPMsgHost task[4]={
    {"127.0.0.1",1,-1}, /* iqwrite */
    {"127.0.0.1",2,-1}, /* rawacfwrite */
    {"127.0.0.1",3,-1}, /* fitacfwrite */
    {"127.0.0.1",4,-1}  /* rtserver */
  };

/* Define the available barker codes for phasecoding*/
  int *bcode=NULL;
  int bcode1[1]={1};
  int bcode2[2]={1,-1};
  int bcode3[3]={1,1,-1};
  int bcode4[4]={1,1,-1,1};
  int bcode5[5]={1,1,1,-1,1};
  int bcode7[7]={1,1,1,-1,-1,1,-1};
  int bcode11[11]={1,1,1,-1,-1,-1,1,-1,-1,1,-1};
  int bcode13[13]={1,1,1,1,1,-1,-1,1,1,-1,1,-1,1};

/* Pulse sequence Table */
  int *pcode_scan=NULL;
  int mppul_scan=8;
  int mplgs_scan=23;
  int mpinc_scan=1500;
  int rsep_scan=45;
  int nrang_scan=75;
  int txpl_scan=300;
  int nbaud_scan=1;
  int ptab_scan[8] = {0,14,22,24,27,31,42,43}; 
  int lags_scan[LAG_SIZE][2] = {
    { 0, 0},            /*  0 */
    {42,43},            /*  1 */
    {22,24},            /*  2 */
    {24,27},            /*  3 */
    {27,31},            /*  4 */
    {22,27},            /*  5 */

    {24,31},            /*  7 */
    {14,22},            /*  8 */
    {22,31},            /*  9 */
    {14,24},            /* 10 */
    {31,42},            /* 11 */
    {31,43},            /* 12 */
    {14,27},            /* 13 */
    { 0,14},            /* 14 */
    {27,42},            /* 15 */
    {27,43},            /* 16 */
    {14,31},            /* 17 */
    {24,42},            /* 18 */
    {24,43},            /* 19 */
    {22,42},            /* 20 */
    {22,43},            /* 21 */
    { 0,22},            /* 22 */

    { 0,24},            /* 24 */

    {43,43}};           /* alternate lag-0  */

  int *pcode_camp=NULL;
  int mppul_camp=16;
  int mplgs_camp=121;
  int mpinc_camp=100;
  int rsep_camp=15;
  int txpl_camp=100;
  int nbaud_camp=1;
  int nrang_camp=225;
  int ptab_camp[16] = {0,4,19,42,78,127,191,270,364,474,600,745,905,1083,1280,1495};
  int lags_camp[LAG_SIZE][2] = {
  {1495,1495},          /*  0 */
  {0,4},                /*  1 */
  {4,19},               /*  2 */
  {0,19},               /*  3 */
  {19,42},              /*  4 */
  {42,78},              /*  5 */
  {4,42},               /*  6 */
  {0,42},               /*  7 */
  {78,127},             /*  8 */
  {19,78},              /*  9 */
  {127,191},            /*  10 */
  {4,78},               /*  11 */
  {0,78},               /*  12 */
  {191,270},            /*  13 */
  {42,127},             /*  14 */
  {270,364},            /*  15 */
  {19,127},             /*  16 */
  {364,474},            /*  17 */
  {78,191},             /*  18 */
  {4,127},              /*  19 */
  {474,600},            /*  20 */
  {0,127},              /*  21 */
  {127,270},            /*  22 */
  {600,745},            /*  23 */
  {42,191},             /*  24 */
  {745,905},            /*  25 */
  {191,364},            /*  26 */
  {19,191},             /*  27 */
  {905,1083},           /*  28 */
  {4,191},              /*  29 */
  {0,191},              /*  30 */
  {78,270},             /*  31 */
  {1083,1280},          /*  32 */
  {270,474},            /*  33 */
  {1280,1495},          /*  34 */
  {42,270},             /*  35 */
  {127,364},            /*  36 */
  {364,600},            /*  37 */
  {19,270},             /*  38 */
  {4,270},              /*  39 */
  {0,270},              /*  40 */
  {474,745},            /*  41 */
  {191,474},            /*  42 */
  {78,364},             /*  43 */
  {600,905},            /*  44 */
  {42,364},             /*  45 */
  {270,600},            /*  46 */
  {745,1083},           /*  47 */
  {19,364},             /*  48 */
  {127,474},            /*  49 */
  {4,364},              /*  50 */
  {0,364},              /*  51 */
  {905,1280},           /*  52 */
  {364,745},            /*  53 */
  {78,474},             /*  54 */
  {191,600},            /*  55 */
  {1083,1495},          /*  56 */
  {474,905},            /*  57 */
  {42,474},             /*  58 */
  {19,474},             /*  59 */
  {4,474},              /*  60 */
  {127,600},            /*  61 */
  {0,474},              /*  62 */
  {270,745},            /*  63 */
  {600,1083},           /*  64 */
  {78,600},             /*  65 */
  {745,1280},           /*  66 */
  {364,905},            /*  67 */
  {191,745},            /*  68 */
  {42,600},             /*  69 */
  {19,600},             /*  70 */
  {905,1495},           /*  71 */
  {4,600},              /*  72 */
  {0,600},              /*  73 */
  {474,1083},           /*  74 */
  {127,745},            /*  75 */
  {270,905},            /*  76 */
  {78,745},             /*  77 */
  {600,1280},           /*  78 */
  {42,745},             /*  79 */
  {191,905},            /*  80 */
  {364,1083},           /*  81 */
  {19,745},             /*  82 */
  {4,745},              /*  83 */
  {0,745},              /*  84 */
  {745,1495},           /*  85 */
  {127,905},            /*  86 */
  {474,1280},           /*  87 */
  {270,1083},           /*  88 */
  {78,905},             /*  89 */
  {42,905},             /*  90 */
 {19,905},             /*  91 */
  {191,1083},           /*  92 */
  {600,1495},           /*  93 */
  {4,905},              /*  94 */
  {0,905},              /*  95 */
  {364,1280},           /*  96 */
  {127,1083},           /*  97 */
  {78,1083},            /*  98 */
  {270,1280},           /*  99 */
  {474,1495},           /*  100 */
  {42,1083},            /*  101 */
  {19,1083},            /*  102 */
  {4,1083},             /*  103 */
  {0,1083},             /*  104 */
  {191,1280},           /*  105 */
  {364,1495},           /*  106 */
  {127,1280},           /*  107 */
  {78,1280},            /*  108 */
  {270,1495},           /*  109 */
  {42,1280},            /*  110 */
  {19,1280},            /*  111 */
  {4,1280},             /*  112 */
  {0,1280},             /*  113 */
  {191,1495},           /*  114 */
  {127,1495},           /*  115 */
  {78,1495},            /*  116 */
  {42,1495},            /*  117 */
  {19,1495},            /*  118 */
  {4,1495},             /*  119 */
  {0,1495},             /*  120 */
  {1495,1495}};         /*  121 */

/* Integration period variables */
  int scnsc=120;
  int scnus=0;
  int total_scan_usecs=0;
  int total_camp_usecs=0;
  long scan_elapsed_usecs=0;
  int camp_integration_usecs=0;
  int end_of_scan_usecs=10E6;
  int total_integration_usecs=0;
  int intsc_scan=0;
  int intus_scan=0;
/* End of Scan Multi-Frequency Camping Beam*/
  int nscnsc=0;
  int nscnus=0;
  int campsc=2;
  int campus=0;
  int camprep=1;
  int campbm=8;
  int *campfreqs=NULL;

  /* Variables for controlling clear frequency search */
  struct timeval t0,t1;
  /* Variables for controlling scan no wait repetition*/
  struct timeval s0,s1;
  int elapsed_secs=0;
  int default_clrskip_secs=30;
  int scansync=0,startup=1;

  /* XCF processing variables */
  int cnt=0;

  /* Variables associated with beam scanning */
  int beams=0;
  int skip,skip_scan=0;

  /* Variables for coordinating file locks */
                             /* l_type   l_whence  l_start  l_len  l_pid   */
    struct flock scan_lock = {F_RDLCK, SEEK_SET,   0,      0,     0 };
    struct flock camp_lock = {F_RDLCK, SEEK_SET,   0,      0,     0 };
    struct flock check_lock = {F_WRLCK, SEEK_SET,   0,      0,     0 };
    int scan_fd, camp_fd,lock_test;


  /* create commandline argument structs */
  /* First lets define a help argument */
  struct arg_lit  *al_help       = arg_lit0(NULL, "help", "Prints help infomation and then exits");
  /* Now lets define the keyword arguments */
  struct arg_lit  *al_debug      = arg_lit0(NULL, "debug","Enable debugging messages");
  struct arg_lit  *al_test       = arg_lit0(NULL, "test","Test-only, report parameter settings and exit without connecting to ros server");
  struct arg_lit  *al_discretion = arg_lit0(NULL, "di","Flag this is discretionary time operation"); 
  struct arg_lit  *al_fast       = arg_lit0(NULL, "fast","Flag this as fast 1-minute scan duration"); 
  struct arg_lit  *al_nowait     = arg_lit0(NULL, "nowait","Do not wait for minute scan boundary"); 
  struct arg_lit  *al_onesec     = arg_lit0(NULL, "onesec","Use one second integration times"); 
  struct arg_lit  *al_clrscan    = arg_lit0(NULL, "clrscan","Force clear frequency search at start of scan"); 
  /* Now lets define the integer valued arguments */
  struct arg_int  *ai_campsc     = arg_int0(NULL, "campsc", NULL,"Integer Seconds for end of scan camping beam period"); 
  struct arg_int  *ai_camprep    = arg_int0(NULL, "camprep", NULL,"Number of repetitions for camping beam period"); 
  struct arg_int  *ai_campus     = arg_int0(NULL, "campus", NULL,"Integer Microsecs for end of scan camping beam"); 
  struct arg_int  *ai_campbm     = arg_int0(NULL, "campbm", NULL,"Beam number for end of scan camping beam"); 
  struct arg_int  *ai_campfreq   = arg_intn(NULL, "campfreq", NULL,0,10,"Add a freq in kHz to use for camping beam"); 
  struct arg_int  *ai_baud       = arg_int0(NULL, "baud", NULL,"Baud to use for phasecoded sequences"); /*OptionAdd( &opt, "baud", 'i', &nbaud);*/
  struct arg_int  *ai_tau        = arg_int0(NULL, "tau", NULL,"Lag spacing in usecs"); /*OptionAdd( &opt, "tau", 'i', &mpinc);*/
  struct arg_int  *ai_nrang      = arg_int0(NULL, "nrang", NULL,"Number of range cells"); /*OptionAdd(&opt,"nrang",'i',&nrang);*/
  struct arg_int  *ai_frang      = arg_int0(NULL, "frang", NULL,"Distance to first range cell in km"); /*OptionAdd(&opt,"frang",'i',&frang); */
  struct arg_int  *ai_rsep       = arg_int0(NULL, "rsep", NULL,"Range cell extent in km"); /*OptionAdd(&opt,"rsep",'i',&rsep); */
  struct arg_int  *ai_dt         = arg_int0(NULL, "dt", NULL,"UTC Hour indicating start of day time operation"); /*OptionAdd( &opt, "dt", 'i', &day); */
  struct arg_int  *ai_nt         = arg_int0(NULL, "nt", NULL,"UTC Hour indicating start of night time operation"); /*OptionAdd( &opt, "nt", 'i', &night); */
  struct arg_int  *ai_df         = arg_int0(NULL, "df", NULL,"Day time transmit frequency in KHz"); /*OptionAdd( &opt, "df", 'i', &dfrq); */
  struct arg_int  *ai_nf         = arg_int0(NULL, "nf", NULL,"Night time transmit frequency in KHz"); /*OptionAdd( &opt, "nf", 'i', &nfrq); */
  struct arg_int  *ai_fixfrq     = arg_int0(NULL, "fixfrq", NULL,"Fixes the transmit frequency of the radar to one frequency, in KHz"); /*OptionAdd( &opt, "fixfrq", 'i', &fixfrq); */
  struct arg_int  *ai_xcf        = arg_int0(NULL, "xcf", NULL,"Enable xcf, --xcf 1: for all sequences --xcf 2: for every other sequence, etc..."); /*OptionAdd( &opt, "xcf", 'i', &xcnt); */
  struct arg_int  *ai_ep         = arg_int0(NULL, "ep", NULL,"Local TCP port for errorlog process"); /*OptionAdd(&opt,"ep",'i',&errlog.port); */
  struct arg_int  *ai_sp         = arg_int0(NULL, "sp", NULL,"Local TCP port for radarshall process"); /*OptionAdd(&opt,"sp",'i',&shell.port); */
  struct arg_int  *ai_bp         = arg_int0(NULL, "bp", NULL,"Local TCP port for start of support task proccesses"); /*OptionAdd(&opt,"bp",'i',&baseport); */
  struct arg_int  *ai_sb         = arg_int0(NULL, "sb", NULL,"Limits the minimum beam to the given value."); /*OptionAdd(&opt,"sb",'i',&sbm); */
  struct arg_int  *ai_eb         = arg_int0(NULL, "eb", NULL,"Limits the maximum beam number to the given value."); /*OptionAdd(&opt,"eb",'i',&ebm); */
  struct arg_int  *ai_cnum       = arg_int0("c", "cnum", NULL,"Radar Channel number, minimum value 1"); /*OptionAdd(&opt,"c",'i',&cnum); */
  struct arg_int  *ai_clrskip    = arg_int0(NULL, "clrskip",NULL,"Minimum number of seconds to skip between clear frequency search"); /*OptionAdd(&opt, "clrskip", 'i', &clrskip_secs); */
  struct arg_int  *ai_cpid       = arg_int0(NULL, "cpid",NULL,"Select control program ID number"); /*OptionAdd(&opt, "cpid", 'i', &cpid); */

  /* Now lets define the string valued arguments */
  struct arg_str  *as_ros        = arg_str0(NULL, "ros", NULL,        "IP address of ROS server process"); /* OptionAdd(&opt,"ros",'t',&roshost); */
  struct arg_str  *as_ststr      = arg_str0(NULL, "stid", NULL,       "The station ID string. For example, use aze for azores east."); /* OptionAdd(&opt,"stid",'t',&ststr); */
  struct arg_str  *as_libstr     = arg_str0(NULL, "lib", NULL,       "The site library string. For example, use ros for for common libsite.ros"); 
  struct arg_str  *as_verstr     = arg_str0(NULL, "version", NULL,   "The site library version string. Defaults to: \"1\" "); 

  /* required end argument */
  struct arg_end  *ae_argend     = arg_end(ARG_MAXERRORS);

  /* create list of all arguement structs */
  void* argtable[] = {al_help,al_debug,al_test,al_discretion, al_fast, al_nowait, al_onesec, \
                      ai_baud,ai_campsc,ai_campus,ai_campbm, ai_camprep, ai_campfreq, \
                      ai_tau, ai_nrang, ai_frang, ai_rsep, ai_dt, ai_nt, ai_df, ai_nf, ai_fixfrq, ai_xcf, ai_ep, ai_sp, ai_bp, ai_sb, ai_eb, ai_cnum, \
                      as_ros, as_ststr, as_libstr,as_verstr,ai_clrskip,al_clrscan,ai_cpid,ae_argend};
  /* Lets create those file locks now */
    scan_lock.l_pid = getpid();
    camp_lock.l_pid = getpid();
    camp_lock.l_type = F_RDLCK;
    if ((scan_fd = open("/tmp/scan_lock", O_CREAT|O_RDWR,0644)) == -1) {
        perror("open scan_lock file");
        exit(1);
    }
    if ((camp_fd = open("/tmp/camp_lock", O_CREAT|O_RDWR),0644) == -1) {
        perror("open camp_lock file");
        exit(1);
    }
/* END of variable defines */

/* Set default values of globally defined variables here*/
  cp=9200;
  intsc=7;
  intus=0;
/* Setup things initially for scan pulse sequence */
  mppul=mppul_scan;
  mplgs=mplgs_scan;
  mpinc=mpinc_scan;
  nrang=nrang_scan;
  rsep=rsep_scan;
  txpl=txpl_scan;
  nbaud=nbaud_scan;
  pcode=pcode_scan;

/* Set default values for all the cmdline options */
  al_discretion->count = 0;
  al_fast->count = 0;
  al_nowait->count = 0;
  al_onesec->count = 0;
  al_clrscan->count = 0;
  al_debug->count = 0;
  ai_bp->ival[0] = 44100;
  ai_fixfrq->ival[0] = -1;
  ai_baud->ival[0] = nbaud_scan;
  ai_tau->ival[0] = mpinc_scan;
  ai_nrang->ival[0] = nrang_scan;
  ai_rsep->ival[0] = rsep_scan;
  ai_frang->ival[0] = frang;
  ai_dt->ival[0] = day;
  ai_nt->ival[0] = night;
  ai_df->ival[0] = dfrq;
  ai_nf->ival[0] = nfrq;
  ai_xcf->ival[0] = xcnt;
  ai_ep->ival[0] = errlog.port;
  ai_sp->ival[0] = shell.port;
  ai_sb->ival[0] = sbm;
  ai_eb->ival[0] = ebm;
  ai_cnum->ival[0] = cnum;
  ai_clrskip->ival[0] = -1;
  ai_cpid->ival[0] = 0;

 /* ========= PROCESS COMMAND LINE ARGUMENTS ============= */
  nerrors = arg_parse(argc,argv,argtable);

  if (nerrors > 0) {
    arg_print_errors(stdout,ae_argend,"spaletamode");
  }
  
  if (argc == 1) {
    printf("No arguements found, try running %s with --help for more information.\n", progid);
  }

  if(al_help->count > 0) {
    printf("Usage: %s", progid);
    arg_print_syntax(stdout,argtable,"\n");
    /* TODO: Add other useful help text describing the purpose of spaletamode here */
    arg_print_glossary(stdout,argtable,"  %-25s %s\n");
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return 0;
  }

  /* Set debug flag from command line arguement */
  if (al_debug->count) {
    debug = al_debug->count;
  }

  /* Load roshost argument here */
  if(strlen(as_ros->sval[0])) {
    roshost = malloc((strlen(as_ros->sval[0]) + 1) * sizeof(char));
    strcpy(roshost, as_ros->sval[0]);
  } else {
    roshost = getenv("ROSHOST");
    if (roshost == NULL) roshost = droshost;
  }

  /* Load station string argument here */
  if(strlen(as_ststr->sval[0])) {
    ststr = malloc((strlen(as_ststr->sval[0]) + 1) * sizeof(char));
    strcpy(ststr, as_ststr->sval[0]);
  } else {
    ststr = getenv("STSTR");
  }

  /* Load site library argument here */
  if(strlen(as_libstr->sval[0])) {
    libstr = malloc((strlen(as_libstr->sval[0]) + 1) * sizeof(char));
    strcpy(libstr, as_libstr->sval[0]);
  } else {
    libstr = getenv("LIBSTR");
    if (libstr == NULL) libstr=ststr;
  }
  if(strlen(as_verstr->sval[0])) {
    verstr = malloc((strlen(as_verstr->sval[0]) + 1) * sizeof(char));
    strcpy(verstr, as_verstr->sval[0]);
  } else {
    verstr = NULL;
  }
  printf("Requested :: ststr: %s libstr: %s verstr: %s\n",ststr,libstr,verstr);
/* This loads Radar Site information from hdw.dat files */
  OpsStart(ststr);

/* This loads Site library via dlopen and maps:
 * site library specific functions into Site name space
*/
  status=SiteBuild(libstr,verstr); /* second argument is version string */
  if (status==-1) {
    fprintf(stderr,"Could not load requested site library\n");
    exit(1);
  }
 
/* Run SiteStart library function to load Site specific default values for global variables
 * This should be run before all options are parsed and before any task sockets are opened
 * arguments: host ip address, 3-letter station string
*/
  
  status=SiteStart(roshost,ststr);
  if (status==-1) {
    fprintf(stderr,"SiteStart failure\n");
    exit(1);
  }


/* load any provided argument values overriding default values provided by SiteStart */ 
  if (ai_xcf->count) xcnt = ai_xcf->ival[0];
  if (ai_baud->count)  nbaud_scan = ai_baud->ival[0];
  if (ai_tau->count)   mpinc_scan = ai_tau->ival[0];
  if (ai_nrang->count) nrang_scan = ai_nrang->ival[0];
  if (ai_rsep->count)  rsep_scan = ai_rsep->ival[0];
  if (ai_frang->count) frang = ai_frang->ival[0];
  if (ai_dt->count) day = ai_dt->ival[0];
  if (ai_nt->count) night = ai_nt->ival[0];
  if (ai_df->count) dfrq = ai_df->ival[0];
  if (ai_nf->count) nfrq = ai_nf->ival[0];
  if (ai_ep->count) errlog.port = ai_ep->ival[0];
  if (ai_sp->count) shell.port = ai_sp->ival[0];
  if (ai_sb->count) sbm = ai_sb->ival[0];
  if (ai_eb->count) ebm = ai_eb->ival[0];
  if (ai_cnum->count) cnum = ai_cnum->ival[0];
  if (ai_bp->count) baseport=ai_bp->ival[0];

  if (ai_campfreq->count){
    campfreqs=(int*)calloc(ai_campfreq->count,sizeof(int));
    for(i=0;i<ai_campfreq->count;i++){
      campfreqs[i]=ai_campfreq->ival[i];
    }
    if (ai_campsc->count) campsc = ai_campsc->ival[0];
    if (ai_campus->count) campus = ai_campus->ival[0];
    if (ai_camprep->count) camprep = ai_camprep->ival[0];
    if (ai_campbm->count) campbm = ai_campbm->ival[0];
  } else {
    camprep=0;
    campsc=0;
    campus=0;
  }
/* Open Connection to errorlog */  
  if ((errlog.sock=TCPIPMsgOpen(errlog.host,errlog.port))==-1) {    
    fprintf(stderr,"Error connecting to error log.\n Host: %s  Port: %d\n",errlog.host,errlog.port);
  }
/* Open Connection to radar shell */  
  if ((shell.sock=TCPIPMsgOpen(shell.host,shell.port))==-1) {    
    fprintf(stderr,"Error connecting to shell.\n");
  }

/* Open Connection to helper utilities like fitacfwrite*/  
  for (n=0;n<tnum;n++) task[n].port+=baseport;

/* Prep command string for tasks */ 
  strncpy(combf,progid,80);   
  OpsSetupCommand(argc,argv);
  OpsLogStart(errlog.sock,progname,argc,argv);  
  OpsSetupTask(tnum,task,errlog.sock,progname);
  for (n=0;n<tnum;n++) {
    RMsgSndReset(task[n].sock);
    RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
  }


  /* Initialize timing variables */
  elapsed_secs=0;
  gettimeofday(&t1,NULL);
  gettimeofday(&t0,NULL);

  /* Set up scan periods and beam integration times */
  beams=abs(ebm-sbm)+1;
  if (al_fast->count) {
    /* If fast option selected use 1 minute scan boundaries */
    cp=9201;
    intsc=3;
    intus=500000;
    scnsc=60;
    scnus=0;
    sprintf(modestr," (fast)");
    strncat(progname,modestr,strlen(modestr)+1);
  } else {
    /* If fast option not selected use 2 minute scan boundaries */
    intsc=7;
    intus=0;
    scnsc=120;
    scnus=0;
  }
  if (al_onesec->count) {
    /* If onesec option selected , no longer wait for scan boundaries, activate clear frequency skip option*/
    cp=9202;
    intsc=1;
    intus=0;
    scnsc=beams+4;
    scnus=0;
    sprintf(modestr," (onesec)");
    strncat(progname,modestr,strlen(modestr)+1);
    al_nowait->count=1;
    if(ai_clrskip->ival[0] < 0) ai_clrskip->ival[0]=default_clrskip_secs;
  }
  if(beams==1) {
    /* Camping Beam, no longer wait for scan boundaaries, activate clear frequency skip option */
    sprintf(modestr," (camp)");
    strncat(progname,modestr,strlen(modestr)+1);
    al_nowait->count=1;
    if(ai_clrskip->ival[0] < 0) ai_clrskip->ival[0]=default_clrskip_secs;
    cp=9203;
    sprintf(logtxt,"spaletamode configured for camping beam");
    ErrLog(errlog.sock,progname,logtxt);
    sprintf(logtxt," fast: %d onesec: %d cp: %d clrskip_secs: %d intsc: %d",al_fast->count,al_onesec->count,cp,ai_clrskip->ival[0],intsc);
    ErrLog(errlog.sock,progname,logtxt);
  }
  camp_integration_usecs=campsc*1E6+campus;
  total_camp_usecs=(camprep*ai_campfreq->count+1)*1.2*(camp_integration_usecs);
  if(total_camp_usecs > end_of_scan_usecs) end_of_scan_usecs=total_camp_usecs;
  total_camp_usecs=end_of_scan_usecs;
  total_scan_usecs=(scnsc*1E6)+scnus - end_of_scan_usecs;
  total_integration_usecs=total_scan_usecs/(beams);
  nscnsc=scnsc;
  nscnus=scnus;
  if(beams > 1 || camprep> 0 ) {
    /* if number of beams in scan greater than legacy 16, or end of scan camping is enabled recalculate beam dwell time to avoid over running scan boundary 
    *  If scan boundary wait is active. 
    */ 
      if (al_nowait->count==0 && al_onesec->count==0) {
      /* 
        total_camp_usecs=camprep*ai_campfreq->count*((campsc)*1E6+campus);
        total_scan_usecs=(scnsc*1E6)+scnus - total_camp_usecs - 1E6;
        total_integration_usecs=total_scan_usecs/beams;
      */
        intsc=total_integration_usecs/1E6;
        intus=total_integration_usecs -(intsc*1E6);
        nscnsc=total_scan_usecs/1E6;
        nscnus=total_scan_usecs -(nscnsc*1E6);
      }
  }
  if(camprep > 0) al_nowait->count=0;

  intsc_scan=intsc;
  intus_scan=intus;
  /* Configure phasecoded operation if nbaud > 1 */ 
  switch(nbaud) {
    case 1:
      bcode=bcode1;
    case 2:
      bcode=bcode2;
      break;
    case 3:
      bcode=bcode3;
      break;
    case 4:
      bcode=bcode4;
      break;
    case 5:
      bcode=bcode5;
      break;
    case 7:
      bcode=bcode7;
      break;
    case 11:
      bcode=bcode11;
      break;
    case 13:
      bcode=bcode13;
      break;
    default:
      ErrLog(errlog.sock,progname,"Error: Unsupported nbaud requested, exiting");
      SiteExit(1);
  }
  pcode_scan=(int *)malloc((size_t)sizeof(int)*mppul_scan*nbaud_scan);
  for(i=0;i<mppul_scan;i++){
    for(n=0;n<nbaud_scan;n++){
      pcode_scan[i*nbaud_scan+n]=bcode[n];
    }
  }
  pcode_camp=(int *)malloc((size_t)sizeof(int)*mppul_camp*nbaud_camp);
  for(i=0;i<mppul_camp;i++){
    for(n=0;n<nbaud_camp;n++){
      pcode_camp[i*nbaud_camp+n]=bcode[n];
    }
  }

  /* Set special cpid if provided on commandline */
  if(ai_cpid->count > 0) cp=ai_cpid->ival[0];
  /* Set cp to negative value indication discretionary period */
  if (al_discretion->count) cp= -cp;


  /* Calculate txpl setting from rsep */

  txpl=(nbaud*rsep*20)/3;

  /* Attempt to adjust mpinc to be a multiple of 10 and a muliple of txpl */

  if ((mpinc % txpl) || (mpinc % 10))  {
    sprintf(logtxt,"Error: mpinc not multiple of txpl... checking to see if it can be adjusted");
    ErrLog(errlog.sock,progname,logtxt);
    sprintf(logtxt,"Initial: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep);
    ErrLog(errlog.sock,progname,logtxt);
    if((txpl % 10)==0) {

      sprintf(logtxt,"Attempting to adjust mpinc to correct");
      ErrLog(errlog.sock,progname,logtxt);
      if (mpinc < txpl) mpinc=txpl;
      int minus_remain=mpinc % txpl;
      int plus_remain=txpl -(mpinc % txpl);
      if (plus_remain > minus_remain)
        mpinc = mpinc - minus_remain;
      else
         mpinc = mpinc + plus_remain;
      if (mpinc==0) mpinc = mpinc + plus_remain;

    }
  }
  /* Check mpinc and if still invalid, exit with error */
  if ((mpinc % txpl) || (mpinc % 10) || (mpinc==0))  {
     sprintf(logtxt,"Error: mpinc: %d txpl: %d  nbaud: %d  rsep: %d", mpinc , txpl, nbaud, rsep);
     ErrLog(errlog.sock,progname,logtxt);
     exitpoll=1;
     SiteExit(0);
  }

  if(al_test->count > 0) {
        
    fprintf(stdout,"Control Program Argument Parameters::\n");
    fprintf(stdout,"  xcf arg:: count: %d value: %d xcnt: %d\n",ai_xcf->count,ai_xcf->ival[0],xcnt);
    fprintf(stdout,"  baud arg:: count: %d value: %d nbaud: %d\n",ai_baud->count,ai_baud->ival[0],nbaud);
    fprintf(stdout,"  clrskip arg:: count: %d value: %d\n",ai_clrskip->count,ai_clrskip->ival[0]);
    fprintf(stdout,"  cpid: %d progname: \'%s\'\n",cp,progname);
    fprintf(stdout,"Scan Sequence Parameters::\n");
    fprintf(stdout,"  txpl: %d mpinc: %d nbaud: %d rsep: %d\n",txpl,mpinc,nbaud,rsep);
    fprintf(stdout,"  intsc: %d intus: %d scnsc: %d scnus: %d nowait: %d\n",intsc,intus,scnsc,scnus,al_nowait->count);
    fprintf(stdout,"  sbm: %d ebm: %d  beams: %d\n",sbm,ebm,beams);
    
    fprintf(stdout,"  Total normal period: %lf secs\n",((float)total_integration_usecs)/1E6);
    fprintf(stdout,"  Total camp period: %lf secs\n",((float)total_camp_usecs)/1E6);
    if (ai_campfreq->count){
      fprintf(stdout,"End of Scan Camp Beam Parameters::\n");
      fprintf(stdout,"  campsc: %d campus: %d\n",campsc,campus);
      fprintf(stdout,"  camprep: %d\n",camprep);
      fprintf(stdout,"  Camp Freqs::\n");
      fprintf(stdout,"  txpl: %d mpinc: %d nbaud: %d rsep: %d\n",txpl_camp,mpinc_camp,nbaud_camp,rsep_camp);
      for(i=0;i<ai_campfreq->count;i++){
        fprintf(stdout,"  %d:: freq: %d\n",i,campfreqs[i]);
      }
    } else {
      fprintf(stdout,"No End of Scan Camp Beam Selected\n");

    }
    
    /* TODO: ADD PARAMETER CHECKING, SEE IF PCODE IS SANE AND WHATNOT */
   if(nbaud >= 1) {
        /* create tsgprm struct and pass to TSGMake, check if TSGMake makes something valid */
        /* checking with SiteTimeSeq(ptab); would be easier, but that talks to hardware..*/
        /* the job of aggregating a tsgprm from global variables should probably be a function in maketsg.c */
        int flag = 0;

        if (tsgprm.pat !=NULL) free(tsgprm.pat);
        if (tsgbuf !=NULL) TSGFree(tsgbuf);

        memset(&tsgprm,0,sizeof(struct TSGprm));   
        tsgprm.nrang = nrang;
        tsgprm.frang = frang;
        tsgprm.rsep = rsep; 
        tsgprm.smsep = smsep;
        tsgprm.txpl = txpl;
        tsgprm.mppul = mppul;
        tsgprm.mpinc = mpinc;
        tsgprm.mlag = 0;
        tsgprm.nbaud = nbaud;
        tsgprm.stdelay = 18 + 2;
        tsgprm.gort = 1;
        tsgprm.rtoxmin = 0;

        tsgprm.pat = malloc(sizeof(int)*mppul);
        tsgprm.code = ptab_scan;

        for (i=0;i<tsgprm.mppul;i++) tsgprm.pat[i]=ptab_scan[i];

        tsgbuf=TSGMake(&tsgprm,&flag);
        fprintf(stdout,"Sequence Parameters::\n");
        fprintf(stdout,"  lagfr: %d smsep: %d  txpl: %d\n",tsgprm.lagfr,tsgprm.smsep,tsgprm.txpl);
    
        if(tsgprm.smsep == 0 || tsgprm.lagfr == 0) {
            fprintf(stdout,"Sequence Parameters::\n");
            fprintf(stdout,"  lagfr: %d smsep: %d  txpl: %d\n",tsgprm.lagfr,tsgprm.smsep,tsgprm.txpl);
            fprintf(stdout,"WARNING: lagfr or smsep is zero, invalid timing sequence genrated from given baud/rsep/nrang/mpinc will confuse TSGMake and FitACF into segfaulting");
        }

        else {
            fprintf(stdout,"The phase coded timing sequence looks good\n");
        }

        tsgprm.nrang = nrang_camp;
        tsgprm.frang = frang;
        tsgprm.rsep = rsep_camp; 
        tsgprm.smsep = smsep;
        tsgprm.txpl = txpl_camp;
        tsgprm.mppul = mppul_camp;
        tsgprm.mpinc = mpinc_camp;
        tsgprm.mlag = 0;
        tsgprm.nbaud = nbaud_camp;
        tsgprm.stdelay = 18 + 2;
        tsgprm.gort = 1;
        tsgprm.rtoxmin = 0;

        tsgprm.pat = malloc(sizeof(int)*mppul_camp);
        tsgprm.code = ptab_camp;

        for (i=0;i<tsgprm.mppul;i++) tsgprm.pat[i]=ptab_camp[i];

        tsgbuf=TSGMake(&tsgprm,&flag);
        fprintf(stdout,"Camp Sequence Parameters::\n");
        fprintf(stdout,"  lagfr: %d smsep: %d  txpl: %d\n",tsgprm.lagfr,tsgprm.smsep,tsgprm.txpl);
    
        if(tsgprm.smsep == 0 || tsgprm.lagfr == 0) {
            fprintf(stdout," Camp Sequence Parameters::\n");
            fprintf(stdout,"  lagfr: %d smsep: %d  txpl: %d\n",tsgprm.lagfr,tsgprm.smsep,tsgprm.txpl);
            fprintf(stdout,"WARNING: lagfr or smsep is zero, invalid timing sequence genrated from given baud/rsep/nrang/mpinc will confuse TSGMake and FitACF into segfaulting");
        } else {
            fprintf(stdout,"The phase coded timing sequence looks good\n");
        }

    } else {
        fprintf(stdout,"WARNING: nbaud needs to be  > 0\n");
    }

    if (nerrors > 0) {
        fprintf(stdout,"Errors found in commandline arguements: \n");
        arg_print_errors(stdout,ae_argend,"spaletamode");
    }
    OpsFitACFStart();
 
    fprintf(stdout,"Test option enabled, exiting\n");
    return 0;
  }
  /* SiteSetupRadar, establish connection to ROS server and do initial setup of memory buffers for raw samples */
  printf("Running SiteSetupRadar Station ID: %s  %d\n",ststr,stid);
  status=SiteSetupRadar();
  if (status !=0) {
    ErrLog(errlog.sock,progname,"Error locating hardware.");
    exit (1);
  }

  printf("Preparing OpsFitACFStart Station ID: %s  %d\n",ststr,stid);
  OpsFitACFStart();

  printf("Preparing SiteTimeSeq Station ID: %s  %d\n",ststr,stid);
  tsgid=SiteTimeSeq(ptab_scan);

  scansync=1;
  printf("Entering Scan loop Station ID: %s  %d\n",ststr,stid);
  do {
    /* JDS: TODO Lets set the scan file lock here */
      scan_lock.l_type = F_RDLCK;
      if (fcntl(scan_fd, F_SETLK, &scan_lock) == -1) {
        perror("setlk scan_fd");
        exit(1);
      }
    /* JDS: TODO Lets test to see if the camp file lock is still active and wait for it to clear */
    lock_test=1;
    while(lock_test==1) {
      lock_test=0;
      check_lock.l_type = F_WRLCK;
      if (fcntl(scan_fd, F_GETLK, &check_lock) == -1) {
        perror("fcntl scan_fd");
        exit(1);
      }
      if(check_lock.l_type==F_UNLCK) {
        fprintf(stdout,"Start of Scan: scan_lock is unlocked\n");
      } else {
        fprintf(stdout,"Start of Scan: scan_lock is locked\n");
      }
      check_lock.l_type = F_WRLCK;
      if (fcntl(camp_fd, F_GETLK, &check_lock) == -1) {
        perror("fcntl camp_fd");
        exit(1);
      }
      if(check_lock.l_type==F_UNLCK) {
        fprintf(stdout,"Start of Scan: camp_lock is unlocked\n");
      } else {
        lock_test|=1;
        fprintf(stdout,"Start of Scan: camp_lock is locked\n");
        SiteWait(1,0);
      }
    }

    fprintf(stdout,"Site Start Scan\n");
    if (SiteStartScan() !=0) continue;
    if (OpsReOpen(2,0,0) !=0) {
      ErrLog(errlog.sock,progname,"Opening new files.");
      for (n=0;n<tnum;n++) {
        RMsgSndClose(task[n].sock);
        RMsgSndOpen(task[n].sock,strlen( (char *) command),command);     
      }
    }
    fprintf(stdout,"Finished with Site Start Scan\n");

    /* Setup things for scan pulse sequence */
    mppul=mppul_scan;
    mplgs=mplgs_scan;
    mpinc=mpinc_scan;
    nrang=nrang_scan;
    rsep=rsep_scan;
    txpl=txpl_scan;
    nbaud=nbaud_scan;
    intsc=intsc_scan;
    intus=intus_scan;
    pcode=pcode_scan;

    tsgid=SiteTimeSeq(ptab_scan);
    fprintf(stdout,"Finished with Site TimeSeq\n");

    gettimeofday(&s0,NULL);
    scan=1;
/*
    ErrLog(errlog.sock,progname,"Starting scan.");
*/
    if(al_clrscan->count) startup=1;
    if (xcnt>0) {
      cnt++;
      if (cnt==xcnt) {
        xcf=1;
        cnt=0;
      } else xcf=0;
    } else xcf=0;

    if(al_nowait->count==0){
      skip=OpsFindSkip(scnsc,scnus);
      printf("Skip: %d :: %d %d :: %d %d :: %d\n",skip,nscnsc,nscnus,intsc,intus,backward);
    }
    else skip=0;
    fprintf(stdout,"Finished with Find Skip\n");

    if (backward) {
      skip_scan=0;
      bmnum=sbm-skip;
      if (bmnum<ebm) skip_scan=1;
    } else {
      skip_scan=0;
      bmnum=sbm+skip;
      if (bmnum>ebm) skip_scan=1;
    }
    if ((beams==1) && (scansync==1)) skip_scan=1; 

    /* This starts the actual loop of N=beams number of scan beams 
     * with beam dwell time intsc/intus packed into scantime of scansc/scanus 
     */
    do {
      scansync=0;
      check_lock.l_type = F_WRLCK;
      if (fcntl(scan_fd, F_GETLK, &check_lock) == -1) {
        perror("getlk scan_fd");
        exit(1);
      }
      if(check_lock.l_type==F_UNLCK) {
        fprintf(stdout,"scan_lock is unlocked\n");
      } else {
        fprintf(stdout,"scan_lock is locked\n");
      }
      if(skip_scan) {
        fprintf(stdout,"Skipping Scan\n");
        break;
      }
      if (backward) {
        if (bmnum>sbm) bmnum=sbm;
        if (bmnum<ebm) bmnum=ebm;
      } else {
        if (bmnum<sbm) bmnum=sbm;
        if (bmnum>ebm) bmnum=ebm;
      }

      TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
/* TODO: JDS: You can not make any day night changes that impact TR gate timing at dual site locations. Care must be taken with day night operation*/      
      if (OpsDayNight()==1) {
        stfrq=dfrq;
      } else {
        stfrq=nfrq;
      }        
      if(ai_fixfrq->ival[0]>0) {
        stfrq=ai_fixfrq->ival[0];
        tfreq=ai_fixfrq->ival[0];
        noise=0; 
      }

      ErrLog(errlog.sock,progname,"Starting Integration.");
      sprintf(logtxt," Int parameters:: rsep: %d mpinc: %d sbm: %d ebm: %d nrang: %d nbaud: %d scannowait: %d clrskip_secs: %d clrscan: %d cpid: %d",
              rsep,mpinc,sbm,ebm,nrang,nbaud,al_nowait->count,ai_clrskip->ival[0],al_clrscan->count,cp);
      ErrLog(errlog.sock,progname,logtxt);

      sprintf(logtxt,"Integrating beam:%d intt:%ds.%dus (%d:%d:%d:%d)",bmnum,
                      intsc,intus,hr,mt,sc,us);
      ErrLog(errlog.sock,progname,logtxt);
            
      printf("Entering Site Start Intt Station ID: %s  %d\n",ststr,stid);
      SiteStartIntt(intsc,intus);
      gettimeofday(&t1,NULL);
      elapsed_secs=t1.tv_sec-t0.tv_sec;
      if(elapsed_secs<0) elapsed_secs=0;
      if((elapsed_secs >= ai_clrskip->ival[0])||(startup==1)) {
        startup=0;
        ErrLog(errlog.sock,progname,"Doing clear frequency search.");

        sprintf(logtxt, "FRQ: %d %d", stfrq, frqrng);
        ErrLog(errlog.sock,progname, logtxt);

        if(ai_fixfrq->ival[0]<=0) {
          tfreq=SiteFCLR(stfrq-frqrng/2,stfrq+frqrng/2);
        }
        t0.tv_sec=t1.tv_sec;
        t0.tv_usec=t1.tv_usec;
      }
      sprintf(logtxt,"Transmitting on: %d (Noise=%g)",tfreq,noise);
      ErrLog(errlog.sock,progname,logtxt);
    
      nave=SiteIntegrate(lags_scan);   
      if (nave<0) {
        sprintf(logtxt,"Integration error:%d",nave);
        ErrLog(errlog.sock,progname,logtxt); 
        continue;
      }
      sprintf(logtxt,"Number of sequences: %d",nave);
      ErrLog(errlog.sock,progname,logtxt);

      OpsBuildPrm(prm,ptab_scan,lags_scan);
      
      OpsBuildIQ(iq,&badtr);
            
      OpsBuildRaw(raw);
       
      FitACF(prm,raw,fblk,fit);
      
      msg.num=0;
      msg.tsize=0;

      tmpbuf=RadarParmFlatten(prm,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,
		PRM_TYPE,0); 

      tmpbuf=IQFlatten(iq,prm->nave,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,IQ_TYPE,0);

      RMsgSndAdd(&msg,sizeof(unsigned int)*2*iq->tbadtr,
                 (unsigned char *) badtr,BADTR_TYPE,0);
		 
      RMsgSndAdd(&msg,strlen(sharedmemory)+1,(unsigned char *) sharedmemory,
		 IQS_TYPE,0);

      tmpbuf=RawFlatten(raw,prm->nrang,prm->mplgs,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,RAW_TYPE,0); 
 
      tmpbuf=FitFlatten(fit,prm->nrang,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,FIT_TYPE,0); 

        
      RMsgSndAdd(&msg,strlen(progname)+1,(unsigned char *) progname,
		NME_TYPE,0);   
     

     
      for (n=0;n<tnum;n++) RMsgSndSend(task[n].sock,&msg); 

      for (n=0;n<msg.num;n++) {
        if (msg.data[n].type==PRM_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==IQ_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==RAW_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==FIT_TYPE) free(msg.ptr[n]); 
      }          
      if (exitpoll !=0) break;
      scan=0;
      gettimeofday(&s1,NULL);
      if (beams > 1) { 
        if (bmnum==ebm) break;
        if (backward) bmnum--;
        else {
          bmnum++;
        }
      } else {

        scan_elapsed_usecs=(s1.tv_sec-s0.tv_sec)*1E6;
        scan_elapsed_usecs+=(s1.tv_usec-s0.tv_usec);
        if (scan_elapsed_usecs >=total_scan_usecs || skip_scan) {
          break;
        } 

      }
    } while (1);
    /* JDS: TODO Lets clear our scan file lock if we hold it */
    scan_lock.l_type = F_UNLCK;
    if (fcntl(scan_fd, F_SETLK, &scan_lock) == -1) {
      perror("setlk scan_fd");
      exit(1);
    }

    /* 
      This ends the loop of N=beams number of scan beams 
    */

    /* JDS: TODO Lets test to see if the scan file lock is still active in a tight loop with a timeout */

    if ((exitpoll==0) && (skip_scan==0) && (ai_campfreq->count)) {
    
      /* JDS: TODO Lets set the camp file lock here*/
      camp_lock.l_type = F_RDLCK;
      if (fcntl(camp_fd, F_SETLK, &camp_lock) == -1) {
        perror("setlk camp_fd");
        exit(1);
      }
      /* JDS: TODO Lets test to see if the scan file lock is still active and wait for it to clear */
      lock_test=1;
      while(lock_test==1) {
        lock_test=0;
        check_lock.l_type = F_WRLCK;
        if (fcntl(scan_fd, F_GETLK, &check_lock) == -1) {
          perror("fcntl scan_fd");
          exit(1);
        }
        if(check_lock.l_type==F_UNLCK) {
          fprintf(stdout,"Start of Camp: scan_lock is unlocked\n");
        } else {
          lock_test|=1;
          fprintf(stdout,"Start of Camp: scan_lock is locked\n");
          SiteWait(1,0);
        }
        check_lock.l_type = F_WRLCK;
        if (fcntl(camp_fd, F_GETLK, &check_lock) == -1) {
          perror("fcntl camp_fd");
          exit(1);
        }
        if(check_lock.l_type==F_UNLCK) {
          fprintf(stdout,"Start of Camp: camp_lock is unlocked\n");
        } else {
          fprintf(stdout,"Start of Camp: camp_lock is locked\n");
        }
      }

      ErrLog(errlog.sock,progname,"Running Multi-frequency Camping Beam");
      /* Setup things for camp pulse sequence */
      mppul=mppul_scan;
      mplgs=mplgs_scan;
      mpinc=mpinc_scan;
      nrang=nrang_scan;
      rsep=rsep_scan;
      txpl=txpl_scan;
      nbaud=nbaud_scan;
      intsc=campsc;
      intus=campus;
      pcode=pcode_scan;
      tsgid=SiteTimeSeq(ptab_scan);
      bmnum=campbm;
      for(i=0;i<camprep;i++){
        ErrLog(errlog.sock,progname,"Starting Camp Rep");
        for(j=0;j<ai_campfreq->count;j++){
          lock_test=0;
          check_lock.l_type = F_WRLCK;
          if (fcntl(scan_fd, F_GETLK, &check_lock) == -1) {
            perror("fcntl scan_fd");
            exit(1);
          }
          if(check_lock.l_type!=F_UNLCK) {
            lock_test=1;
            fprintf(stdout,"Camp Check: scan_lock is locked! Aborting Camping run!\n");
          }
          if (lock_test==1) break;
          stfrq=campfreqs[j];
          sprintf(logtxt," Camp Int parameters:: rsep: %d mpinc: %d sbm: %d ebm: %d nrang: %d nbaud: %d scannowait: %d clrskip_secs: %d clrscan: %d cpid: %d",
              rsep,mpinc,sbm,ebm,nrang,nbaud,al_nowait->count,ai_clrskip->ival[0],al_clrscan->count,cp);
          ErrLog(errlog.sock,progname,logtxt);

          sprintf(logtxt,"Integrating campbeam:%d intt:%ds.%dus (%d:%d:%d:%d)",bmnum,
                      intsc,intus,hr,mt,sc,us);
          ErrLog(errlog.sock,progname,logtxt);

          printf("Entering Site Start Intt Station ID: %s  %d\n",ststr,stid);
          SiteStartIntt(intsc,intus);
          gettimeofday(&t1,NULL);
          elapsed_secs=t1.tv_sec-t0.tv_sec;
          if(elapsed_secs<0) elapsed_secs=0;
          if((elapsed_secs >= ai_clrskip->ival[0])||(startup==1)) {
            startup=0;
            ErrLog(errlog.sock,progname,"Doing clear frequency search.");

            sprintf(logtxt, "FRQ: %d %d", stfrq, frqrng);
            ErrLog(errlog.sock,progname, logtxt);

            if(ai_fixfrq->ival[0]<=0) {
              tfreq=SiteFCLR(stfrq-frqrng/2,stfrq+frqrng/2);
            }
            t0.tv_sec=t1.tv_sec;
            t0.tv_usec=t1.tv_usec;
          }
          sprintf(logtxt,"Transmitting on: %d (Noise=%g)",tfreq,noise);
          ErrLog(errlog.sock,progname,logtxt);

          nave=SiteIntegrate(lags_scan);
          if (nave<0) {
            sprintf(logtxt,"Integration error:%d",nave);
            ErrLog(errlog.sock,progname,logtxt);
            continue;
          }
          sprintf(logtxt,"Number of sequences: %d",nave);
          ErrLog(errlog.sock,progname,logtxt);

          OpsBuildPrm(prm,ptab_scan,lags_scan);
          OpsBuildIQ(iq,&badtr);
          OpsBuildRaw(raw);

          FitACF(prm,raw,fblk,fit);

          msg.num=0;
          msg.tsize=0;

          tmpbuf=RadarParmFlatten(prm,&tmpsze);
          RMsgSndAdd(&msg,tmpsze,tmpbuf,
                PRM_TYPE,0);

          tmpbuf=IQFlatten(iq,prm->nave,&tmpsze);
          RMsgSndAdd(&msg,tmpsze,tmpbuf,IQ_TYPE,0);

          RMsgSndAdd(&msg,sizeof(unsigned int)*2*iq->tbadtr,
                 (unsigned char *) badtr,BADTR_TYPE,0);

          RMsgSndAdd(&msg,strlen(sharedmemory)+1,(unsigned char *) sharedmemory,
                 IQS_TYPE,0);

          tmpbuf=RawFlatten(raw,prm->nrang,prm->mplgs,&tmpsze);
          RMsgSndAdd(&msg,tmpsze,tmpbuf,RAW_TYPE,0);

          tmpbuf=FitFlatten(fit,prm->nrang,&tmpsze);
          RMsgSndAdd(&msg,tmpsze,tmpbuf,FIT_TYPE,0);


          RMsgSndAdd(&msg,strlen(progname)+1,(unsigned char *) progname,
                NME_TYPE,0);

          for (n=0;n<tnum;n++) RMsgSndSend(task[n].sock,&msg);

          for (n=0;n<msg.num;n++) {
            if (msg.data[n].type==PRM_TYPE) free(msg.ptr[n]);
            if (msg.data[n].type==IQ_TYPE) free(msg.ptr[n]);
            if (msg.data[n].type==RAW_TYPE) free(msg.ptr[n]);
            if (msg.data[n].type==FIT_TYPE) free(msg.ptr[n]);
          }
          if (exitpoll !=0) break;

        }
        if (lock_test==1) break;
      }
      /* JDS: TODO Lets clear the camp file lock here*/
      camp_lock.l_type = F_UNLCK;
      if (fcntl(camp_fd, F_SETLK, &camp_lock) == -1) {
        perror("setlk camp_fd");
        exit(1);
      }
    }

    if ((exitpoll==0) && (al_nowait->count==0 || skip_scan)) {
      ErrLog(errlog.sock,progname,"Waiting for scan boundary.");
      SiteEndScan(scnsc,scnus);
    }
  } while (exitpoll==0);
  for (n=0;n<tnum;n++) RMsgSndClose(task[n].sock);
  
  /* free argtable and space allocated for arguements */
  arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
  free(ststr);
  free(roshost);
  
  ErrLog(errlog.sock,progname,"Ending program.");


  SiteExit(0);

  return 0;   
} 