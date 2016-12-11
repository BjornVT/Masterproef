#ifndef __DEBUG_H_
#define __DEBUG_H_

#define DEBUG 0

#define bugline cout << __FILE__ << " " << __LINE__ << " " << __func__ << endl;

#if defined(DEBUG) && DEBUG > 0
 #define bugprintf(fmt, args...) fprintf(stdout, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define bugprintf(fmt, args...) /* Don't do anything in release builds */
#endif


#endif
