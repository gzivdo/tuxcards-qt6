/***************************************************************************
                          StringCrypter.cpp
                          - legacy Blowfish + MD5 (TuxCards 2.0 format)
                          - AES-256-GCM + PBKDF2 (OpenSSL backend)
                          - XChaCha20-Poly1305 + Argon2i (monocypher backend)
 ***************************************************************************/

#include "BlowFish.h"
#include "MD5.h"

#include <QString>
#include <QByteArray>
#include <QIODevice>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <random>

#ifdef TUXCARDS_BACKEND_OPENSSL
#  include <openssl/evp.h>
#  include <openssl/rand.h>
#  include <openssl/err.h>
#endif

#ifdef TUXCARDS_BACKEND_MONOCYPHER
extern "C" {
#  include "monocypher/monocypher.h"
}
#endif

#include "StringCrypter.h"

const int StringCrypter::BUFFER_SIZE = 512;

// ---------- format magic headers ----------
static const char* LEGACY_MAGIC = "Fh_enc:BF10";
static const char* AESGCM_MAGIC = "Fh_enc:AES256GCM-PBKDF2";   // 23 bytes
static constexpr int  AESGCM_VERSION       = 1;
static constexpr int  AESGCM_SALT_LEN      = 16;
static constexpr int  AESGCM_IV_LEN        = 12;
static constexpr int  AESGCM_TAG_LEN       = 16;
static constexpr int  AESGCM_KEY_LEN       = 32;
static constexpr int  AESGCM_PBKDF2_ITERS  = 200000;

static const char* XCHACHA_MAGIC = "Fh_enc:XChaCha20Poly1305-Argon2i-v1"; // 35 bytes
static constexpr int  XCHACHA_VERSION    = 1;
static constexpr int  XCHACHA_SALT_LEN   = 16;
static constexpr int  XCHACHA_NONCE_LEN  = 24;
static constexpr int  XCHACHA_MAC_LEN    = 16;
static constexpr int  XCHACHA_KEY_LEN    = 32;
// Argon2i defaults: 64 MB / 3 passes / 1 lane (memory hard).
static constexpr quint32 XCHACHA_ARGON2_NB_BLOCKS = 65536; // 64 MB
static constexpr quint16 XCHACHA_ARGON2_PASSES    = 3;
static constexpr quint8  XCHACHA_ARGON2_LANES     = 1;

static const int BFISH_BUF_MOD = 8;

// Backend identifier for the in-process key cache. Distinguishes derivations
// made with PBKDF2 (OpenSSL/AES-GCM) from those made with Argon2i (monocypher)
// — same 32-byte key length, completely different KDF.
enum class CryptoBackend : int { None = 0, AesGcm = 1, XChaCha = 2 };


// =========================================================================
//                    In-process AES-GCM key cache
// =========================================================================
//
// PBKDF2 with 200000 iterations costs ~100ms on a current CPU. A save of
// an encrypted note tree calls encryptString once per element, and a load
// calls decryptString once per element — so without a cache, the UI
// thread runs PBKDF2 dozens of times per save/load and visibly freezes.
//
// The cache stores (password, salt, derived key) for the most recent
// derivation. encryptString reuses salt+key as long as the password is
// the same; the consequence is that all elements in a single save share
// one salt — which is fine, since AES-GCM stays secure as long as IVs
// are unique (we still generate a fresh random IV per element).
// decryptString reuses the key when both the password and the salt read
// from the blob match the cache.
//
// Cached values live for the lifetime of the process unless
// clearKeyCache() is called; they are not more sensitive than the file
// password already held by CInformationCollection.

// Per-backend cache state. AES-GCM uses (salt+iters); XChaCha uses
// (salt + argon2 params). We keep one slot per backend so a single
// session that touches both formats does not thrash one slot.
namespace {
   // Common
   CryptoBackend g_cacheBackend = CryptoBackend::None;
   bool          g_cacheValid   = false;
   QByteArray    g_cachePassword;
   unsigned char g_cacheSalt[32];      // max(AESGCM_SALT_LEN, XCHACHA_SALT_LEN)
   unsigned char g_cacheKey [32];      // both backends derive 32-byte keys

   // AES-GCM extra: PBKDF2 iter count
   quint32       g_cacheAesIters  = 0;

   // XChaCha extra: Argon2i parameters
   quint32       g_cacheArgonBlocks = 0;
   quint16       g_cacheArgonPasses = 0;
   quint8        g_cacheArgonLanes  = 0;
}

int StringCrypter::identifyBlobFormat( const QByteArray& blob )
{
   if ( blob.size() <= 0 )
      return BLOB_UNENCRYPTED;

   auto startsWith = [&](const char* magic) {
      const int n = (int)std::strlen(magic);
      return blob.size() >= n && 0 == std::memcmp(blob.constData(), magic, n);
   };

   if ( startsWith(AESGCM_MAGIC) )  return BLOB_AESGCM;
   if ( startsWith(XCHACHA_MAGIC) ) return BLOB_XCHACHA;
   if ( startsWith(LEGACY_MAGIC) )  return BLOB_LEGACY_BF;
   return BLOB_UNKNOWN;
}

bool StringCrypter::isBackendAvailableFor( int blobFormat )
{
   switch ( blobFormat ) {
   case BLOB_UNENCRYPTED: return true;
   case BLOB_LEGACY_BF:   return true;  // always compiled in
   case BLOB_AESGCM:
#ifdef TUXCARDS_BACKEND_OPENSSL
      return true;
#else
      return false;
#endif
   case BLOB_XCHACHA:
#ifdef TUXCARDS_BACKEND_MONOCYPHER
      return true;
#else
      return false;
#endif
   default:
      return false;
   }
}

void StringCrypter::clearKeyCache()
{
   g_cacheBackend = CryptoBackend::None;
   g_cacheValid   = false;
   g_cachePassword.fill('\0');
   g_cachePassword.clear();
   std::memset(g_cacheSalt, 0, sizeof(g_cacheSalt));
   std::memset(g_cacheKey,  0, sizeof(g_cacheKey));
   g_cacheAesIters    = 0;
   g_cacheArgonBlocks = 0;
   g_cacheArgonPasses = 0;
   g_cacheArgonLanes  = 0;
}


// =========================================================================
//                              Public dispatcher
// =========================================================================

// Runtime write-backend selector. The application reads
// CTuxCardsConfiguration::S_ENCRYPTION_FORMAT at startup and pushes the
// choice into here via setWriteBackend(); StringCrypter itself does not
// depend on the configuration class (keeps the test build small).
namespace {
   int g_writeBackend = -1;   // -1 = uninitialised, use compile-time default
}

void StringCrypter::setWriteBackend( int blobFormat )
{
   // Only accept values we can actually serve; reject others.
   if ( blobFormat == BLOB_AESGCM || blobFormat == BLOB_XCHACHA )
      g_writeBackend = blobFormat;
}

int StringCrypter::getWriteBackend()
{
   if ( g_writeBackend > 0 )
      return g_writeBackend;
#if defined(TUXCARDS_WRITE_BACKEND_XCHACHA) && defined(TUXCARDS_BACKEND_MONOCYPHER)
   return BLOB_XCHACHA;
#elif defined(TUXCARDS_BACKEND_OPENSSL)
   return BLOB_AESGCM;
#elif defined(TUXCARDS_BACKEND_MONOCYPHER)
   return BLOB_XCHACHA;
#else
   return BLOB_LEGACY_BF;
#endif
}

void StringCrypter::encryptString( const QString& sInputString, const QString& sPassWd,
                                   QByteArray& encryptedData )
{
   const int requested = getWriteBackend();

#ifdef TUXCARDS_BACKEND_MONOCYPHER
   if ( requested == BLOB_XCHACHA ) {
      encryptStringXChaCha( sInputString, sPassWd, encryptedData );
      return;
   }
#endif
#ifdef TUXCARDS_BACKEND_OPENSSL
   if ( requested == BLOB_AESGCM ) {
      encryptStringAESGCM( sInputString, sPassWd, encryptedData );
      return;
   }
#endif

   // Requested backend not linked into this build — fall through to
   // whatever IS available, preferring modern over BF10.
#ifdef TUXCARDS_BACKEND_OPENSSL
   encryptStringAESGCM( sInputString, sPassWd, encryptedData );
#elif defined(TUXCARDS_BACKEND_MONOCYPHER)
   encryptStringXChaCha( sInputString, sPassWd, encryptedData );
#else
   encryptStringLegacyBF( sInputString, sPassWd, encryptedData );
#endif
}

int StringCrypter::decryptString( const QByteArray& encryptedData,
                                  const QString& sPassWd, QString& sOutputString )
{
   if ( encryptedData.size() <= 0 )
      return ERROR_INVALID_ENCRYPTEDDATA;

   const int aesLen     = (int)std::strlen(AESGCM_MAGIC);
   const int xchachaLen = (int)std::strlen(XCHACHA_MAGIC);
   const int legacyLen  = (int)std::strlen(LEGACY_MAGIC);

   if ( encryptedData.size() >= aesLen &&
        0 == std::memcmp(encryptedData.constData(), AESGCM_MAGIC, aesLen) )
   {
#ifdef TUXCARDS_BACKEND_OPENSSL
      return decryptStringAESGCM( encryptedData, sPassWd, sOutputString );
#else
      std::cerr << "StringCrypter: blob is AES-256-GCM but this build "
                   "was compiled without the OpenSSL backend."
                << std::endl;
      return ERROR_UNSUPPORTED_BACKEND;
#endif
   }
   if ( encryptedData.size() >= xchachaLen &&
        0 == std::memcmp(encryptedData.constData(), XCHACHA_MAGIC, xchachaLen) )
   {
#ifdef TUXCARDS_BACKEND_MONOCYPHER
      return decryptStringXChaCha( encryptedData, sPassWd, sOutputString );
#else
      std::cerr << "StringCrypter: blob is XChaCha20-Poly1305 but this "
                   "build was compiled without the monocypher backend."
                << std::endl;
      return ERROR_UNSUPPORTED_BACKEND;
#endif
   }
   if ( encryptedData.size() >= legacyLen &&
        0 == std::memcmp(encryptedData.constData(), LEGACY_MAGIC, legacyLen) )
   {
      return decryptStringLegacyBF( encryptedData, sPassWd, sOutputString );
   }

   std::cout << "StringCrypter: unknown encrypted-data magic." << std::endl;
   return ERROR_INVALID_FILEHEADER;
}


// =========================================================================
//                AES-256-GCM + PBKDF2-HMAC-SHA256 (current)
// =========================================================================
//
//  Layout of an AES-GCM blob:
//  [ MAGIC (23) | ver (1) | salt (16) | iv (12) | iters (4 LE)
//  | tag (16) | ciphertext (variable) ]
//
// Both helpers are only compiled when OpenSSL is available — see the
// public encryptString/decryptString dispatchers above for the no-
// OpenSSL fallbacks.

#ifdef TUXCARDS_BACKEND_OPENSSL

void StringCrypter::encryptStringAESGCM( const QString& sInputString,
                                         const QString& sPassWd,
                                         QByteArray& encryptedData )
{
   encryptedData.clear();

   if ( sPassWd.isEmpty() )
   {
      std::cerr << "StringCrypter::encryptStringAESGCM: empty password" << std::endl;
      return;
   }

   QByteArray passBytes = sPassWd.toUtf8();
   QByteArray plaintext = sInputString.toUtf8();

   unsigned char salt[AESGCM_SALT_LEN];
   unsigned char iv  [AESGCM_IV_LEN];
   if ( 1 != RAND_bytes(iv, AESGCM_IV_LEN) ) {
      std::cerr << "StringCrypter::encryptStringAESGCM: RAND_bytes failed" << std::endl;
      return;
   }

   unsigned char key[AESGCM_KEY_LEN];
   if ( g_cacheValid && g_cacheBackend == CryptoBackend::AesGcm &&
        passBytes == g_cachePassword &&
        g_cacheAesIters == (quint32)AESGCM_PBKDF2_ITERS ) {
      // Cache hit — reuse salt + key from the previous derivation.
      std::memcpy(salt, g_cacheSalt, AESGCM_SALT_LEN);
      std::memcpy(key,  g_cacheKey,  AESGCM_KEY_LEN);
   } else {
      // Cache miss — fresh random salt, derive key, populate cache.
      if ( 1 != RAND_bytes(salt, AESGCM_SALT_LEN) ) {
         std::cerr << "StringCrypter::encryptStringAESGCM: RAND_bytes failed" << std::endl;
         return;
      }
      if ( 1 != PKCS5_PBKDF2_HMAC( passBytes.constData(), passBytes.size(),
                                   salt, AESGCM_SALT_LEN,
                                   AESGCM_PBKDF2_ITERS,
                                   EVP_sha256(),
                                   AESGCM_KEY_LEN, key ) ) {
         std::cerr << "StringCrypter::encryptStringAESGCM: PBKDF2 failed" << std::endl;
         return;
      }
      g_cacheBackend = CryptoBackend::AesGcm;
      g_cachePassword = passBytes;
      std::memcpy(g_cacheSalt, salt, AESGCM_SALT_LEN);
      std::memcpy(g_cacheKey,  key,  AESGCM_KEY_LEN);
      g_cacheAesIters = AESGCM_PBKDF2_ITERS;
      g_cacheValid = true;
   }

   QByteArray ciphertext( plaintext.size(), Qt::Uninitialized );
   unsigned char tag[AESGCM_TAG_LEN];

   EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
   if ( !ctx ) return;

   int outLen = 0, totalLen = 0;
   bool ok = true;
   ok = ok && (1 == EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr));
   ok = ok && (1 == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AESGCM_IV_LEN, nullptr));
   ok = ok && (1 == EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv));
   if ( ok ) {
      ok = (1 == EVP_EncryptUpdate(ctx,
              (unsigned char*)ciphertext.data(), &outLen,
              (const unsigned char*)plaintext.constData(), plaintext.size()));
      totalLen = outLen;
   }
   if ( ok ) {
      ok = (1 == EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext.data() + totalLen, &outLen));
      totalLen += outLen;
   }
   if ( ok ) {
      ok = (1 == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AESGCM_TAG_LEN, tag));
   }
   EVP_CIPHER_CTX_free(ctx);

   // wipe key from stack
   std::memset(key, 0, AESGCM_KEY_LEN);

   if ( !ok ) {
      std::cerr << "StringCrypter::encryptStringAESGCM: EVP failed" << std::endl;
      encryptedData.clear();
      return;
   }

   ciphertext.resize(totalLen);

   // assemble: magic | ver | salt | iv | iters | tag | ciphertext
   encryptedData.append( AESGCM_MAGIC, (int)std::strlen(AESGCM_MAGIC) );
   encryptedData.append( (char)AESGCM_VERSION );
   encryptedData.append( (const char*)salt, AESGCM_SALT_LEN );
   encryptedData.append( (const char*)iv,   AESGCM_IV_LEN );
   const quint32 it = AESGCM_PBKDF2_ITERS;
   encryptedData.append( (char)((it >>  0) & 0xff) );
   encryptedData.append( (char)((it >>  8) & 0xff) );
   encryptedData.append( (char)((it >> 16) & 0xff) );
   encryptedData.append( (char)((it >> 24) & 0xff) );
   encryptedData.append( (const char*)tag, AESGCM_TAG_LEN );
   encryptedData.append( ciphertext );
}


int StringCrypter::decryptStringAESGCM( const QByteArray& encryptedData,
                                        const QString& sPassWd,
                                        QString& sOutputString )
{
   sOutputString.clear();

   const int magicLen   = (int)std::strlen(AESGCM_MAGIC);
   const int headerLen  = magicLen + 1 + AESGCM_SALT_LEN + AESGCM_IV_LEN + 4 + AESGCM_TAG_LEN;
   if ( encryptedData.size() < headerLen )
      return ERROR_INVALID_ENCRYPTEDDATA;

   const unsigned char* p = (const unsigned char*)encryptedData.constData();
   int offset = magicLen;
   const unsigned char ver = p[offset++];
   if ( ver != AESGCM_VERSION )
      return ERROR_INVALID_FILEHEADER;

   const unsigned char* salt = p + offset; offset += AESGCM_SALT_LEN;
   const unsigned char* iv   = p + offset; offset += AESGCM_IV_LEN;
   const quint32 iters = ((quint32)p[offset+0]) |
                         ((quint32)p[offset+1] <<  8) |
                         ((quint32)p[offset+2] << 16) |
                         ((quint32)p[offset+3] << 24);
   offset += 4;
   const unsigned char* tag  = p + offset; offset += AESGCM_TAG_LEN;
   const int ctLen = encryptedData.size() - offset;
   const unsigned char* ct = p + offset;

   QByteArray passBytes = sPassWd.toUtf8();
   unsigned char key[AESGCM_KEY_LEN];
   if ( g_cacheValid && g_cacheBackend == CryptoBackend::AesGcm &&
        passBytes == g_cachePassword &&
        g_cacheAesIters == iters &&
        0 == std::memcmp(salt, g_cacheSalt, AESGCM_SALT_LEN) ) {
      // Cache hit — key already derived for this (password, salt, iters).
      std::memcpy(key, g_cacheKey, AESGCM_KEY_LEN);
   } else {
      if ( 1 != PKCS5_PBKDF2_HMAC( passBytes.constData(), passBytes.size(),
                                   salt, AESGCM_SALT_LEN,
                                   (int)iters,
                                   EVP_sha256(),
                                   AESGCM_KEY_LEN, key ) ) {
         return ERROR_CRYPTO;
      }
      g_cacheBackend = CryptoBackend::AesGcm;
      g_cachePassword = passBytes;
      std::memcpy(g_cacheSalt, salt, AESGCM_SALT_LEN);
      std::memcpy(g_cacheKey,  key,  AESGCM_KEY_LEN);
      g_cacheAesIters = iters;
      g_cacheValid = true;
   }

   QByteArray plain( ctLen, Qt::Uninitialized );

   EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
   if ( !ctx ) {
      std::memset(key, 0, AESGCM_KEY_LEN);
      return ERROR_CRYPTO;
   }

   int outLen = 0, totalLen = 0;
   bool ok = true;
   ok = ok && (1 == EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr));
   ok = ok && (1 == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AESGCM_IV_LEN, nullptr));
   ok = ok && (1 == EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, iv));
   if ( ok ) {
      ok = (1 == EVP_DecryptUpdate(ctx,
              (unsigned char*)plain.data(), &outLen,
              ct, ctLen));
      totalLen = outLen;
   }
   if ( ok ) {
      ok = (1 == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AESGCM_TAG_LEN, (void*)tag));
   }
   int finalRc = 0;
   if ( ok ) {
      // Final returns 0 if tag verification fails — that's how GCM signals
      // a wrong password or tampered ciphertext.
      finalRc = EVP_DecryptFinal_ex(ctx, (unsigned char*)plain.data() + totalLen, &outLen);
      totalLen += outLen;
   }
   EVP_CIPHER_CTX_free(ctx);
   std::memset(key, 0, AESGCM_KEY_LEN);

   if ( !ok )
      return ERROR_CRYPTO;
   if ( finalRc != 1 )
      return ERROR_INVALID_PASSWD;

   plain.resize(totalLen);
   sOutputString = QString::fromUtf8(plain);
   return NO_ERROR;
}

#endif  // TUXCARDS_BACKEND_OPENSSL


// =========================================================================
//             XChaCha20-Poly1305 + Argon2i (monocypher backend)
// =========================================================================
//
//  Layout of an XChaCha blob:
//  [ MAGIC (35) | ver (1) | salt (16) | nonce (24)
//  | argon2 mem_kb (4 LE) | passes (2 LE) | lanes (1) | reserved (1)
//  | mac (16) | ciphertext (variable) ]
//
// argon2 mem_kb is encoded in KiB. Default 65536 (64 MB), 3 passes,
// 1 lane — strong-but-affordable defaults for an interactive tool.

#ifdef TUXCARDS_BACKEND_MONOCYPHER

static void writeLE32( QByteArray& dst, quint32 v )
{
   dst.append( (char)((v >>  0) & 0xff) );
   dst.append( (char)((v >>  8) & 0xff) );
   dst.append( (char)((v >> 16) & 0xff) );
   dst.append( (char)((v >> 24) & 0xff) );
}
static void writeLE16( QByteArray& dst, quint16 v )
{
   dst.append( (char)((v >>  0) & 0xff) );
   dst.append( (char)((v >>  8) & 0xff) );
}
static quint32 readLE32( const unsigned char* p )
{
   return ((quint32)p[0])       |
          ((quint32)p[1] <<  8) |
          ((quint32)p[2] << 16) |
          ((quint32)p[3] << 24);
}
static quint16 readLE16( const unsigned char* p )
{
   return ((quint16)p[0]) | ((quint16)p[1] << 8);
}

static bool deriveArgon2iKey( const QByteArray& passBytes,
                              const unsigned char* salt,
                              quint32 nb_blocks, quint32 nb_passes, quint32 nb_lanes,
                              unsigned char outKey[XCHACHA_KEY_LEN] )
{
   if ( nb_blocks == 0 || nb_passes == 0 || nb_lanes == 0 )
      return false;
   // monocypher requires nb_blocks >= 8 * nb_lanes.
   if ( nb_blocks < 8u * nb_lanes )
      return false;

   const size_t work_area_size = (size_t)nb_blocks * 1024u;
   void* work_area = std::malloc(work_area_size);
   if ( !work_area )
      return false;

   crypto_argon2_config cfg;
   cfg.algorithm = CRYPTO_ARGON2_I;
   cfg.nb_blocks = nb_blocks;
   cfg.nb_passes = nb_passes;
   cfg.nb_lanes  = nb_lanes;

   crypto_argon2_inputs inp;
   inp.pass      = (const uint8_t*)passBytes.constData();
   inp.pass_size = (quint32)passBytes.size();
   inp.salt      = salt;
   inp.salt_size = XCHACHA_SALT_LEN;

   crypto_argon2(outKey, XCHACHA_KEY_LEN, work_area, cfg, inp,
                 crypto_argon2_no_extras);

   crypto_wipe(work_area, work_area_size);
   std::free(work_area);
   return true;
}

void StringCrypter::encryptStringXChaCha( const QString& sInputString,
                                          const QString& sPassWd,
                                          QByteArray& encryptedData )
{
   encryptedData.clear();

   if ( sPassWd.isEmpty() )
   {
      std::cerr << "StringCrypter::encryptStringXChaCha: empty password" << std::endl;
      return;
   }

   QByteArray passBytes = sPassWd.toUtf8();
   QByteArray plaintext = sInputString.toUtf8();

   unsigned char salt [XCHACHA_SALT_LEN];
   unsigned char nonce[XCHACHA_NONCE_LEN];
   unsigned char key  [XCHACHA_KEY_LEN];

   // nonce always fresh.
#ifdef TUXCARDS_BACKEND_OPENSSL
   if ( 1 != RAND_bytes(nonce, XCHACHA_NONCE_LEN) ) {
      std::cerr << "StringCrypter::encryptStringXChaCha: RAND_bytes (nonce) failed" << std::endl;
      return;
   }
#else
   // Without OpenSSL we still need entropy. Use /dev/urandom via
   // QRandomGenerator::system() — cryptographically seeded on every
   // platform Qt 6 supports.
   {
      QByteArray tmp(XCHACHA_NONCE_LEN, Qt::Uninitialized);
      // Avoid a Qt header import here — fall back to a portable
      // open()+read() over /dev/urandom or BCryptGenRandom on Windows.
      // Simplest portable path: use std::random_device byte by byte.
      std::random_device rd;
      for ( int i = 0; i < XCHACHA_NONCE_LEN; ++i )
         nonce[i] = (unsigned char)(rd() & 0xff);
   }
#endif

   if ( g_cacheValid && g_cacheBackend == CryptoBackend::XChaCha &&
        passBytes == g_cachePassword &&
        g_cacheArgonBlocks == XCHACHA_ARGON2_NB_BLOCKS &&
        g_cacheArgonPasses == XCHACHA_ARGON2_PASSES &&
        g_cacheArgonLanes  == XCHACHA_ARGON2_LANES ) {
      std::memcpy(salt, g_cacheSalt, XCHACHA_SALT_LEN);
      std::memcpy(key,  g_cacheKey,  XCHACHA_KEY_LEN);
   } else {
#ifdef TUXCARDS_BACKEND_OPENSSL
      if ( 1 != RAND_bytes(salt, XCHACHA_SALT_LEN) ) {
         std::cerr << "StringCrypter::encryptStringXChaCha: RAND_bytes (salt) failed" << std::endl;
         return;
      }
#else
      std::random_device rd;
      for ( int i = 0; i < XCHACHA_SALT_LEN; ++i )
         salt[i] = (unsigned char)(rd() & 0xff);
#endif
      if ( !deriveArgon2iKey(passBytes, salt,
                             XCHACHA_ARGON2_NB_BLOCKS,
                             XCHACHA_ARGON2_PASSES,
                             XCHACHA_ARGON2_LANES,
                             key) ) {
         std::cerr << "StringCrypter::encryptStringXChaCha: Argon2i failed" << std::endl;
         return;
      }
      g_cacheBackend = CryptoBackend::XChaCha;
      g_cachePassword = passBytes;
      std::memcpy(g_cacheSalt, salt, XCHACHA_SALT_LEN);
      std::memcpy(g_cacheKey,  key,  XCHACHA_KEY_LEN);
      g_cacheArgonBlocks = XCHACHA_ARGON2_NB_BLOCKS;
      g_cacheArgonPasses = XCHACHA_ARGON2_PASSES;
      g_cacheArgonLanes  = XCHACHA_ARGON2_LANES;
      g_cacheValid = true;
   }

   QByteArray ciphertext( plaintext.size(), Qt::Uninitialized );
   unsigned char mac[XCHACHA_MAC_LEN];

   crypto_aead_lock( (uint8_t*)ciphertext.data(), mac, key, nonce,
                     nullptr, 0,
                     (const uint8_t*)plaintext.constData(),
                     (size_t)plaintext.size() );

   // Wipe stack-local key (cache still holds its own copy intentionally).
   crypto_wipe(key, XCHACHA_KEY_LEN);

   // assemble
   encryptedData.append( XCHACHA_MAGIC, (int)std::strlen(XCHACHA_MAGIC) );
   encryptedData.append( (char)XCHACHA_VERSION );
   encryptedData.append( (const char*)salt,  XCHACHA_SALT_LEN );
   encryptedData.append( (const char*)nonce, XCHACHA_NONCE_LEN );
   writeLE32(encryptedData, XCHACHA_ARGON2_NB_BLOCKS);
   writeLE16(encryptedData, XCHACHA_ARGON2_PASSES);
   encryptedData.append( (char)XCHACHA_ARGON2_LANES );
   encryptedData.append( (char)0 );  // reserved
   encryptedData.append( (const char*)mac, XCHACHA_MAC_LEN );
   encryptedData.append( ciphertext );
}


int StringCrypter::decryptStringXChaCha( const QByteArray& encryptedData,
                                         const QString& sPassWd,
                                         QString& sOutputString )
{
   sOutputString.clear();

   const int magicLen  = (int)std::strlen(XCHACHA_MAGIC);
   const int headerLen = magicLen + 1 + XCHACHA_SALT_LEN + XCHACHA_NONCE_LEN
                       + 4 + 2 + 1 + 1 + XCHACHA_MAC_LEN;
   if ( encryptedData.size() < headerLen )
      return ERROR_INVALID_ENCRYPTEDDATA;

   const unsigned char* p = (const unsigned char*)encryptedData.constData();
   int off = magicLen;
   const unsigned char ver = p[off++];
   if ( ver != XCHACHA_VERSION )
      return ERROR_INVALID_FILEHEADER;

   const unsigned char* salt  = p + off; off += XCHACHA_SALT_LEN;
   const unsigned char* nonce = p + off; off += XCHACHA_NONCE_LEN;
   const quint32 mem_kb = readLE32(p + off); off += 4;
   const quint16 passes = readLE16(p + off); off += 2;
   const quint8  lanes  = p[off++];
   off++;  // reserved
   const unsigned char* mac = p + off; off += XCHACHA_MAC_LEN;
   const int ctLen = encryptedData.size() - off;
   const unsigned char* ct = p + off;

   // Sanity caps so a malicious file can't force a multi-gigabyte
   // Argon2 allocation. 256 MB / 32 passes is far above any default
   // we ship.
   if ( mem_kb == 0 || mem_kb > 262144u )
      return ERROR_INVALID_FILEHEADER;
   if ( passes == 0 || passes > 32u )
      return ERROR_INVALID_FILEHEADER;
   if ( lanes == 0 || lanes > 16u )
      return ERROR_INVALID_FILEHEADER;

   QByteArray passBytes = sPassWd.toUtf8();
   unsigned char key[XCHACHA_KEY_LEN];

   if ( g_cacheValid && g_cacheBackend == CryptoBackend::XChaCha &&
        passBytes == g_cachePassword &&
        g_cacheArgonBlocks == mem_kb &&
        g_cacheArgonPasses == passes &&
        g_cacheArgonLanes  == lanes &&
        0 == std::memcmp(salt, g_cacheSalt, XCHACHA_SALT_LEN) ) {
      std::memcpy(key, g_cacheKey, XCHACHA_KEY_LEN);
   } else {
      if ( !deriveArgon2iKey(passBytes, salt, mem_kb, passes, lanes, key) )
         return ERROR_CRYPTO;
      g_cacheBackend = CryptoBackend::XChaCha;
      g_cachePassword = passBytes;
      std::memcpy(g_cacheSalt, salt, XCHACHA_SALT_LEN);
      std::memcpy(g_cacheKey,  key,  XCHACHA_KEY_LEN);
      g_cacheArgonBlocks = mem_kb;
      g_cacheArgonPasses = passes;
      g_cacheArgonLanes  = lanes;
      g_cacheValid = true;
   }

   QByteArray plain( ctLen, Qt::Uninitialized );
   const int rc = crypto_aead_unlock( (uint8_t*)plain.data(), mac, key, nonce,
                                      nullptr, 0, ct, (size_t)ctLen );
   crypto_wipe(key, XCHACHA_KEY_LEN);

   if ( rc != 0 )
      return ERROR_INVALID_PASSWD;

   sOutputString = QString::fromUtf8(plain);
   return NO_ERROR;
}

#endif  // TUXCARDS_BACKEND_MONOCYPHER


// =========================================================================
//                Legacy Blowfish + MD5 (TuxCards 2.0 format)
//                Kept for backward-compatible *reading* only.
// =========================================================================

void StringCrypter::encryptStringLegacyBF( const QString& sInputString,
                                           const QString& sPassWd,
                                           QByteArray& encryptedData )
{
   if ( sPassWd.isEmpty() )
   {
      std::cerr << "StringCrypter::encryptStringLegacyBF: empty password" << std::endl;
      return;
   }

   QByteArray passBytes = sPassWd.toUtf8();

   CMD5 md5;
   md5.MD5Init();
   md5.MD5Update((unsigned char *)passBytes.data(), passBytes.size());
   md5.MD5Final();

   CBlowFish fish;
   fish.Initialize( (unsigned char*)passBytes.data(), passBytes.size() );

   encryptedData.clear();
   encryptedData.append( LEGACY_MAGIC, (int)std::strlen(LEGACY_MAGIC) );

   unsigned char passWordHashValue[16];
   std::memset( passWordHashValue, 0, 16 );
   std::memcpy( passWordHashValue, md5.GetDigestBinary(), 16 );
   encryptedData.append( (const char*)passWordHashValue, 16 );

   union aInt iSize;
   iSize.theInt = sInputString.length();
   encryptedData.append( (char)iSize.i.byte0 );
   encryptedData.append( (char)iSize.i.byte1 );
   encryptedData.append( (char)iSize.i.byte2 );
   encryptedData.append( (char)iSize.i.byte3 );
   encryptedData.append( '*' );

   unsigned char buf_inout[BUFFER_SIZE];

   QByteArray inputBytes = sInputString.toUtf8();
   int totalLen = inputBytes.size();
   int offset = 0;

   while ( offset < totalLen )
   {
      int partLen = std::min((int)BUFFER_SIZE, totalLen - offset);
      std::memset( buf_inout, 0, BUFFER_SIZE );
      std::memcpy( buf_inout, inputBytes.constData() + offset, partLen );

      int encryptPadding = 0;
      if ( partLen % BFISH_BUF_MOD != 0 )
         encryptPadding = BFISH_BUF_MOD - partLen % BFISH_BUF_MOD;

      int iEncodedSize = fish.Encode( buf_inout, buf_inout, partLen + encryptPadding );
      encryptedData.append( (const char*)buf_inout, iEncodedSize );

      offset += BUFFER_SIZE;
   }
}


int StringCrypter::decryptStringLegacyBF( const QByteArray& encryptedData,
                                          const QString& sPassWd,
                                          QString& sOutputString )
{
   unsigned char buf_inout[BUFFER_SIZE];
   CBlowFish fish;

   if (encryptedData.size() <= 0)
      return ERROR_INVALID_ENCRYPTEDDATA;

   unsigned char sHash[16];
   std::memset( sHash, 0, 16 );
   int offset = (int)std::strlen(LEGACY_MAGIC);
   for ( int i = 0; i < 16; i++ )
      sHash[i] = encryptedData[offset + i];

   QByteArray passBytes = sPassWd.toUtf8();

   CMD5 md5;
   md5.MD5Init();
   md5.MD5Update( (unsigned char *)passBytes.data(), passBytes.size() );
   md5.MD5Final();

   if ( 0 != std::memcmp(sHash, md5.GetDigestBinary(), 16) )
      return ERROR_INVALID_PASSWD;

   union aInt iSize;
   iSize.i.byte0 = encryptedData[offset+16+0];
   iSize.i.byte1 = encryptedData[offset+16+1];
   iSize.i.byte2 = encryptedData[offset+16+2];
   iSize.i.byte3 = encryptedData[offset+16+3];

   int iIndexOfAsterisc = offset+16+3+1;
   int tmpArraySize = encryptedData.size() - iIndexOfAsterisc - 1;
   QByteArray tmpArray;
   tmpArray.resize(tmpArraySize);
   for ( int i = iIndexOfAsterisc+1; i < encryptedData.size(); i++ )
      tmpArray[i-iIndexOfAsterisc-1] = encryptedData[i];

   fish.Initialize( (unsigned char *)passBytes.data(), passBytes.size() );

   QByteArray leftArray;
   int bufSize = StringCrypter::min(tmpArray.size(), BUFFER_SIZE);
   sOutputString = "";

   while ( bufSize > 0 )
   {
      getLeftBytes(leftArray, tmpArray, bufSize );
      std::memset( buf_inout, 0, BUFFER_SIZE );
      std::memcpy( buf_inout, leftArray.data(), bufSize );

      int decryptPadding = 0;
      if ( bufSize % BFISH_BUF_MOD != 0 )
         decryptPadding = BFISH_BUF_MOD - bufSize % BFISH_BUF_MOD;

      fish.Decode( buf_inout, buf_inout, bufSize + decryptPadding );

      if ( bufSize < BUFFER_SIZE )
         sOutputString += QString::fromUtf8( (const char*) buf_inout, -1 );
      else
         sOutputString += QString::fromUtf8( (const char*) buf_inout, bufSize );

      bufSize = StringCrypter::min(tmpArray.size(), BUFFER_SIZE);
   }

   return NO_ERROR;
}


// =========================================================================
//                              Utilities
// =========================================================================

int StringCrypter::min(int num1, int num2)
{
   return num1 < num2 ? num1 : num2;
}

void StringCrypter::getLeftBytes( QByteArray& dest, QByteArray& source, uint len )
{
   dest.clear();

   if ( (uint)source.size() < len )
      len = source.size();

   dest.resize(len);
   for ( uint i = 0; i < len; i++ )
      dest[i] = source[i];

   int newSize = source.size() - len;
   QByteArray tmp;
   tmp.resize(newSize);
   for ( int i = 0; i < newSize; i++ )
      tmp[i] = source[i + len];

   source = tmp;
}
