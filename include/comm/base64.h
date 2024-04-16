
#ifndef _BASE64_H_
#define _BASE64_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


size_t base64_encoded_size(size_t size);
size_t base64_decoded_size(const unsigned char *encoded_data, size_t encoded_size);
int base64_encode(const unsigned char *data, size_t data_size, unsigned char *encoded_data);
int base64_decode(const unsigned char *encoded_data, size_t encoded_size, unsigned char *data);


#ifdef __cplusplus
}
#endif

#endif  /* _BASE64_H_ */

