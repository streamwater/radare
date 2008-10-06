/* @LICENSE_START@ */
/*
 * Copyright (C) 2008 Alfredo Pesoli <revenge[AT]0xcafebabe.it>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its
 *    contributors may be used to endorse or promote products derived from 
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* @LICENSE_END@ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <mach-o/arch.h>
#include <mach-o/fat.h>
#include <mach-o/swap.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <mach/machine.h>
#include <mach/i386/thread_state.h>
#include <mach/machine/thread_status.h>

#include "dietmach0_errors.h"
#include "dietmach0.h"

void *offset, *toff, *init_offset;
unsigned int ncmds;


void
dm_read_codesign (int i)
{  
  char *stmt;
  int   rc;

  ld = dm_allocate(sizeof(struct linkedit_data_command));
  memcpy(ld, (char *)offset, 
         sizeof(struct linkedit_data_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_CODE_SIGNATURE\n");
  printf("cmd size \t\t: %d\n",       ld->cmdsize);
  printf("dataoff \t\t: %u\n",       ld->dataoff);
  printf("datasize \t\t: %u\n",       ld->datasize);
}

void
dm_read_uuid (int i)
{  
  char *stmt;
  int   rc;

  uuid = dm_allocate(sizeof(struct uuid_command));
  memcpy(uuid, (char *)offset, 
         sizeof(struct uuid_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_UUID\n");
  printf("cmd size \t\t: %d\n",       uuid->cmdsize);

  printf("uuid 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
         (unsigned int)uuid->uuid[0], (unsigned int)uuid->uuid[1],
         (unsigned int)uuid->uuid[2],  (unsigned int)uuid->uuid[3],
         (unsigned int)uuid->uuid[4],  (unsigned int)uuid->uuid[5],
         (unsigned int)uuid->uuid[6],  (unsigned int)uuid->uuid[7]);
  printf("     0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
         (unsigned int)uuid->uuid[8],  (unsigned int)uuid->uuid[9],
         (unsigned int)uuid->uuid[10], (unsigned int)uuid->uuid[11],
         (unsigned int)uuid->uuid[12], (unsigned int)uuid->uuid[13],
         (unsigned int)uuid->uuid[14], (unsigned int)uuid->uuid[15]);
}

void
dm_read_routines (int i)
{  
  char *stmt;
  int   rc;

  routines_command = dm_allocate(sizeof(struct routines_command));
  memcpy(routines_command, (char *)offset, 
         sizeof(struct routines_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_ROUTINES\n");
  printf("cmd size \t\t: %d\n",       routines_command->cmdsize);
  printf("init_address \t\t\t %d:\n", routines_command->init_address);
  printf("init_module \t\t\t: %d\n",  routines_command->init_module);
  printf("reserved1 \t\t\t: %d\n",    routines_command->reserved1);
  printf("reserved2 \t\t\t: %d\n",    routines_command->reserved2);
  printf("reserved3 \t\t\t: %d\n",    routines_command->reserved3);
  printf("reserved4 \t\t\t: %d\n",    routines_command->reserved4);
  printf("reserved5 \t\t\t: %d\n",    routines_command->reserved5);
  printf("reserved6 \t\t\t: %d\n",    routines_command->reserved6);	    
}

void
dm_read_twolevel_hints (int i)
{
  char *stmt;
  int   rc;

  tlh_command = dm_allocate(sizeof(struct twolevel_hints_command));
  memcpy(tlh_command, (char *)offset,
         sizeof(struct twolevel_hints_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_TWOLEVEL_HINTS\n");
  printf("cmd size \t\t: %d\n",       tlh_command->cmdsize);
  printf("offset \t\t\t: %d\n",       tlh_command->offset);
  printf("nhints \t\t\t: %d\n",       tlh_command->nhints);
}

void
dm_read_thread (int i)
{
  char *p;
  char *stmt;
  int   rc;
  unsigned long flavor;
  unsigned long count;

  thr_command = dm_allocate(sizeof(struct thread_command));
  memcpy(thr_command, (char *)offset, sizeof(struct thread_command));

  printf("\n [-] Load command %d\n", i);

  if (thr_command->cmd == LC_UNIXTHREAD)
    printf("\ncmd \t\t\t: LC_UNIXTHREAD\n");
  else
    printf("\ncmd \t\t\t: LC_THREAD\n");

  printf("cmd size \t\t: %d\n",    	thr_command->cmdsize);

  if (m_header.cputype == CPU_TYPE_I386)
    {
      i386_thread_state_t         state;

      /*
       * Pointing to the begin of flavor field
       */
      p = (char *)offset + sizeof(struct thread_command);

      memcpy(&flavor, p, sizeof(unsigned long));
      printf("flavor \t\t\t: %lu\n", flavor);

      /*
       * Pointing to the count field
       */
      p += sizeof(unsigned long);

      memcpy(&count, p, sizeof(unsigned long));
      printf("count \t\t\t: %lu\n", count);

      /*
       * Moving on to the beginning of the 
       * i386_thread_state structure
       */
      p += sizeof(unsigned long);

      memset(&state, 0x0, sizeof(i386_thread_state_t));
      memcpy(&state, (char *)p, sizeof(i386_thread_state_t));

      printf(
             "State \t\t\t: eax 0x%08x ebx    0x%08x ecx 0x%08x edx 0x%08x\n"
             "State \t\t\t: edi 0x%08x esi    0x%08x ebp 0x%08x esp 0x%08x\n"
             "State \t\t\t: ss  0x%08x eflags 0x%08x eip 0x%08x cs  0x%08x\n"
             "State \t\t\t: ds  0x%08x es     0x%08x fs  0x%08x gs  0x%08x\n",
             state.eax, state.ebx, state.ecx, state.edx, state.edi, state.esi,
             state.ebp, state.esp, state.ss, state.eflags, state.eip, state.cs,
             state.ds, state.es, state.fs, state.gs);
    }
  else
    dm_fatal(ECPU);
}
    
void
dm_read_prebound_dylib_command (int i)
{
  char *pname, *plinked_modules;
  unsigned long j;

  prebound_dy_command = dm_allocate(sizeof(struct prebound_dylib_command));
  memcpy(prebound_dy_command, (char *)offset,
         sizeof(struct prebound_dylib_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_PREBOUND_DYLIB\n");
  printf("cmd size \t\t: %d\n",    	prebound_dy_command->cmdsize);

  pname = (char *)offset + prebound_dy_command->name.offset;
  printf("name %s (offset %u)\n",     pname,
         prebound_dy_command->name.offset);

  printf("nmodules \t\t\t: %d\n",  prebound_dy_command->nmodules);

  plinked_modules = (char *)offset + 
    prebound_dy_command->linked_modules.offset;

  for (j = 0; j < prebound_dy_command->nmodules && j < 8; j++)
    {
      if ( ((plinked_modules[j/8] >> (j%8)) & 1) )
        printf("%lu\n", j); 
    }
}

void
dm_read_sub_client (int i)
{
  char *p;
  char *stmt;
  int   rc;

  sub_client = dm_allocate(sizeof(struct sub_client_command));
  memcpy(sub_client, (char *)offset, sizeof(struct sub_client_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_SUB_CLIENT\n");
  printf("cmd size \t\t: %d\n",    	sub_client->cmdsize);

  p = (char *)offset + sub_client->client.offset;
  printf("sub_client %s (offset %u)\n", p, sub_client->client.offset);
}

void
dm_read_sub_library (int i)
{
  char* p;
  char *stmt;
  int   rc;

  sub_library = dm_allocate(sizeof(struct sub_library_command));
  memcpy(sub_library, (char *)offset, sizeof(struct sub_library_command));

  printf("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_SUB_LIBRARY\n");
  printf("cmd size \t\t: %d\n",    	sub_library->cmdsize);

  p = (char *)offset + sub_library->sub_library.offset;
  printf("sub_library %s (offset %u)\n", p, sub_library->sub_library.offset);
}

void
dm_read_sub_umbrella (int i)
{
  char *p;
  char *stmt;
  int   rc;

  sub_umbrella = dm_allocate(sizeof(struct sub_umbrella_command));
  memcpy(sub_umbrella, (char *)offset, sizeof(struct sub_umbrella_command));

  printf ("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_SUB_UMBRELLA\n");
  printf("cmd size \t\t: %d\n",    	sub_umbrella->cmdsize);

  p = (char *)offset + sub_umbrella->sub_umbrella.offset;
  printf("sub_umbrella %s (offset %u)\n", p, sub_umbrella->sub_umbrella.offset);
}

void
dm_read_sub_framework (int i)
{
  char *p;
  char *stmt;
  int   rc;

  sub_framework = dm_allocate(sizeof(struct sub_framework_command));
  memcpy(sub_framework, (char *)offset, sizeof(struct sub_framework_command));

  printf ("\n [-] Load command %d\n", i);

  printf("\ncmd \t\t\t: LC_SUB_FRAMEWORK\n");
  printf("cmd size \t\t: %d\n",    	sub_framework->cmdsize);

  p = (char *)offset + sub_framework->umbrella.offset;
  printf("umbrella %s (offset %u)\n", p, sub_framework->umbrella.offset);
}

void
dm_read_dylib (int i)
{
  char *p;
  char *stmt;
  int   rc;

  dyl_command = dm_allocate(sizeof(struct dylib_command));
  memcpy(dyl_command, (char *)offset, sizeof(struct dylib_command));

  printf ("\n [-] Load command %d\n", i);

  if ( dyl_command->cmd == LC_ID_DYLIB )
    printf("\ncmd \t\t\t: LC_ID_DYLIB\n");
  else if ( dyl_command->cmd == LC_LOAD_DYLIB )
    printf("\ncmd \t\t\t: LC_LOAD_DYLIB\n");
  else
    printf("\ncmd \t\t\t: LC_LOAD_WEAK_DYLIB\n");

  printf("cmd size \t\t: %d\n",               dyl_command->cmdsize);

  p = (char *)offset + dyl_command->dylib.name.offset;
  printf("name \t\t\t: %s (offset %u)\n", p,  dyl_command->dylib.name.offset);

  printf("time stamp \t\t: %u ",              dyl_command->dylib.timestamp);
  printf("%s", ctime((const long *)&(         dyl_command->dylib.timestamp)));

  printf("current version \t: %u.%u.%u\n",
         dyl_command->dylib.current_version >> 16,
         (dyl_command->dylib.current_version >> 8) & 0xff,
         dyl_command->dylib.current_version & 0xff);

  printf("compatibility version \t: %u.%u.%u\n",
         dyl_command->dylib.compatibility_version >> 16,
         (dyl_command->dylib.compatibility_version >> 8) & 0xff,
         dyl_command->dylib.compatibility_version & 0xff);
}

void
dm_read_fvmlib (int i)
{
  char *p;
  char *stmt;
  int	  rc;

  fvm_command = dm_allocate(sizeof(struct fvmlib_command));
  memcpy(fvm_command, (char *)offset, sizeof(struct fvmlib_command));

  printf ("\n [-] Load command %d\n", i);

  if ( fvm_command->cmd == LC_IDFVMLIB )
    printf ("\ncmd \t\t: LC_IDFVMLIB\n");
  else
    printf ("\ncmd \t\t: LC_LOADFVMLIB\n");

  printf ("cmd size \t: %d\n",            fvm_command->cmdsize);

  p = (char *)offset + fvm_command->fvmlib.name.offset;
  printf("Path %s (offset %u)\n", 	p,  fvm_command->fvmlib.name.offset);
  printf("minor version %u\n",            fvm_command->fvmlib.minor_version);
  printf("header addr 0x%08x\n",          fvm_command->fvmlib.header_addr);
}

void
dm_read_symseg (int i)
{
  int rc;
  char *stmt;

  symseg_command = dm_allocate(sizeof(struct symseg_command));
  memcpy(symseg_command, (char *)offset, sizeof(struct symseg_command));

  printf ("\n [-] Load command %d\n", i);
  printf ("\ncmd \t\t: LC_SYMSEG\n");
  printf ("cmd size \t: %d\n",    symseg_command->cmdsize);
  printf ("symoff \t\t: %d\n",    symseg_command->offset);
  printf ("size \t\t: %d\n",      symseg_command->size);
}

void
dm_read_dysymtab (int i)
{
  int rc;
  char *stmt;

  dysym_command = dm_allocate(sizeof(struct dysymtab_command));
  memcpy(dysym_command, (char *)offset, sizeof(struct dysymtab_command));

  printf ("\n [-] Load command %d\n", i);
  printf ("\nSection Name \t: LC_DYSYMTAB\n");
  printf ("cmd size \t: %d\n",        dysym_command->cmdsize);
  printf ("ilocalsym \t: %d\n",       dysym_command->ilocalsym);
  printf ("nlocalsym \t: %d\n",       dysym_command->nlocalsym);
  printf ("iextdefsym \t: %d\n",      dysym_command->iextdefsym);
  printf ("nextdefsym \t: %d\n",      dysym_command->nextdefsym);
  printf ("iundefsym \t: %d\n",       dysym_command->iundefsym);
  printf ("nundefsym \t: %d\n",       dysym_command->nundefsym);
  printf ("tocoff \t\t: %d\n",        dysym_command->tocoff);
  printf ("ntoc \t\t: %d\n",          dysym_command->ntoc);
  printf ("modtaboff \t: %d\n",       dysym_command->modtaboff);
  printf ("nmodtab \t: %d\n",         dysym_command->nmodtab);
  printf ("extrefsymoff \t: %d\n",    dysym_command->extrefsymoff);
  printf ("indirectsymoff \t: %d\n",  dysym_command->indirectsymoff);
  printf ("nindirectsyms \t: %d\n",   dysym_command->nindirectsyms);
  printf ("extreloff \t: %d\n",       dysym_command->extreloff);
  printf ("nextrel \t: %d\n",         dysym_command->nextrel);
  printf ("locreloff \t: %d\n",       dysym_command->locreloff);
  printf ("nlocrel \t: %d\n",         dysym_command->nlocrel);
}

void
dm_read_symtab (int i)
{
  int rc;
  char *stmt;

  sy_command = dm_allocate(sizeof(struct symtab_command));
  memcpy(sy_command, (char *)offset, sizeof(struct symtab_command));

  printf("\n [-] Load command %d\n", i);
  printf("\ncmd \t\t: LC_SYMTAB\n");
  printf("cmd size \t: %d\n",    sy_command->cmdsize);
  printf("symoff \t\t: %d\n",    sy_command->symoff);
  printf("nsyms \t\t: %d\n",     sy_command->nsyms);
  printf("stroff \t\t: %d\n",    sy_command->stroff);
  printf("strsize \t: %d\n",     sy_command->strsize);
}

void
dm_read_section ()
{
  char *stmt;
  int rc;

  sect = dm_allocate(sizeof(struct section));
  memcpy(sect, (char *)toff, sizeof(struct section));

  printf("\n [-] Section \n");
  printf("\nSection Name \t: %s\n", sect->sectname);
  printf("Segment Name \t: %s\n", 	sect->segname);
  printf("Address \t: 0x%08x\n", 	  sect->addr);
  printf("Size \t\t: 0x%08x\n",     sect->size);
  printf("Offset \t\t: %d\n",       sect->offset);
  printf("Align \t\t: %d\n",        sect->align);
  printf("Reloff \t\t: %d\n",       sect->reloff);
  printf("Nreloc \t\t: %d\n",       sect->nreloc);
  printf("Flags \t\t: 0x%08x\n", 	  sect->flags);
  printf("Reserved1 \t: %d\n",      sect->reserved1);
  printf("Reserved2 \t: %d\n",      sect->reserved2);
}

void
dm_read_dylinker (int i)
{
  char *p;
  char *stmt;
  int  rc;

  dy_command = dm_allocate(sizeof(struct dylinker_command));
  memcpy(dy_command, offset, sizeof(struct dylinker_command));

  printf("\n [-] Load command %d\n", i);

  if (dy_command->cmd == LC_ID_DYLINKER)
    printf("\ncmd \t\t: LC_ID_DYLINKER\n");
  else
    printf("\ncmd \t\t: LC_LOAD_DYLINKER\n");

  printf("cmd size \t: %d\n", 	dy_command->cmdsize);

  p = (char *)offset + dy_command->name.offset;
  printf("Path %s (offset %u)\n", 	p, dy_command->name.offset);
}

void
dm_read_header (int p)
{
  void          *archp;
  int           i386_found=0, i, rc;
  unsigned int  nfat;
  char	        *stmt;

  memset(&fat_header, 0x0,        sizeof(struct fat_header));
  memcpy(&fat_header, fileaddr,   sizeof(struct fat_header));

  if (fat_header.magic == FAT_CIGAM)
    {
      swap_fat_header(&fat_header, LITTLE_ENDIAN);
      swapped = 1;

      printf ("\n\n [-] fat Header\n\n");
      printf ("Magic Number \t: 0x%08x\n", fat_header.magic);
      printf ("fat archs \t: %d\n", fat_header.nfat_arch);
    }

  nfat = fat_header.nfat_arch;
  archp = fileaddr + sizeof(struct fat_header);

  if ( fat_header.magic == FAT_MAGIC )
    {
      archs = dm_allocate(sizeof(struct fat_arch) * nfat);

      memcpy(archs, archp, sizeof(struct fat_arch) * nfat);
      swap_fat_arch(archs, nfat, LITTLE_ENDIAN);

      for(i = 0; i < nfat; i++)
        {
          printf("\n [-] Architecture %d\n\n", i);

          if (archs[i].cputype == CPU_TYPE_X86)
            {
              fileaddr += archs[i].offset;
              i386_found++;
              printf("cputype \t: CPU_TYPE_X86\n");
              printf("offset \t\t: %u\n", (unsigned int)archs[i].offset);
            }
          else
            {
              printf("cputype \t: %d\n", archs[i].cputype);
              printf("offset \t\t: %u\n", (unsigned int)archs[i].offset);
            }
        }
    }

  if ( !swapped || i386_found )
    {
      memset(&m_header, '\0',     sizeof(struct mach_header));
      memcpy(&m_header, fileaddr, sizeof(struct mach_header));

      if ( p )
        {
          printf ("\n\n [-] mach-o Header\n");
          printf ("\nMagic Number \t: 0x%08x\n", 	m_header.magic);
          printf ("CPU Type \t: %d\n",		        m_header.cputype);
          printf ("CPU Sub Type \t: %d\n",		    m_header.cpusubtype);
          printf ("File Type \t: %d\n",		        m_header.filetype);
          printf ("Num Of Cmds \t: %d\n",		      m_header.ncmds);
          printf ("Size Of Cmds \t: %d\n",		    m_header.sizeofcmds);
          printf ("Flags \t\t: 0x%08x\n\n",		    m_header.flags);
        }
    }
}

void
dm_read_segment (int i)
{
  char *stmt;
  int rc;

  printf ("\n [-] Load command %d\n", i);
  printf ("\ncmd \t\t: LC_SEGMENT\n");
  printf ("cmd size \t: %d\n",        seg_command->cmdsize);
  printf ("seg name \t: %s\n",        seg_command->segname);
  printf ("VM Addr \t: 0x%08x\n",     seg_command->vmaddr);
  printf ("VM Size \t: 0x%08x\n",     seg_command->vmsize);
  printf ("Fileoff \t: %d\n",         seg_command->fileoff);
  printf ("Filesize \t: %d\n",        seg_command->filesize);
  printf ("Max Prot \t: 0x%08x\n",    seg_command->maxprot);
  printf ("Init Prot \t: 0x%08x\n",   seg_command->initprot);
  printf ("NSects \t\t: %d\n",        seg_command->nsects);
  printf ("Flags \t\t: 0x%08x\n",     seg_command->flags);    
}

void
dm_read_command (int ctr)
{
  int i, z, nsects;

  if ( ctr )
    dm_read_header(1);
  else
    dm_read_header(0);

  ncmds = m_header.ncmds;

  // Offset to the first load command
  init_offset = (void *)fileaddr+sizeof(struct mach_header);

  // Offset to walk through the load_command structures
  offset  = init_offset;

  // Offset to walk through the section structures
  toff    = offset;

  for ( i=0; i < ncmds; i++ )
    {
      /*
       * l_command = dm_allocate(sizeof(struct load_command));
       * memcpy((struct load_command *)l_command, (struct load_command *)offset, 
       * sizeof(struct load_command));
       */
      seg_command = dm_allocate(sizeof(struct segment_command));
      memcpy((struct segment_command *)seg_command,
             (struct segment_command *)offset,
             sizeof(struct segment_command));

      switch (seg_command->cmd)
        {
        case LC_SEGMENT:
          nsects = seg_command->nsects;

          dm_read_segment(i);
          toff = offset+sizeof(struct segment_command);

          for ( z=0; z < nsects; z++ )
            {
              dm_read_section();
              toff += sizeof(struct section);
            }
          break;
        case LC_SYMTAB:
          dm_read_symtab(i);
          break;
        case LC_DYSYMTAB:
          dm_read_dysymtab(i);
          break;
        case LC_IDFVMLIB:
        case LC_LOADFVMLIB:
          dm_read_fvmlib(i);
          break;
        case LC_ID_DYLIB:
        case LC_LOAD_DYLIB:
        case LC_LOAD_WEAK_DYLIB:
          dm_read_dylib(i);
          break;
        case LC_SUB_FRAMEWORK:
          dm_read_sub_framework(i);
          break;
        case LC_SUB_UMBRELLA:
          dm_read_sub_umbrella(i);
          break;
        case LC_SUB_LIBRARY:
          dm_read_sub_library(i);
          break;
        case LC_SUB_CLIENT:
          dm_read_sub_client(i);
          break;
        case LC_PREBOUND_DYLIB:
          dm_read_prebound_dylib_command(i);
          break;
        case LC_ID_DYLINKER:
        case LC_LOAD_DYLINKER:
          dm_read_dylinker(i);
          break;
        case LC_THREAD:
        case LC_UNIXTHREAD:
          dm_read_thread(i);
          break;
        case LC_TWOLEVEL_HINTS:
          dm_read_twolevel_hints(i);
          break;
        case LC_ROUTINES:
          dm_read_routines(i);
          break;
        case LC_UUID:
          dm_read_uuid(i);
          break;
        case LC_CODE_SIGNATURE:
          dm_read_codesign(i);
          break;
        default:
          break;
        }        
      offset += seg_command->cmdsize;
    }
}