#include <config.h>

#if defined(HAVE_TLS)

#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <pthread.h>

#include "fluid/TLS.hh"

namespace fluid_base {

void* tls_obj = NULL;
pthread_mutex_t* ssl_locks;
int ssl_num_locks;

static unsigned long get_thread_id_cb(void) {
  return (unsigned long)pthread_self();
}

static void thread_lock_cb(int mode, int which, const char* f, int l) {
  if (which < ssl_num_locks) {
    if (mode & CRYPTO_LOCK) {
      pthread_mutex_lock(&(ssl_locks[which]));
    } else {
      pthread_mutex_unlock(&(ssl_locks[which]));
    }
  }
}

// TODO: make these function idempotent

void libfluid_tls_init(const char* cert, const char* privkey,
                       const char* trustedcert) {
  int i;
  SSL_CTX* server_ctx;

  tls_obj = NULL;

  ssl_num_locks = CRYPTO_num_locks();
  ssl_locks = (pthread_mutex_t*)malloc(ssl_num_locks * sizeof(pthread_mutex_t));
  if (ssl_locks == NULL) return;

  for (i = 0; i < ssl_num_locks; i++) {
    pthread_mutex_init(&(ssl_locks[i]), NULL);
  }

  // TODO: change this to the CRYPTO_THREADID family of functions to aid
  // portability. While this will work on Linux, it is deprecated and should
  // be changed.
  CRYPTO_set_id_callback(get_thread_id_cb);
  CRYPTO_set_locking_callback(thread_lock_cb);

  /* Initialize OpenSSL */
  SSL_load_error_strings();
  SSL_library_init();

  /* Stop if there's no entropy */
  if (!RAND_poll()) return;

  server_ctx = SSL_CTX_new(SSLv23_server_method());

  if (!SSL_CTX_load_verify_locations(server_ctx, trustedcert, NULL)) {
    fprintf(stderr, "Error loading verification CA certificate.\n");
    return;
  }
  SSL_CTX_set_verify(server_ctx,
                     SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

  if (!SSL_CTX_use_certificate_chain_file(server_ctx, cert)) {
    fprintf(stderr, "Error loading certificate.\n");
    return;
  }

  if (!SSL_CTX_use_PrivateKey_file(server_ctx, privkey, SSL_FILETYPE_PEM)) {
    fprintf(stderr, "Error loading private key.\n");
    return;
  }

  tls_obj = server_ctx;
}

void libfluid_tls_clear() {
  // TODO: investigate how to free the memory (~84k) that is still reachable
  // after this function runs.
  if (tls_obj != NULL) {
    SSL_CTX_free((SSL_CTX*)tls_obj);
  }
}

}  // namespace fluid_base

#endif