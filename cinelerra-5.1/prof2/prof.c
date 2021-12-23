/* prof.c - initialize sampling profiler
    load user executable in ptrace mode
    at exec catchpoint, set breakpoint at main
    add LD_PRELOAD = libprofile
    run ld-linux to map libraries, stop at main
    add ptr to main as argument, start profileStart
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <link.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sys.h"

#define unlikely(x) __builtin_expect(!!(x), 0)
#define dprintf(s...) do { if( unlikely(debug!=0) ) fprintf(s); } while(0)

#ifdef __i386__
#define nmAddr 0
#define nmType 9
#define nmName 11
#define _LX "%08lx"
#ifndef LIB
#define LIB "/lib/"
#endif
#endif
#ifdef __x86_64__
#define nmAddr 0
#define nmType 17
#define nmName 19
#define _LX "%016lx"
#ifndef LIB
#define LIB "/lib64/"
#endif
#endif

#define SYM2MAX 1024*256
#define SYMINCR 1024

#define DEBUG 0
#define PROF_DEBUG 0
#define PROF_OUTPUT "--"
#define PROF_TMR_TYPE 0
#define PROF_TMR_USEC 10000

static int debug = DEBUG;
static int prof_debug = PROF_DEBUG;
static int prof_type = PROF_TMR_TYPE;
static int prof_tmrval = PROF_TMR_USEC;
static long prof_stkptr = 0;
static char *prof_output = NULL;
static const int max_signals = sizeof(signal_name)/sizeof(signal_name[0]);
static const int max_sysreqs = sizeof(sysreq_name)/sizeof(sysreq_name[0]);

#define LIBPROFILE_NAME "libprofile.so"
#define LIBPROFILE_PATH LIB LIBPROFILE_NAME

#ifdef __i386__
#define LD_LINUX_SO LIB "ld-linux.so.2"
#endif
#ifdef __x86_64__
#define LD_LINUX_SO LIB "ld-linux-x86-64.so.2"
#endif

char *libprofile_path = NULL;

typedef struct {
   pid_t pid;
   unsigned long addr;
   long orig_value;
   int enabled;
} breakpoint;

typedef struct {
  char *nm;
  unsigned long adr;
} tSym;

typedef struct {
  int n, m;
  tSym sym[SYMINCR];
} tSymTbl;

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

static int
symCmpr(const void *a, const void *b)
{
   const char *anm = ((tSym const*)a)->nm;
   const char *bnm = ((tSym const*)b)->nm;
   return strcmp(anm,bnm);
}

static int
newSym(tSymTbl **ptp, const char *nm, unsigned long adr)
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
      if( rp == NULL ) return -1;
      tp = rp;
      tp->m = m;
      *ptp = tp;
   }
   sp = &tp->sym[n++];
   tp->n = n;
   sp->nm = strdup(nm);
   sp->adr = adr;
   return 0;
}
static void
delSymTbl(tSymTbl **ptp)
{
   int i;  char *nm;
   tSymTbl *tp = *ptp;
   tSym *sp = &tp->sym[0];
   for( i=tp->n; --i>=0; ++sp ) {
      if( (nm=sp->nm) != NULL )
         free(nm);
   }
   free(tp);
   *ptp = NULL;
}

static int
isSymNm(char *cp, char *bp)
{
   if( !*cp || *cp=='\n' ) return 0;
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
mapObj(char *fn,tSymTbl **ptp,const char *typs)
{
   char nmLine[512], nmCmd[512], *lp, *rp;
   unsigned long adr;
   FILE *fp;
   if( access(fn,R_OK) < 0 ) return -1;
   strcpy(&nmCmd[0],"smap 2> /dev/null < ");
   strcat(&nmCmd[0],fn);
   if( (fp=popen(&nmCmd[0],"r")) == NULL ) return -1;
   while((lp=fgets(&nmLine[0],sizeof nmLine,fp)) != NULL ) {
      int t = nmLine[nmType];
      if( t != 'T' && t != 't' && (!typs || !strchr(typs,t)) ) continue;
      if( isSymNm(&nmLine[nmName],&nmCmd[0]) == 0 ) continue;
      adr = strtoul(&nmLine[nmAddr],&rp,16);
      if( (rp-&nmLine[nmAddr]) != nmType-1 ) continue;
      if( newSym(ptp,&nmCmd[0],adr) != 0 ) break;
   }
   pclose(fp);
   return lp != NULL ? -1 : 0;
}


static tSymTbl *
mapFile(char *fn,const char *typs)
{
   tSymTbl *tp = newSymTbl();
   if( tp && mapObj(fn,&tp,typs) != 0 )
     delSymTbl(&tp);
   if( tp )
     qsort(&tp->sym,tp->n,sizeof(tp->sym[0]),symCmpr);
   return tp;
}

static tSym *
symLkup(tSymTbl *tp,const char *nm)
{
   int l, m, n, r;
   tSym *mp = NULL;
   tSym *sp = &tp->sym[0];
   l = n = -1;  r = tp->n;
   while( (r-l) > 1 ) {
      m = (r+l) >> 1;
      mp = &sp[m];
      n = strcmp(mp->nm,nm);
      if( n == 0 ) break;
      if( n < 0 ) l = m;
      else r = m;
   }
   return n == 0 ? mp : NULL;
}

static void
findSym(tSymTbl *tp,char *nm,tSym **psp)
{
   tSym *sp = symLkup(tp,nm);
   if( sp == NULL ) {
      fprintf(stderr,"cant find symbol '%s'\n",nm);
      exit(1);
   }
   *psp = sp;
}

int
execute_program(char *fn,char **argv)
{
   pid_t pid = fork();
   if( pid < 0 ) {
      perror("fork");
      return -1;
   }
   if( pid != 0 )
      return pid;
   if (ptrace(PTRACE_TRACEME, 0, 1, 0)<0) {
      perror("PTRACE_TRACEME");
      exit(1);
   }
   setenv("LD_PRELOAD",libprofile_path,1);
   //setenv("LD_DEBUG","files",1);
   execvp(fn,argv);
   fprintf(stderr, "Can't execute `%s': %s\n", fn, strerror(errno));
   exit(1);
}

#ifdef __i386__
static long
peek_text(pid_t pid,long addr)
{
   return ptrace(PTRACE_PEEKTEXT,pid,addr,0);
}

static void
poke_text(pid_t pid,long addr,long value)
{
   ptrace(PTRACE_POKETEXT,pid,addr,value);
}

static long
get_eip(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.eip;
}

static void
set_eip(pid_t pid,long addr)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   regs.eip = addr;
   ptrace(PTRACE_SETREGS,pid,0,&regs);
}

static long
get_sysreq(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.orig_eax;
}

static long
get_sysret(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.eax;
}

static long
get_esp(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.esp;
}

static long
peek_stk(pid_t pid,long addr)
{
   return ptrace(PTRACE_PEEKDATA,pid,addr,0);
}

static void
poke_stk(pid_t pid,long addr,long value)
{
   ptrace(PTRACE_POKEDATA,pid,addr,value);
}

#endif

#ifdef __x86_64__
static long
peek_text(pid_t pid,long addr)
{
   long ret = ptrace(PTRACE_PEEKTEXT,pid,addr,0);
   return ret;
}

static void
poke_text(pid_t pid,long addr,long value)
{
   ptrace(PTRACE_POKETEXT,pid,addr,value);
}

static long
get_eip(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.rip;
}

static void
set_eip(pid_t pid,long addr)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   regs.rip = addr;
   ptrace(PTRACE_SETREGS,pid,0,&regs);
}

static long
get_sysreq(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.orig_rax;
}

static long
get_sysret(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.rax;
}

static long
get_esp(pid_t pid)
{
   struct user_regs_struct regs;
   ptrace(PTRACE_GETREGS,pid,0,&regs);
   return regs.rsp;
}

static long
peek_stk(pid_t pid,long addr)
{
   errno = 0;
   long ret = ptrace(PTRACE_PEEKDATA,pid,addr,0);
   if( ret == -1 && errno )
      dprintf(stderr,"peek_stk failed @ "_LX" = %d:%s",
         addr, errno, strerror(errno));
   return ret;
}

static void
poke_stk(pid_t pid,long addr,long value)
{
   ptrace(PTRACE_POKEDATA,pid,addr,value);
}
#endif

static int
pstrcmp(pid_t pid,long addr,char *cp)
{
   int n, v;  long w, adr;
   adr = addr & ~(sizeof(w)-1);
   n = addr & (sizeof(w)-1);
   w = peek_stk(pid,adr);
   adr += sizeof(long);
   w >>= n*8;
   n = sizeof(w) - n;
   for(;;) {
      if( (v=(w&0xff)-*cp) != 0 ) break;
      if( *cp == 0 ) break;
      if( --n <= 0 ) {
         w = peek_stk(pid,adr);
         adr += sizeof(w);
         n = sizeof(w);
      }
      else
         w >>= 8;
      ++cp;
   }
   return v;
}

static char *
pstrcpy(pid_t pid,long addr,char *cp)
{
   long w, by;
   unsigned char *bp = (unsigned char *)cp;
   long adr = addr & ~(sizeof(w)-1);
   int n = 8*(addr & (sizeof(w)-1));
   w = peek_stk(pid,adr);
   do {
      by = *bp++;
      w &= ~(0xffUL << n);
      w |= by << n;
      if( (n+=8) >= sizeof(w)*8 ) {
         poke_stk(pid,adr,w);
         adr += sizeof(w);
         w = peek_stk(pid,adr);
         n = 0;
      }
   } while( by );
   if( n )
     poke_stk(pid,adr,w);
   return cp;
}

/* instruction = int 3 */
#define BREAKPOINT_VALUE 0xcc
#define BREAKPOINT_LENGTH 1
#define DECR_PC_AFTER_BREAK 1

void
enable_breakpoint(breakpoint *bp)
{
   if( bp->enabled == 0 ) {
      int shft;  long word, mask;
      pid_t pid = bp->pid;
      unsigned long addr = bp->addr & ~(sizeof(long)-1);
      word = peek_text(pid,addr);
      bp->orig_value = word;
      shft = 8 * (bp->addr&(sizeof(long)-1));
      mask = 0xffUL << shft;
      word = (word & ~mask) | ((unsigned long)BREAKPOINT_VALUE << shft);
      poke_text(pid,addr,word);
      bp->enabled = 1;
      dprintf(stderr,"set bp "_LX"\n",bp->addr);
   }
}

void
disable_breakpoint(breakpoint *bp)
{
   if( bp->enabled != 0 ) {
      int shft;  long word, mask;
      pid_t pid = bp->pid;
      unsigned long addr = bp->addr & ~(sizeof(long)-1);
      word = peek_text(pid,addr);
      shft = 8 * (bp->addr&(sizeof(long)-1));
      mask = 0xffUL << shft;
      word = (word & ~mask) | (bp->orig_value & mask);
      poke_text(pid,addr,word);
      bp->enabled = 0;
   }
}

breakpoint *newBrkpt(pid_t pid,unsigned long addr)
{
   breakpoint *bp;
   bp = (breakpoint *)malloc(sizeof(*bp));
   memset(bp,0,sizeof(*bp));
   bp->pid = pid;
   bp->addr = addr;
   return bp;
}

static long
get_map_fwa(pid_t pid,char *pfx1, char *pfx2)
{
   FILE *fp; int n, len1, len2;
   long fwa, lwa1, pgoff;
   unsigned long inode;
   int major, minor;
   char f1, f2, f3, f4, path[512], bfr[512];
   sprintf(&bfr[0],"/proc/%d/maps",(int)pid);
   if( (fp=fopen(&bfr[0],"r")) == NULL ) {
      perror(&bfr[0]);  exit(1);
   }
   len1 = pfx1 ? strlen(pfx1) : 0;
   len2 = pfx2 ? strlen(pfx2) : 0;
   for(;;) {
      if( fgets(&bfr[0],sizeof(bfr),fp) == NULL ) {
         fprintf(stderr,"cant find process %d '%s' maps\n",(int)pid,pfx1);
         exit(1);
      }
      n = sscanf(&bfr[0],"%lx-%lx %c%c%c%c %lx %x:%x %lu %s",
            &fwa,&lwa1,&f1,&f2,&f3,&f4,&pgoff,&major,&minor,&inode,&path[0]); 
      if( n != 11 ) continue;
      // if( f3 != 'x' ) continue;
      if( !pfx1 && !pfx2 ) break;
      if( pfx1 && strncmp(pfx1,&path[0],len1) == 0 ) break;
      if( pfx2 && strncmp(pfx2,&path[0],len2) == 0 ) break;
   }
   fclose(fp);
   return fwa;
}
#ifdef __i386__
#define CHILD(v) (peek_stk(pid,(long)(v)))
#define ACHILD(v) CHILD(v)
#endif
#ifdef __x86_64__
#define CHILD(v) (peek_stk(pid,(long)(v)&0x7fffffffffffL))
#define ACHILD(v) (CHILD(v)&0x7fffffffffffL)
#endif
#define VCHILD(v) ((void*)CHILD(v))

static void
set_parm(pid_t pid,tSymTbl *tprof,long ofs,char *nm,long val)
{
   tSym *var;
   dprintf(stderr,"set %s = %#lx\n",nm,val);
   findSym(tprof,nm,&var);
   var->adr += ofs;
   poke_stk(pid,var->adr,val);
}

static void
usage(char *av0)
{
   fprintf(stderr,"usage: %s [-o path] [-d] [-e] [-p libpath] [-012] [-u #] cmd args...\n",av0);
   fprintf(stderr,"  -o  profile output pathname, -=stdout, --=stderr\n");
   fprintf(stderr,"  -d  debug output enabled\n");
   fprintf(stderr,"  -e  child debug output enabled\n");
   fprintf(stderr,"  -p  specify path for libprofile.so\n");
   fprintf(stderr,"  -0  usr+sys cpu timer intervals (sigprof)\n");
   fprintf(stderr,"  -1  usr only cpu timer intervals (sigvtalrm)\n");
   fprintf(stderr,"  -2  real time timer intervals (sigalrm)\n");
   fprintf(stderr,"  -u  profile timer interval in usecs\n");
   fprintf(stderr," use: kill -USR1 <pid> to report, rescan and restart\n");
}

long run_to_breakpoint(pid_t ppid, int init_syscall)
{
   int in_syscall = init_syscall;
   if( in_syscall < 0 )
     ptrace(PTRACE_SYSCALL,ppid,0,0);
   for(;;) {
      int status;
      pid_t pid = wait(&status);    /* wait for ptrace status */
      if( ppid != pid ) continue;
      if( WIFSTOPPED(status) ) {
         int stopsig = WSTOPSIG(status);
         if( stopsig == SIGTRAP ) {
            long eip = get_eip(pid);
            int sysnum = get_sysreq(pid);
            if( in_syscall < 0 ) {  /* starting syscall */
               if( sysnum < 0 ) {   /* hit breakpoint */
                  dprintf(stderr,"hit bp at 0x"_LX"\n",eip);
                  return eip;
               }
               dprintf(stderr,"syscall %3d "_LX" - %s",sysnum,eip,
                 sysnum >= max_sysreqs ? "unknown" : sysreq_name[sysnum]);
               in_syscall = sysnum;
            }
            else {     /* finishing syscall */
               long ret = get_sysret(pid);
               if( in_syscall != sysnum )
                  dprintf(stderr,"(%d!)",sysnum);
               dprintf(stderr," = %#lx\n",ret);
               if( init_syscall >= 0 )
                 return eip;
               in_syscall = -1;
            }
         }
         /* resume execution */
         ptrace(PTRACE_SYSCALL,pid,0,0);
      }
      else {      /* unexepected status returned */
         if( WIFEXITED(status) ) {
            int exit_code = WEXITSTATUS(status);
            fprintf(stderr,"exit %d\n",exit_code);
         }
         else if( WIFSIGNALED(status) ) {
            int signum = WTERMSIG(status);
            fprintf(stderr,"killed %d - %s\n", signum,
               signum < 0 || signum >= max_signals ?
                 "unknown" : signal_name[signum]);
         }
         else {
            fprintf(stderr,"unknown status %d\n",status);
         }
         exit(1);
      }
   }
}

static void
setup_profile(pid_t pid,long r_debug,long bp_main)
{
   tSymTbl *tprof;
   tSym *sym;
   struct link_map *so;
   long absOffset = get_map_fwa(pid, LIB "ld-", "/usr" LIB "ld-");
   struct r_debug *rdebug = (struct r_debug *)(r_debug+absOffset);
   /* look through maps - find libprofile image, save absOffset */
   dprintf(stderr,"rdebug "_LX"\n",(long)rdebug);
   for( so=VCHILD(&rdebug->r_map); so!=NULL; so=VCHILD(&so->l_next) ) {
      absOffset = CHILD(&so->l_addr);
      if( absOffset == 0 ) continue;
      if( pstrcmp(pid,ACHILD(&so->l_name),libprofile_path) == 0 ) break;
   }
   if( so == NULL ) {
      fprintf(stderr,"%s did not preload\n",libprofile_path);
      exit(1);
   }
   dprintf(stderr,"libprofile "_LX"\n",absOffset);
   /* map libprofile with nm */
   tprof = mapFile(libprofile_path,"bd");
   prof_stkptr = get_esp(pid);
   dprintf(stderr,"orig esp "_LX"\n",prof_stkptr);
   set_parm(pid,tprof,absOffset,"prof_stkptr",prof_stkptr);
   set_parm(pid,tprof,absOffset,"prof_debug",prof_debug);
   set_parm(pid,tprof,absOffset,"prof_type",prof_type);
   set_parm(pid,tprof,absOffset,"prof_tmrval",prof_tmrval);
   findSym(tprof,"prof_output",&sym);
   sym->adr += absOffset;
   pstrcpy(pid,sym->adr,&prof_output[0]);
   findSym(tprof,"profileMain",&sym);
   sym->adr += absOffset;
   dprintf(stderr,"set eip = %#lx\n",sym->adr);
   /* resume execution in profileStart */
   set_eip(pid,sym->adr);
}

int main(int ac, char **av)
{
   char *cp;
   tSymTbl *tpath, *tld_linux;
   tSym *sym_main, *sym__r_debug;
   int pid, status, execve;
   breakpoint *main_bp = NULL;
   char *lib_path = NULL, *lib_argv = NULL;


   /* decode cmdline params */
   while( ac > 1 && *(cp=av[1]) == '-' ) {
      switch( cp[1] ) {
      case 'd':  /* enable debug output */
         debug = 1;
         break;
      case 'e':  /* enable child debug output */
         prof_debug = 1;
         break;
      case '0':  /* use sigprof */
         prof_type = 0;
         break;
      case '1':  /* use sigvtalrm */
         prof_type = 1;
         break;
      case '2':  /* use sigalrm */
         prof_type = 2;
         break;
      case 'u':  /* use timer interval usec */
         prof_tmrval = strtoul(av[2],NULL,0);
         --ac;  ++av;
         break;
      case 'o':  /* specify output path */
         prof_output = av[2];
         --ac;  ++av;
         break;
      case 'p':  /* specify libprofile */
         lib_argv = av[2];
         --ac;  ++av;
         break;
      default:
         fprintf(stderr,"unknown parameter - '%s'\n",cp);
      case 'h':  /* help */
         ac = 0;
         break;
      }
      --ac;  ++av;
   }

   if( ac < 2 ) {
      usage(av[0]);
      exit(1);
   }

   if( lib_argv != NULL ) {
      if( access(&lib_argv[0],R_OK) == 0 )
         lib_path = strdup(lib_argv);
   }
   if( lib_path == NULL ) {
      cp = "./" LIBPROFILE_NAME;
      if( access(cp,R_OK) == 0 )
         lib_path = strdup(cp);
   }
   if( lib_path == NULL && (cp=getenv("HOME")) != NULL ) {
      char libpath[PATH_MAX];
      snprintf(&libpath[0],sizeof(libpath),"%s/bin/%s",
               cp,LIBPROFILE_NAME);
      if( access(&libpath[0],R_OK) == 0 )
         lib_path = strdup(&libpath[0]);
   }
   if( lib_path == NULL ) {
      if( access(cp=LIBPROFILE_PATH,R_OK) == 0 )
         lib_path = strdup(cp);
   }
   if( lib_path == NULL ) {
      fprintf(stderr,"cant find %s\n",LIBPROFILE_NAME);
      exit(1);
   }
   libprofile_path = lib_path;

   if( prof_output == NULL )
      prof_output = PROF_OUTPUT;

   /* read nm output for executable */
   tpath = mapFile(av[1],0);
   if( tpath == NULL ) {
      fprintf(stderr,"cant map %s\n",av[1]);
      exit(1);
   }
   /* read nm output for ld-linux */
   tld_linux = mapFile(LD_LINUX_SO,"BD");
   if( tld_linux == NULL ) {
      fprintf(stderr,"cant map %s\n",LD_LINUX_SO);
      exit(1);
   }

   /* lookup main, _r_debug */
   findSym(tpath,"main",&sym_main);
   findSym(tld_linux,"_r_debug",&sym__r_debug);
   /* fork child with executable */
   pid = execute_program(av[1],&av[1]);
   if( pid < 0 ) {
      perror(av[1]);
      exit(1);
   }

   printf("prof pid = %d\n", pid);
   execve = sizeof(sysreq_name)/sizeof(sysreq_name[0]);
   while( --execve >= 0 && strcmp("execve",sysreq_name[execve]) );
   if( execve < 0 ) {
      fprintf(stderr,"cant find execve sysreq\n");
      exit(1);
   }
   main_bp = newBrkpt(pid,sym_main->adr);
   run_to_breakpoint(pid, execve);
   /* add offset of main module */
   main_bp->addr += get_map_fwa(pid,0,0);
   enable_breakpoint(main_bp);
   run_to_breakpoint(pid, -1);
   /* hit breakpoint at 'main', setup profiler */
   setup_profile(pid,sym__r_debug->adr,main_bp->addr);
   disable_breakpoint(main_bp);
   ptrace(PTRACE_DETACH,pid,0,0);  /* turn off ptrace */
   pid = wait(&status);            /* wait for child status */
   if( WIFEXITED(status) ) {
      status = WEXITSTATUS(status);
      dprintf(stderr,"exit %d\n",status);
   }
   else if( WIFSIGNALED(status) ) {
      int signum = WTERMSIG(status);
      fprintf(stderr,"killed %d - %s\n", signum,
         signum < 0 || signum >= max_signals ?
            "unknown" : signal_name[signum]);
   }
   else {
      fprintf(stderr,"unknown status %d\n",status);
      status = 1;
   }
   return status;
}

