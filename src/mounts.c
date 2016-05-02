/* 
 * Copyright (c) 2015-2016, Gregory M. Kurtzer. All rights reserved.
 * 
 * “Singularity” Copyright (c) 2016, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * If you have questions about your rights to use or distribute this software,
 * please contact Berkeley Lab's Innovation & Partnerships Office at
 * IPO@lbl.gov.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h> 
#include <string.h>
#include <fcntl.h>
#include <libgen.h>

#include "config.h"
#include "mounts.h"
#include "util.h"
#include "loop-control.h"


int mount_image(char * image_path, char * mount_point, int writable) {
    char * loop_device;

    if ( s_is_file(image_path) < 0 ) {
        fprintf(stderr, "ERROR: Could not access image file: %s\n", image_path);
        return(-1);
    }

    if ( s_is_dir(mount_point) < 0 ) {
        fprintf(stderr, "ERROR: Mount point is not available: %s\n", mount_point);
        return(-1);
    }

    if ( obtain_loop_dev(&loop_device) < 0 ) {
        fprintf(stderr, "FAILED: Could not obtain loop device\n");
        return(-1);
    }

    if ( associate_loop_dev(image_path, loop_device) < 0 ) {
        fprintf(stderr, "FAILED: Could not associate loop device\n");
        return(-1);
    }

    if ( writable > 0 ) {
        if ( mount(loop_device, mount_point, "ext4", MS_NOSUID, "discard") < 0 ) {
            fprintf(stderr, "ERROR: Failed to mount '%s' at '%s': %s\n", loop_device, mount_point, strerror(errno));
            return(-1);
        }
    } else {
        if ( mount(loop_device, mount_point, "ext4", MS_NOSUID|MS_RDONLY, "discard") < 0 ) {
            fprintf(stderr, "ERROR: Failed to mount '%s' at '%s': %s\n", loop_device, mount_point, strerror(errno));
            return(-1);
        }
    }

    return(0);
}


int mount_bind(char * image_path, char * source, char * dest, int writable) {
    char * image_dest;

    image_dest = (char *) malloc(strlen(dest) + strlen(image_path) + 3);
    snprintf(image_dest, strlen(dest) + strlen(image_path) + 3, "%s%s", image_path, dest);

    if ( s_is_dir(source) != 0 && s_is_file(source) != 0 ) {
        fprintf(stderr, "ERROR: Bind source path is not a file or directory: %s\n", source);
        return(1);
    }

    if ( s_is_dir(image_dest) != 0 && s_is_file(image_dest) != 0 ) {
        fprintf(stderr, "ERROR: Container bind path is not a file or directory: %s\n", dest);
        return(1);
    }

    if ( mount(source, image_dest, NULL, MS_BIND|MS_REC, NULL) < 0 ) {
        fprintf(stderr, "ERROR: Could not bind mount %s: %s\n", dest, strerror(errno));
        return(-1);
    }

    if ( writable <= 0 ) {
        if ( mount(NULL, image_dest, NULL, MS_BIND|MS_REC|MS_REMOUNT|MS_RDONLY, "remount,ro") < 0 ) {
            fprintf(stderr, "ERROR: Could not make bind mount read only %s: %s\n", dest, strerror(errno));
            return(-1);
        }
    }

    return(0);
}