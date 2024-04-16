/* IIO - useful set of util functionality
 *
 * Copyright (c) 2008 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#include "deftypes.h"
#include "sys_utils.h"

INT FindGpioDevByName(const CHAR *pcDevDir, const CHAR *pcName)
{
    const struct dirent *ent;
    DIR *dp;

    dp = opendir(pcDevDir);
    if (!dp)
    {
        log_err("No %s dev\n", pcDevDir);
        return -ENODEV;
    }

    while (ent = readdir(dp), ent)
    {
        if ((strlen(ent->d_name) == strlen(pcName)) &&
            (strncmp(ent->d_name, pcName, strlen(pcName)) == 0))
        {
            if (closedir(dp) == -1)
            {
                log_err("closedir fail\n");
                return -errno;
            }   

            return 1;           
        }
    }
    
    closedir(dp);   
    log_err("No %s dev\n", pcName);
    
    return -ENODEV;
}


INT FindDevByName(const CHAR *pcDevDir)
{
    DIR *dp;

    dp = opendir(pcDevDir);
    if (!dp)
    {
        log_err("No input dev\n");
        return -ENODEV;
    }
    
    closedir(dp);
    return 0;
}


INT WriteSysfsInt(const CHAR *pcNodePath, INT iVal)
{
    assert(NULL != pcNodePath);
    int ret = 0;
    FILE *sysfsfp;
    
    sysfsfp = fopen(pcNodePath, "w");
    if (!sysfsfp)
    {
        log_err("failed to open %s\n", pcNodePath);
        return -errno;
    }

    ret = fprintf(sysfsfp, "%d", iVal);
    if (ret < 0)
    {
        fclose(sysfsfp);
        return -errno;
    }

    fclose(sysfsfp);
    return ret;
}

INT ReadSysfsInt(const CHAR *pcNodePath, INT *piVal)
{
    assert(NULL != pcNodePath);
    int ret = 0;
    FILE  *sysfsfp;

    sysfsfp = fopen(pcNodePath, "r");
    if (!sysfsfp)
    {
        log_err("fopen[%s] Fail\n", pcNodePath);
        return -errno;
    }

    if (fscanf(sysfsfp, "%d\n", piVal) != 1)
    {
        fclose(sysfsfp);
        return -errno;
    }

    fclose(sysfsfp);
    return ret;
}

