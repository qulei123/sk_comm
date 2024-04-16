
#ifndef _SYS_UTILS_H_
#define _SYS_UTILS_H_


INT FindGpioDevByName(const CHAR *pcDevDir, const CHAR *pcName);
INT FindDevByName(const CHAR *pcDevDir);
INT WriteSysfsInt(const CHAR *pcNodePath, INT iVal);
INT ReadSysfsInt(const CHAR *pcNodePath, INT *piVal);

#endif /* _SYS_UTILS_H_ */

