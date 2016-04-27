#ifndef EJOY_SVR_AUTH_H
#define EJOY_SVR_AUTH_H

struct ejoy_token
{
	unsigned long long timestamp;
	unsigned long long randomkey;
	char uid[64];
};

// return 0 on success, non-zero otherwise
int ejoy_decrypt_auth_token
(
	const char* token,
	unsigned int token_sz,
	const char* svr_pass,
	unsigned int svr_pass_sz,
	const char* svr_sig,
	unsigned int svr_sig_sz,
	struct ejoy_token* t
);

#endif
