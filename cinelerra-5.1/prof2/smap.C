#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>

#include<string>
using namespace std;

#include<elf.h>
#include<lzma.h>
#define HAVE_DECL_BASENAME 1
#include<demangle.h>

// install binutils-dev
// c++ -Wall -I/usr/include/libiberty -g smap.C -llzma -liberty

typedef uint64_t addr_t;

class smap_t;

class smap_t {
  static void demangle(const char *cp, char *bp, int n) {
    const char *dp = cplus_demangle(cp, DMGL_PARAMS+DMGL_ANSI);
    strncpy(bp, (dp ? dp : cp), n);
  }

  class chrbfr {
    char *cp;
  public:
    unsigned char *ptr() { return (unsigned char *)cp; }
    operator char*() { return cp; }
    chrbfr(int n) { cp = new char[n]; }
    ~chrbfr() { delete [] cp; }
  };
  Elf64_Ehdr elf;
  FILE *sfp;
  char *shdrs, *strtbl;
  addr_t adr_plt;
  int is_rela;
  size_t plt_sz;  char *plts;
  size_t rel_sz;  char *rels;
  size_t sym_sz, sym_entsize;
  size_t dyn_sz, dyn_entsize;
  char *symstr, *symtbl;
  char *dynstr, *dyntbl;
  addr_t vofs, pofs;
  int init();
  void finit();
  char *unxz(unsigned char *ibfr, size_t isz, size_t &out);
  void splt();
  int sect(int num);

  int rdtbl(char *tbl, off_t ofs, int sz) {
    if( fseek(sfp, ofs, SEEK_SET) ) return -1;
    if( fread(tbl, sz, 1, sfp) != 1 ) return -1;
    return 0;
  }
  int ldtbl(char *&tbl, off_t ofs, int sz) {
    if( !rdtbl(tbl=new char[sz], ofs, sz) ) return 0;
    delete [] tbl;  tbl = 0;  return -1;
  }
public:
  int symbols();

  smap_t(FILE *fp) {
    sfp = fp;
    shdrs = 0;   strtbl = 0;
    adr_plt = 0; is_rela = -1;
    plt_sz = 0;  plts = 0;
    rel_sz = 0;  rels = 0;
    sym_sz = 0;  sym_entsize = 0;
    dyn_sz = 0;  dyn_entsize = 0;
    symstr = 0;  symtbl = 0;
    dynstr = 0;  dyntbl = 0;
    vofs = 0;    pofs = 0;
  }
  ~smap_t() {
    delete [] shdrs;  delete [] strtbl;
    delete [] plts;   delete [] rels;
    delete [] symstr; delete [] symtbl;
    delete [] dynstr; delete [] dyntbl;
  }
};


char *smap_t::
unxz(unsigned char *ibfr, size_t isz, size_t &out)
{
  size_t in = 0, lmt = ~0, osz = 8*isz;
  unsigned char *obfr = new unsigned char[osz];
  out = 0;
  for(;;) {
    int err = lzma_stream_buffer_decode(&lmt,0,0,
      ibfr,&in,isz, obfr,&out,osz);
    if( !err ) return (char *)obfr;
    if( err != LZMA_BUF_ERROR ) break;
    size_t nsz = 2*osz;
    unsigned char *nbfr = new unsigned char[nsz];
    memcpy(nbfr, obfr, osz);
    delete [] obfr;  obfr = nbfr;
    osz = nsz;
  }
  delete [] obfr;
  return 0;
}

int smap_t::
sect(int num)
{
  Elf64_Shdr *shdr = (Elf64_Shdr *)(shdrs + num * elf.e_shentsize);
  switch( shdr->sh_type ) {
  case SHT_REL:
  case SHT_RELA: {
    if( !strtbl ) return 0;
    const char *tp = strtbl + shdr->sh_name;
    is_rela = shdr->sh_type == SHT_RELA ? 1 : 0;
    if( strcmp(tp, is_rela ? ".rela.plt" : ".rel.plt") ) return 0;
    int n = shdr->sh_entsize;
    int esz = is_rela ? sizeof(Elf64_Rela) : sizeof(Elf64_Rel);
    if( n < esz ) return -1;
    if( (rel_sz = shdr->sh_size) <= 0 ) return -1;
    return ldtbl(rels, shdr->sh_offset, rel_sz);
  }  
  case SHT_PROGBITS: {
    if( !strtbl ) return 0;
    const char *tp = strtbl + shdr->sh_name;
    if( !strcmp(tp, ".plt") ) {
      if( (plt_sz = shdr->sh_size) <= 0 ) return -1;
      adr_plt = shdr->sh_addr;
      return ldtbl(plts, shdr->sh_offset, plt_sz);
    }
    if( !strcmp(tp, ".gnu_debugdata") ) {
      size_t isz = shdr->sh_size, out = 0;
      if( !isz ) return -1;
      chrbfr ibfr(isz);
      if( rdtbl(ibfr, shdr->sh_offset, isz) ) return -1;
      char *obfr = unxz(ibfr.ptr(), isz, out);
      int ret = -1;
      if( obfr && out > 0 ) {
        FILE *fp = fmemopen(obfr, out, "r");
        smap_t dmap(fp);
        ret = dmap.symbols();
        fclose(fp);
      }
      delete [] obfr;
      return ret;
    }
    return 0;
  }
  case SHT_SYMTAB:
  case SHT_DYNSYM: {
    size_t &esz = shdr->sh_type == SHT_SYMTAB ? sym_entsize : dyn_entsize;
    esz = shdr->sh_entsize;
    if( esz < sizeof(Elf64_Sym) ) return -1;
    int strtab = shdr->sh_link;
    if( strtab >= elf.e_shnum ) return -1;
    Elf64_Shdr *strhdr = (Elf64_Shdr *)(shdrs + strtab * elf.e_shentsize);
    int ssz = strhdr->sh_size;
    if( ssz <= 0 ) return -1;
    char *&str = shdr->sh_type == SHT_SYMTAB ? symstr : dynstr;
    if( ldtbl(str, strhdr->sh_offset, ssz) ) return -1;
    size_t &syz = shdr->sh_type == SHT_SYMTAB ? sym_sz : dyn_sz;
    char *&tbl = shdr->sh_type == SHT_SYMTAB ? symtbl : dyntbl;
    if( ldtbl(tbl, shdr->sh_offset, syz=shdr->sh_size) ) return -1;
    int nsy = syz / esz, nsyms = 0;
    /**/ printf("\n[section %3d] contains [%d] symbols @ 0x%016lx\n", num, nsy, pofs);
    for( int i=0; i<nsy; ++i ) {
      Elf64_Sym *sym = (Elf64_Sym *)(tbl + esz*i);
      char snm[512];
      demangle(str + sym->st_name, snm, sizeof(snm));
      int bind = ELF64_ST_BIND(sym->st_info);
      switch( bind ) {
      case STB_LOCAL:   break;
      case STB_GLOBAL:  break;
      case STB_WEAK:    break;
      default: continue;
      }
      int type = ELF32_ST_TYPE(sym->st_info);
      switch( type ) {
      case STT_FUNC:    break;
/**/  case STT_OBJECT:  break;  // for r_debug
      default: continue;
      }
      int ndx = sym->st_shndx;
      if( ndx == SHN_UNDEF ) continue;
      if( ndx >= SHN_LORESERVE ) continue;
      addr_t adr = sym->st_value - pofs;
      /**/ int tch = (type == STT_FUNC ? "tTW": "dDW")[bind];
      /**/ printf("%016lx %c %s\n", adr, tch, snm);
      ++nsyms;
    }
    return nsyms;
  }
  default:
    break;
  }
  return 0;
}

int smap_t::
init()
{
  if( fread(&elf, sizeof(elf), 1, sfp) != 1 ||
      memcmp(elf.e_ident, ELFMAG, SELFMAG) != 0 ) return -1;
  // scan region map
  int psz = elf.e_phentsize, pct = elf.e_phnum;
  if( psz < (int) sizeof(Elf64_Phdr) || pct < 1 ) return -1;
  if( fseek(sfp, elf.e_phoff, SEEK_SET) ) return -1;
  chrbfr pbfr(psz);
  Elf64_Phdr *phdr = (Elf64_Phdr *) pbfr.ptr();
  while( --pct >= 0 ) {
    if( fread(pbfr, psz, 1, sfp) != 1 ) break;
    if( phdr->p_type == PT_LOAD && phdr->p_offset == 0 ) {
      vofs = phdr->p_vaddr;
      pofs = phdr->p_paddr;
      break;
    }
  }
  // scan sections
  int n = elf.e_shnum, esz = elf.e_shentsize;
  if( n <= 0 || esz < (int)sizeof(Elf64_Shdr) ) return -1;
  if( ldtbl(shdrs, elf.e_shoff, n*esz) ) return -1;
  // load strtab
  int sndx = elf.e_shstrndx;
  if( sndx >= n ) return -1;
  Elf64_Shdr *stbl = (Elf64_Shdr *)(shdrs + sndx * elf.e_shentsize);
  return ldtbl(strtbl, stbl->sh_offset, stbl->sh_size);
}

void smap_t::
finit()
{
  delete [] strtbl; strtbl = 0;
  delete [] shdrs;  shdrs = 0;
}

void smap_t::
splt()
{
  if( !plts || !rels ) return;
  if( !dynstr || !dyntbl ) return;
  if( is_rela < 0 ) return;
  int rel_entsize = is_rela ? sizeof(Elf64_Rela) : sizeof(Elf64_Rel);
  int plt_entsize = 0x10;
  unsigned char *bp = (unsigned char *)plts;
  for( uint64_t plt_ofs=plt_entsize ; plt_ofs<plt_sz; plt_ofs+=plt_entsize ) {
    unsigned char *cp = bp + plt_ofs;
    if( *cp++ != 0xff ) continue;  // opcode jmp rel ind
    if( *cp++ != 0x25 ) continue;
    addr_t ofs = 0;
    for( int k=0; k<32; k+=8 ) { addr_t by=*cp++;  ofs |= by<<k; }
    ofs += (cp - bp) + adr_plt;
    uint64_t info = 0;  addr_t addr = 0;  int64_t addend = 0;
    unsigned char *dp = (unsigned char *)rels;  int found = 0;
    for( unsigned rel_ofs=0; !found && rel_ofs<rel_sz; rel_ofs+=rel_entsize ) {
      Elf64_Rel *rp = (Elf64_Rel *)(dp + rel_ofs);
      Elf64_Rela *rap = (Elf64_Rela *)rp;
      addr = is_rela ? rap->r_offset : rp->r_offset;
      info = is_rela ? rap->r_info : rp->r_info;
      addend = is_rela ? rap->r_addend : 0;
      if( addr == ofs ) found = 1;
    }
    if( !found ) continue;
    int sndx = ELF64_R_SYM(info);
    uint64_t dyn_ofs = sndx * dyn_entsize;
    if( dyn_ofs >= dyn_sz ) continue;
    Elf64_Sym *sym = (Elf64_Sym *)(dyntbl + dyn_ofs);
    const char *snm = dynstr + sym->st_name;
    string pnm(snm);  pnm += "@plt";
    if( addend ) {
      char adn[64]; int op = addend>=0 ? '+' : '-';
      snprintf(adn,sizeof(adn),"%c%ld",op,addend>=0 ? addend : -addend);
      pnm += adn;
    }
    addr_t adr = adr_plt + plt_ofs - pofs;
    /**/ printf("%016lx %c %s\n", adr, 't', pnm.c_str());
  }
}

int smap_t::
symbols()
{
  if( init() >= 0 ) {
    int n = elf.e_shnum;
    for( int i=0; i<n; ++i ) sect(i);
    splt();
  }
  return 0;
}


int main(int ac, char **av)
{
  smap_t smap(stdin);
  smap.symbols();
  return 0;
}

