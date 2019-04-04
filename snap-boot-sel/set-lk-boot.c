/* Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <malloc.h>
#include "snappy-boot.h"

static uint32_t crc32(uint32_t crc, unsigned char *buf, size_t len)
{
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
    }
    return ~crc;
}


int main (int argc, char **argv) {
    SNAP_BOOT_SELECTION_t boot_select;
    SNAP_BOOT_SELECTION_t *bootSel;
    FILE *fpw = NULL;
    int argp=1,it=1,full_write=0;
    printf("usage: [-w/-r/-sbp] [output file] [snap-mode] [kernel-snap] [core-snap] [boot-1 part name][kernel-snap-rev][boot-2 part name][kernel-snap-rev][boot-2 part name][kernel-snap-rev]..\n");
    if ( !strncmp(argv[argp], "-sbp",4) || !strncmp(argv[argp], "-w",2)) {
      printf( "running in write mode\n");
      if (!strncmp(argv[argp], "-w",2)) {
          full_write=1;
      }
      fpw = fopen (argv[++argp], "w");
      if (!fpw) {  /* validate file open for reading */
          fprintf (stderr, "error: file open failed '%s'.\n", argv[--argp]);
          return 1;
      }

      // fill structure
      boot_select.signature=SNAP_BOOTSELECT_SIGNATURE;
      boot_select.version=SNAP_BOOTSELECT_VERSION;
      if (full_write) {
         printf("setting:snap_mode [%s]\n",argv[++argp]);
         if (argv[argp] != NULL ) strncpy(boot_select.snap_mode,argv[argp],SNAP_MODE_LENGTH);
         // strncpy(boot_select.snap_mode, SNAP_MODE_TRY, SNAP_MODE_LENGTH);
         printf("setting:snap_kernel [%s]\n",argv[++argp]);
         if (argv[argp] != NULL ) strncpy(boot_select.snap_kernel, argv[argp], SNAP_NAME_MAX_LEN);
         printf("setting:snap_core [%s]\n",argv[++argp]);
         if (argv[argp] != NULL ) strncpy(boot_select.snap_core, argv[argp], SNAP_NAME_MAX_LEN);
      } else {
         boot_select.snap_mode[0]=0;
         boot_select.snap_kernel[0]=0;
         boot_select.snap_core[0]=0;
      }
      boot_select.snap_try_kernel[0]=0;
      boot_select.snap_try_core[0]=0;
      boot_select.boot_part_num=0;
      if (full_write) {
          it=2;
      }
      for (++argp; argp < argc; argp+=it) {
        if (full_write) {
            printf("setting:matrix slot %d [ %s ] [ %s ]\n", boot_select.boot_part_num+1, argv[argp], argv[argp+1]);
            strncpy(boot_select.bootimg_matrix[boot_select.boot_part_num][0],argv[argp],SNAP_NAME_MAX_LEN);
            strncpy(boot_select.bootimg_matrix[boot_select.boot_part_num][1],argv[argp+1],SNAP_NAME_MAX_LEN);
        } else {
            printf("setting:matrix slot %d [ %s ] [ ]\n", boot_select.boot_part_num+1, argv[argp]);
            strncpy(boot_select.bootimg_matrix[boot_select.boot_part_num][0],argv[argp],SNAP_NAME_MAX_LEN);
            boot_select.bootimg_matrix[boot_select.boot_part_num][1][0]=0;
        }
        boot_select.boot_part_num++;
      }
      // fill rest of slots with null
      for (int i = boot_select.boot_part_num; i < SNAP_BOOTIMG_PART_NUM_MAX ; i++) {
        strncpy(boot_select.bootimg_matrix[i][0],"",SNAP_NAME_MAX_LEN);
        strncpy(boot_select.bootimg_matrix[i][1],"",SNAP_NAME_MAX_LEN);
      }
      printf("configured %d boot partitions\n", boot_select.boot_part_num);
      boot_select.crc32 = crc32(0x0, (void *)&boot_select, sizeof(SNAP_BOOT_SELECTION_t)-sizeof(uint32_t));
      printf( "Calculated crc32 is [0x%X]\n", boot_select.crc32);
      // save structure
      fwrite (&boot_select, sizeof(SNAP_BOOT_SELECTION_t), 1, fpw);

      if (fpw != stdin) fclose (fpw);   /* close file if not stdin */
      return 0;
    } else if ( !strncmp(argv[1], "-r",2)) {
      printf( "running in read mode \n");
      fpw = argc > 1 ? fopen (argv[2], "r") : stdin;
      if (!fpw) {  /* validate file open for reading */
          fprintf (stderr, "error: file open failed '%s'.\n", argv[2]);
          return 1;
      }
      bootSel = (SNAP_BOOT_SELECTION_t *)malloc(sizeof(SNAP_BOOT_SELECTION_t));
      fread(bootSel, sizeof(SNAP_BOOT_SELECTION_t), 1, fpw);
      if (fpw != stdin) fclose (fpw);   /* close file if not stdin */
      printf("signature: [0x%X]\n", bootSel->signature);
      printf("version:   [0x%X]\n", bootSel->version);
      printf("snap_mode [%s]\n", bootSel->snap_mode);
      printf("snap_kernel [%s]\n", bootSel->snap_kernel);
      printf("snap_core [%s]\n", bootSel->snap_core);
      printf("snap_try_kernel [%s]\n", bootSel->snap_try_kernel);
      printf("snap_try_core [%s]\n", bootSel->snap_try_core);
      printf("number of active boot slots [%d]\n", bootSel->boot_part_num);
      for( int i = 0; i < SNAP_BOOTIMG_PART_NUM_MAX ; i++) {
          printf("matrix: slot %d  [ %s ] [ %s ]\n", i, bootSel->bootimg_matrix[i][0], bootSel->bootimg_matrix[i][1]);
      }
      if (bootSel->crc32 != crc32(0x0, (void *)bootSel, sizeof(SNAP_BOOT_SELECTION_t)-sizeof(uint32_t))) {
          printf("BROEN crc32!!!!! [0x%X] vs [0x%X]\n", bootSel->crc32, crc32(0x0, (void *)bootSel, sizeof(SNAP_BOOT_SELECTION_t)-sizeof(uint32_t)));
      } else {
          printf("crc32 validated [0x%X]\n", bootSel->crc32);
      }
    }
}

