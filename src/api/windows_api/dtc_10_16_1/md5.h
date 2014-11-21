#ifndef _H_MD5_H
#define _H_MD5_H

#include <stdint.h>



#define MD5Init _TX_MD5Init_
#define MD5Update _TX_MD5Update_
#define MD5Final _TX_MD5Final_
#define MD5Digest _TX_MD5Digest_
#define MD5DigestHex _TX_MD5DigestHex_
#define MD5HMAC _TX_MD5HMAC_
#define MD5HMAC2 _TX_MD5HMAC2_

struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	uint8_t  in[64];
};

typedef struct MD5Context md5_t;

void MD5Init(struct MD5Context *);
void MD5Update(struct MD5Context *, unsigned char const *, unsigned);
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);
void MD5Digest( const unsigned char *msg, int len, unsigned char *digest);
void MD5DigestHex( const unsigned char *msg, int len, unsigned char *digest);
void MD5HMAC(const unsigned char *password,  unsigned pass_len,
		const unsigned char *challenge, unsigned chal_len,
		unsigned char response[16]);
void MD5HMAC2(const unsigned char *password,  unsigned pass_len,
		const unsigned char *challenge, unsigned chal_len,
		const unsigned char *challenge2, unsigned chal_len2,
		unsigned char response[16]);


#endif
