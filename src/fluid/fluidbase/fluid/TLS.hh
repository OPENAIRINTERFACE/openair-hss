/** @file Functions for secure communication using SSL */
#ifndef __SSL_IMPL_HH__
#define __SSL_IMPL_HH__

namespace fluid_base {
/** SSL implementation pointer for internal library use. */
extern void* tls_obj;

/** Initialize SSL parameters. You must call this function before
        asking any object to communicate in a secure manner.

        @param cert The controller's certificate signed by a CA
        @param privkey The controller's private key to be used with the
                                   certificate
        @param trustedcert A CA certificate that signs certificates of trusted
           switches */
void libfluid_tls_init(const char* cert, const char* privkey,
                       const char* trustedcert);

/** Free SSL data. You must call this function after you don't need secure
        communication anymore. */
void libfluid_tls_clear();
}  // namespace fluid_base

#endif