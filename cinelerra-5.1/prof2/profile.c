#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <pthread.h>
#include <link.h>

#define smCommand "smap 2> /dev/null < "
#ifdef __i386__
#define nmAddr 0
#define nmType 9
#define nmName 11
#define _LX "%08lx"
#ifndef LIB
#define LIB "/lib"
#endif
#endif
#ifdef __x86_64__
#define nmAddr 0
#define nmType 17
#define nmName 19
#define _LX "%016lx"
#ifndef LIB
#define LIB "/lib64"
#endif
#endif

#define unlikely(x) __builtin_expect(!!(x), 0)
#define dprintf(s...) do { if( unlikely(prof_debug!=0) ) fprintf(s); } while(0)

int profileMain(int ac,char **av,char **ev);
void profileStart(void);
void profileStop(void);
void profileClear(void);
void profileShow(void);
void profileDone(void);

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/times.h>

#define MAP2MAX 256
#define MAPINCR 64
#define MOD2MAX 256
#define MODINCR 64
#define SYM2MAX 1024*256
#define SYMINCR 1024*16
#if 0
static volatile long prof_debug = 1;
static volatile long prof_type = 0;
static volatile long prof_tmrval = 10000;
static volatile unsigned long prof_stkptr = INT_MAX-0x1000;
static volatile char prof_output[256] = { "-" };
#else
static volatile long prof_debug; /* enable debug output */
static volatile long prof_type; /* 0=usr+sys, 1=usr only, 2=real time */
static volatile long prof_tmrval; /* profile timer interval in usecs */
static volatile unsigned long prof_stkptr; /* top of stack at profileStart */
static volatile char prof_output[256]; /* profile output file name */
#endif

static FILE *prof_fp = NULL;

typedef struct {
   char *name;
   unsigned long offs;
} tMap;

typedef struct {
   int n, m;
   tMap map[MAPINCR];
} tMapTbl;

typedef struct {
   char *name;
   unsigned long offs;
   int count;
   int calls;
} tMod;

typedef struct {
   int n, m;
   tMod mod[MODINCR];
} tModTbl;

typedef struct {
   char *name;
   unsigned long adr;
   int count;
   int calls;
   int tick;
   int tcalls;
   int mod;
} tSym;

typedef struct {
   int n, m;
   tSym sym[SYMINCR];
} tSymTbl;

static tSymTbl *tsym = NULL;
static tModTbl *tmod = NULL;
static tMapTbl *tmap = NULL;
static int nTicks;               /* profile timer tick count */
static struct tms start_cpu;     /* cpu time at profile_start */
static struct tms stop_cpu;      /* cpu time at profile_stop */
static struct timeval start_clk; /* real time at profile_start */
static struct timeval stop_clk;  /* real time at profile_stop */

int profileActive = 0;           /* flag, true if profile handler active */
static struct sigaction oldSig;  /* orignal profile handler data */
static struct sigaction oldSegv; /* orignal segv handler data */

static tMapTbl *
newMapTbl(void)
{
   tMapTbl *tp;
   if( (tp=malloc(sizeof(*tp))) != NULL ) {
      tp->n = 0;
      tp->m = sizeof(tp->map)/sizeof(tp->map[0]);
   }
   return tp;
}

static tModTbl *
newModTbl(void)
{
   tModTbl *tp;
   if( (tp=malloc(sizeof(*tp))) != NULL ) {
      tp->n = 0;
      tp->m = sizeof(tp->mod)/sizeof(tp->mod[0]);
   }
   return tp;
}

static tSymTbl *
newSymTbl(void)
{
   tSymTbl *tp;
   if( (tp=malloc(sizeof(*tp))) != NULL ) {
      tp->n = 0;
      tp->m = sizeof(tp->sym)/sizeof(tp->sym[0]);
   }
   return tp;
}

static tSym *
newSym(tSymTbl **ptp,const char *name,unsigned long adr,int mod)
{
   tSym *sp;
   tSymTbl *tp = *ptp;
   int n = tp->n;
   int m = tp->m;
   if( n >= m ) {
      tSymTbl *rp;  int l;
      m = m < SYM2MAX ? m*2 : m+SYMINCR;
      l = sizeof(*tp)-sizeof(tp->sym)+m*sizeof(tp->sym[0]);
      rp = (tSymTbl*) realloc(tp,l);
      if( rp == NULL ) return NULL;
      tp = rp;
      tp->m = m;
      *ptp = tp;
   }
   sp = &tp->sym[n++];
   tp->n = n;
   sp->name = strdup(name);
   sp->adr = adr;
   sp->mod = mod;
   sp->count = sp->calls = 0;
   sp->tick = sp->tcalls = 0;
   return sp;
}

static int
newMod(tModTbl **ptp, const char *name, unsigned long offs)
{
   tMod *mp;
   tModTbl *tp = *ptp;
   int n = tp->n;
   int m = tp->m;
   if( n >= m ) {
      tModTbl *rp;  int l;
      m = m < MOD2MAX ? m*2 : m+MODINCR;
      l = sizeof(*tp)-sizeof(tp->mod)+m*sizeof(tp->mod[0]);
      rp = (tModTbl*) realloc(tp,l);
      if( !rp ) return -1;
      tp = rp;
      *ptp = tp;
   }
   int ret = n++;
   mp = &tp->mod[ret];
   tp->n = n;
   mp->name = strdup(name);
   mp->offs = offs;
   mp->count = mp->calls = 0;
   return ret;
}

int
findMod(tModTbl *tp, const char *nm)
{
   int n = tp->n;
   while( --n >= 0 && strcmp(nm,tp->mod[n].name) );
   return n;
}

static int
newMap(tMapTbl **ptp, const char *name, unsigned long offs)
{
   tMap *mp;
   tMapTbl *tp = *ptp;
   int n = tp->n;
   int m = tp->m;
   if( n >= m ) {
      tMapTbl *rp;  int l;
      m = m < MAP2MAX ? m*2 : m+MAPINCR;
      l = sizeof(*tp)-sizeof(tp->map)+m*sizeof(tp->map[0]);
      rp = (tMapTbl*) realloc(tp,l);
      if( !rp ) return -1;
      tp = rp;
      *ptp = tp;
   }
   int ret = n++;
   mp = &tp->map[ret];
   tp->n = n;
   mp->name = strdup(name);
   mp->offs = offs;
   return ret;
}

static void
delMapTbl(tMapTbl **ptp)
{
   int i;  char *name;
   tMapTbl *tp;  tMap *mp;
   if( (tp=*ptp) == NULL ) return;
   mp = &tp->map[0];
   for( i=tp->n; --i>=0; ++mp ) {
      if( (name=mp->name) != NULL )
         free(name);
   }
   free(tp);
   *ptp = NULL;
}

static void
delModTbl(tModTbl **ptp)
{
   int i;  char *name;
   tModTbl *tp;  tMod *mp;
   if( (tp=*ptp) == NULL ) return;
   mp = &tp->mod[0];
   for( i=tp->n; --i>=0; ++mp ) {
      if( (name=mp->name) != NULL )
         free(name);
   }
   free(tp);
   *ptp = NULL;
}

static void
delSymTbl(tSymTbl **ptp)
{
   int i;  char *name;
   tSymTbl *tp;  tSym *sp;
   if( (tp=*ptp) == NULL ) return;
   sp = &tp->sym[0];
   for( i=tp->n; --i>=0; ++sp ) {
      if( (name=sp->name) != NULL )
         free(name);
   }
   free(tp);
   *ptp = NULL;
}

static void
delAllTbls(void)
{
   delMapTbl(&tmap);
   delModTbl(&tmod);
   delSymTbl(&tsym);
}

static int
mapLkup(tMapTbl *tp,const char *nm)
{
   tMap *mp = &tp->map[0];
   int n = tp->n;
   while( --n >= 0 && strcmp(nm,mp->name)!=0 )
     ++mp;
   return n;
}

static int
findMap(tMapTbl *tp,char *nm)
{
   int ret = mapLkup(tp,nm);
   if( ret < 0 )
      dprintf(stderr,"cant find map '%s'\n",nm);
   return ret;
}

static int
findSymAdr(tSymTbl *tp,const char *nm,unsigned long *adr)
{
  int n = tp->n;
  while( --n >= 0 && strcmp(nm,tp->sym[n].name) );
  if( n < 0 ) return 1;
  *adr = tp->sym[n].adr;
  return 0;
}

static int
isSymNm(char *cp, char *bp)
{
   int ch;
   while( (ch=*cp++) != 0 ) {
      if( ch == '\n' ) break;
      *bp++ = ch;
      if( ch == '#' || ch == '$' || ch == '.' ) return 0;
   }
   *bp = 0;
   return 1;
}

static int
hex(char *cp, unsigned long *val)
{
   char *bp = cp;
   *val = strtoul(cp,&bp,16);
   return bp - cp;
}

static int
mapMod(char *fn,tSymTbl **ptp, int mod)
{
   char nmLine[512], nmCmd[512], *lp;
   unsigned long adr, ofs;  int typ;
   FILE *fp;  tMod *mp;
   if( access(fn, R_OK) < 0 ) return -1;
   strcpy(&nmCmd[0],smCommand);
   strcat(&nmCmd[0],fn);
   lp = NULL;
   if( (fp=popen(&nmCmd[0],"r")) == NULL ) return -1;
   mp = &tmod->mod[mod];  ofs = mp->offs;
   while((lp=fgets(&nmLine[0],sizeof nmLine,fp)) != NULL ) {
      typ = nmLine[nmType];
      if( typ != 'T' && typ != 't' ) continue;
      if( isSymNm(&nmLine[nmName],&nmCmd[0]) == 0 ) continue;
      if( hex(&nmLine[nmAddr],&adr) != nmType-1 ) continue;
      adr += ofs;
      if( newSym(ptp,&nmCmd[0],adr,mod) == NULL ) break;
   } while((lp=fgets(&nmLine[0],sizeof nmLine,fp)) != NULL );
   pclose(fp);
   return lp != NULL ? -1 : 0;
}

static int
adrCmpr(const void *a, const void *b)
{
   unsigned long aadr = ((tSym*)a)->adr;
   unsigned long badr = ((tSym*)b)->adr;
   if( aadr > badr ) return 1;
   if( aadr < badr ) return -1;
   return 0;
}

static int
cntCmpr(const void *a, const void *b)
{
   int acnt = ((tSym*)a)->count;
   int bcnt = ((tSym*)b)->count;
   return acnt - bcnt;
}

static int
tclCmpr(const void *a, const void *b)
{
   int atcl = ((tSym*)a)->tcalls;
   int btcl = ((tSym*)b)->tcalls;
   return atcl - btcl;
}

static int
modCmpr(const void *a, const void *b)
{
   int acnt = ((tMod*)a)->count;
   int bcnt = ((tMod*)b)->count;
   return acnt - bcnt;
}

static int
profile_tally(unsigned long pc, int cnt)
{
   tSym *sp, *tp;
   int l, m, r;
   if( tsym == NULL )
      return -1;
   sp = tp = &tsym->sym[0];
   l = -1;  r = tsym->n;
 /* find function containing pc */
   while( (r-l) > 1 ) {
      m = (r+l) >> 1;
      sp = &tp[m];
      if( sp->adr == pc ) break;
      if( sp->adr < pc ) l = m;
      else r = m;
   }
   if( sp->adr != pc ) {
      if( l < 0 )
         return -1;
      sp = &tp[l];
   }
   tMod *mp = &tmod->mod[sp->mod];
/* incr function/module calls/counts */
   ++sp->calls;
   ++mp->calls;
/* only 1 call per unwinding */
   if( sp->tick < nTicks ) {
      sp->tick = nTicks;
      ++sp->tcalls;
   }
   if( cnt != 0 ) {
      ++sp->count;
      ++mp->count;
   }
   return 0;
}

struct reg_frame {
   unsigned long rf_fp;        /* frame pointer */
   unsigned long rf_rtn;       /* return addr */
};

static int readProcMaps(unsigned long *fwa)
{
   int ret, n, m, fd, len;
   unsigned long vm_start, vm_end, ino;
   char mr, mw, mx, ms, *bp;
   off_t pgoff;
   unsigned int maj, min;
   char mfn[PATH_MAX], ifn[PATH_MAX];
   char bfr[65536];
   pid_t pid = getpid();
   ret = 1;
   snprintf(&mfn[0],sizeof mfn,"/proc/%d/maps",pid);
   fd = open(&mfn[0],O_RDONLY);
   if( fd >= 0 ) {
      len = read(fd,bp=&bfr[0],sizeof bfr);
      close(fd);
      while( len > 0 ) {
         m = sscanf(bp, "%lx-%lx %c%c%c%c %lx %x:%x %lu%n",
            &vm_start,&vm_end,&mr,&mw,&mx,&ms,&pgoff,&maj,&min,&ino,&n);
         if( m == 10 && fwa ) { *fwa = vm_start;  fwa = 0; }
         if( m == 10 && mx == 'x' && pgoff == 0 ) {
            for( bp+=n,len-=n ; len>0 && *bp!='\n' && *bp==' '; ++bp,--len );
            for( m=0 ; len>0 && *bp!='\n' && m<PATH_MAX-1; ++bp,--len )
               ifn[m++] = *bp;
            ifn[m] = 0;
            if( maj !=0 || min != 0 ) {
               dprintf(stderr," map "_LX" %s\n",vm_start,&ifn[0]);
               newMap(&tmap,&ifn[0],vm_start);
            }
         }
         while( --len>=0 && *bp++!='\n' );
      }
      ret = 0;
   }
   return ret;
}

static int readNmMaps(unsigned long fwa)
{
   unsigned long adr;
   int i, k;
   char dfn[PATH_MAX], efn[PATH_MAX], pfn[PATH_MAX];
   struct link_map *so;
   struct r_debug *rdebug = &_r_debug;
   for( so=rdebug->r_map; so!=NULL; so=so->l_next ) {
      int n = tsym->n;
      char *nm = &so->l_name[0];
      if( nm[0] == 0 ) nm = "/proc/self/exe";
      dprintf(stderr,"resolving %s\n",nm);
      if( nm[0] == '.' && nm[1] == '/' ) {
        getcwd(&pfn[0],sizeof(pfn));
        strcat(&pfn[0],&nm[1]);
        nm = &pfn[0];
      }
      while( (i=readlink(nm,&efn[0],sizeof(efn)-1)) >= 0 ) {
        efn[i] = 0;
        dprintf(stderr,"  link %s\n",&efn[0]);
        if( efn[0] != '/' ) { /* get dirname */
           for( k=-1,i=0; i<sizeof(dfn)-1 && (dfn[i++]=*nm)!=0; ++nm )
             if( *nm == '/' ) k = i;
           if( k >= 0 ) {     /* add link name */
             for( i=0; k<sizeof(dfn)-1 && (dfn[k++]=efn[i])!=0; ++i );
           }
           strncpy(&efn[0],&dfn[0],sizeof(efn));
        }
        nm = &efn[0];
        dprintf(stderr,"  path %s\n",nm);
      }
      if( findMod(tmod,nm) >= 0 ) continue;
      int map = !so->l_name[0] ? -1 : findMap(tmap, nm);
      tMap *mp = map>=0 ? &tmap->map[map] : 0;
      adr = mp ? mp->offs : (unsigned long) so->l_addr;
      if( !adr ) adr = fwa;
      dprintf(stderr,"  map %s adr = 0x"_LX" ofs = 0x"_LX"\n",
          nm,adr,(unsigned long)so->l_addr);
      int mod = newMod(&tmod,nm,adr);
      if( mod >= 0 ) {
         if( mapMod(nm,&tsym,mod) < 0 || tsym->n == n ) {
            char dfn[PATH_MAX];  memset(dfn,0,sizeof(dfn));
            strcpy(&dfn[0],"/usr/lib/debug");
            strncat(&dfn[0],nm,sizeof(dfn)-1);
            strncat(&dfn[0],".debug",sizeof(dfn)-1);
            dprintf(stderr,", %s",&dfn[0]);
            if( mapMod(&dfn[0],&tsym,mod) < 0 )
              mod = -1;
         }
      }
      dprintf(stderr,", %d symbols\n",tsym->n - n);
      if( mod < 0 && strncmp("linux-vdso.so.",nm,14) ) {
         fprintf(stderr,"profile - Cannot map module - %s\n",nm);
//         exit(1);
//         return -1;
      }
   }

   /* sort the symbols by address */
   qsort(&tsym->sym[0],tsym->n,sizeof(tsym->sym[0]),adrCmpr);
   if( prof_debug != 0 ) {
      int i; tSym *sp;
      sp = &tsym->sym[0];
      for( i=tsym->n; --i>=0; ++sp ) {
         tMod *mp = &tmod->mod[sp->mod];
         fprintf(stdout,_LX" %-24s %s\n",sp->adr,
            &sp->name[0],&mp->name[0]);
      }
   }
   return 0;
}

/** profile_handler(sign,c)
 * profile periodic timer signal handler.
 * decodes pc/fp from sigcontext, and tallys pc (count)
 *  and stack trace (calls).
 */
#define LOW_TEXT 0x01000UL

static void profile_handler(int n, siginfo_t * info, void *sc)
{
  ucontext_t *uc = (ucontext_t *)sc;
  struct sigcontext *c = (struct sigcontext *)&uc->uc_mcontext;
   struct reg_frame *fp, *last_fp;
   unsigned long pc;
   /* tid marks top of thread stack (currently) */
   unsigned long tid = (unsigned long)pthread_self();
   ++nTicks;
   /* tally pc value */
#ifdef __i386__
   pc = (unsigned long)c->eip;
#define ARCH_OK
#endif
#ifdef __x86_64__
   pc = (unsigned long)c->rip;
#define ARCH_OK
#endif
#ifndef ARCH_OK
#error unknown arch
#endif
   if( pc < LOW_TEXT ) return;
   if( profile_tally(pc,1) != 0 ) return;

   /* access frame pointer and validate */
#ifdef __i386__
   fp = (struct reg_frame *)c->ebp;
   if( (unsigned long)fp < c->esp ) return;
#endif
#ifdef __x86_64__
   fp = (struct reg_frame *)c->rbp;
   if( (unsigned long)fp < c->rsp ) return;
#endif
   if( (unsigned long)fp > prof_stkptr) return;
   if( (unsigned long)fp > tid) return;
   dprintf(stderr,"unwind "_LX"",(unsigned long)pc);
   /* unwind the stack frames, and tally pc values */
   while( (last_fp=fp) != 0 ) {
      pc = fp->rf_rtn;
      dprintf(stderr," "_LX"",(unsigned long)pc);
      if( pc < LOW_TEXT ) break;
      if( profile_tally(pc,0) != 0 ) break;
      fp = (struct reg_frame *)fp->rf_fp;
      if( ((long)fp & 3) != 0 ) break;
      if( fp <= last_fp ) break;
      if( (unsigned long)fp > prof_stkptr ) break;
      if( (unsigned long)fp > tid ) break;
   }
   dprintf(stderr,"\n");

   return;
}

void profileExit(void)
{
   profileStop();
   profileShow();
   profileDone();
}

static void profile_segv(int n, siginfo_t * info, void *sc)
{
  ucontext_t *uc = (ucontext_t *)sc;
  struct sigcontext *c = (struct sigcontext *)&uc->uc_mcontext;
  profileExit();
  fprintf(stderr,"segv at 0x"_LX"\n",
#if __i386__
    c->eip
#endif
#if __x86_64__
    c->rip
#endif
    );
}

void profileStart(void)
{
   char *fn;
   int typ, sig;
   struct itimerval it;
   struct timezone tz;
   struct sigaction act;
   struct sigaction segv;
   unsigned long fwa;

   if( profileActive != 0 ) return;

   if( tmod == NULL || tsym == NULL || tmap == NULL ) {
      setbuf(stderr,NULL);
      dprintf(stderr,"start profile\n");
      delAllTbls();
      unsetenv("LD_PRELOAD");
      profileDone();
      fn = (char *)&prof_output[0];
      if( strcmp("-",&fn[0]) == 0 )
         prof_fp = stdout;
      else if( strcmp("--",&fn[0]) == 0 )
         prof_fp = stderr;
      else
         prof_fp = fopen(&fn[0],"w");
      if( prof_fp == NULL ) {
         perror(&fn[0]);
         fprintf(stderr,"profile: no output path\n");
         exit(1);
      }
      tmap = newMapTbl();  fwa = 0;
      if( tmap != NULL && readProcMaps(&fwa) == 0 ) {
         tmod = newModTbl();  tsym = newSymTbl();
         if( tmod == NULL || tsym == NULL || readNmMaps(fwa) != 0 ) {
            fprintf(stderr,"profile: unable to read nm maps\n");
            delModTbl(&tmod);  delSymTbl(&tsym);
         }
      }
      else {
         fprintf(stderr,"profile: unable to read proc maps\n");
         delMapTbl(&tmap);
      }
   }
   if( tmap == NULL || tmod == NULL || tsym == NULL ) {
      delAllTbls();
      return;
   }

   switch( prof_type ) {
   case 1:  typ = ITIMER_VIRTUAL;  sig = SIGVTALRM;  break;
   case 2:  typ = ITIMER_REAL;     sig = SIGALRM;    break;
   default: typ = ITIMER_PROF;     sig = SIGPROF;    break;
   }

   /* record the start time (cpu/real). */
   times(&start_cpu);
   gettimeofday(&start_clk,&tz);

   /* enable the profile timer signal handler. */
   profileActive = 1;
   memset(&act,0,sizeof(act));
   act.sa_sigaction = profile_handler;
   act.sa_flags = SA_RESTART + SA_SIGINFO;
   sigaction(sig, &act, &oldSig);
   memset(&segv,0,sizeof(segv));
   segv.sa_sigaction = profile_segv;
   segv.sa_flags = SA_SIGINFO;
   sigaction(SIGSEGV,&segv,&oldSegv);

   /* start the periodic profile timer signal */
   if( prof_tmrval == 0 ) prof_tmrval = 1;
   it.it_value.tv_sec = 
   it.it_interval.tv_sec = prof_tmrval / 1000000;
   it.it_value.tv_usec =
   it.it_interval.tv_usec = prof_tmrval % 1000000;
   setitimer(typ, &it, NULL);
   dprintf(stderr,"starting profile.\n");
}

static void sigusr1(int n)
{
   profileStop();
   profileShow();
   delAllTbls();
   profileStart();
}

int profileMain(int ac,char **av,char **ev)
{
   int (*fmain)(int ac,char **av,char **ev);
   unsigned long adr;
   profileStart();
   if( findSymAdr(tsym,"main",&adr) != 0 ) {
     fprintf(stderr,"cant locate sym \"main\"\n");
     exit(1);
   }
   atexit(profileExit);
   signal(SIGUSR1, sigusr1);
   fmain = (int (*)(int,char **,char **))adr;
   dprintf(stderr,"starting \"main\" at: %p\n",fmain);
   int ret = fmain(ac,av,ev);
   dprintf(stderr,"ended \"main\" = %d\n",ret);
   exit(ret);
}

void profileStop(void)
{
   struct itimerval it;
   struct timezone tz;
   struct sigaction act;

   if( profileActive == 0 ) return;
   dprintf(stderr,"stopping profile.\n");

   /* records the stop time (cpu/real). */
   times(&stop_cpu);
   gettimeofday(&stop_clk,&tz);

   /* disables the profile timer signal handler. */
   it.it_interval.tv_sec = 0;
   it.it_interval.tv_usec = 0;
   it.it_value.tv_sec = 0;
   it.it_value.tv_usec = 0;
   setitimer(ITIMER_PROF, &it, NULL);

   /* restore the original profile signal handler. */
   sigaction(SIGPROF, &oldSig, &act);
   sigaction(SIGSEGV, &oldSegv, &act);
   profileActive = 0;
   return;
}


void profileClear(void)
{
   int i;
   nTicks = 0;
   if( tsym != NULL ) {
      tSym *sp = &tsym->sym[0];
      for( i=tsym->n; --i>=0; ++sp ) {
         sp->count = sp->calls = 0;
         sp->tick = sp->tcalls = 0;
      }
   }
   if( tmod != NULL ) {
      tMod *mp = &tmod->mod[0];
      for( i=tmod->n; --i>=0; ++mp )
         mp->count = mp->calls = 0;
   }
} 


void profileShow(void)
{
   FILE *fp; int i;
   double userDt, systDt;
   double tickDt, realDt;
   if( tsym == NULL || tmod == NULL ) return;
   fp = prof_fp;
   qsort(&tsym->sym[0],tsym->n,sizeof(tsym->sym[0]),cntCmpr);
   /* outputs sorted (by count) list of functions called. */
   fprintf(fp,"---- profile start ----\n");
   fprintf(fp," %d ticks %d modules %d syms\n",nTicks,tmod->n,tsym->n);
   tSym *sp = &tsym->sym[0];
   for( i=tsym->n; --i>=0; ++sp ) {
      if( !sp->count ) continue;
      tMod *mp = &tmod->mod[sp->mod];
      fprintf(fp,"%8.3fs %5.1f%% %-24s %s\n",
        prof_tmrval*sp->count/1000000.0,
        sp->count*100.0/nTicks,&sp->name[0],
        &mp->name[0]);
   }

   qsort(&tsym->sym[0],tsym->n,sizeof(tsym->sym[0]),tclCmpr);
   /* outputs sorted (by calls) list of functions called. */
   fprintf(fp,"---- profile calls ----\n");
   sp = &tsym->sym[0];
   for( i=tsym->n; --i>=0; ++sp ) {
      if( !sp->tcalls ) continue;
      tMod *mp = &tmod->mod[sp->mod];
      fprintf(fp,"%8.3fs %5.1f%% %-24s %5.1f %s\n",
        prof_tmrval*sp->tcalls/1000000.0,
        sp->tcalls*100.0/nTicks,&sp->name[0],
        (double)sp->calls/sp->tcalls,&mp->name[0]);
   }

   realDt = ((stop_clk.tv_sec - start_clk.tv_sec) * 1000000.0 +
               (stop_clk.tv_usec - start_clk.tv_usec) ) / 1000000.0;
   /* output sorted (by count) list of modules called. */
   qsort(&tmod->mod[0],tmod->n,sizeof(tmod->mod[0]),modCmpr);
   fprintf(fp,"----\n");
   tMod *mp = &tmod->mod[0];
   for( i=tmod->n; --i>=0; ++mp ) {
      if( mp->count != 0 && mp->name != NULL ) {
         double dt = prof_tmrval*mp->count/1000000.0;
         fprintf(fp,"%8.3fs %5.1f%%/%5.1f%% %s\n",
            dt,mp->count*100.0/nTicks,dt*100.0/realDt,mp->name);
      }
   }

   /* output tick time, cpu user/kernal time, real time. */
   tickDt = prof_tmrval*nTicks / 1000000.0;
   userDt = (stop_cpu.tms_utime - start_cpu.tms_utime) / (double)CLOCKS_PER_SEC;
   systDt = (stop_cpu.tms_stime - start_cpu.tms_stime) / (double)CLOCKS_PER_SEC;

   fprintf(fp,"%8.3ft %0.3fu+%0.3fs %0.3fr %5.1f%%\n",
      tickDt, userDt, systDt, realDt, tickDt*100.0/realDt);
   fprintf(fp,"---- profile end ----\n\n");
   qsort(&tsym->sym[0],tsym->n,sizeof(tsym->sym[0]),adrCmpr);
}

void profileDone(void)
{
   profileStop();
   delModTbl(&tmod);
   delSymTbl(&tsym);
   if( prof_fp != NULL && prof_fp != stdout && prof_fp != stderr ) {
      fclose(prof_fp);  prof_fp = NULL;
   }
}

