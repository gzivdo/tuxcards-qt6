/***************************************************************************
                          StringCrypter.cpp
                          - legacy Blowfish + MD5 (TuxCards 2.0 format)
                          - current AES-256-GCM + PBKDF2 (Qt6 port)
 ***************************************************************************/

#include "BlowFish.h"
#include "MD5.h"

#include <QString>
#include <QByteArray>
#include <QIODevice>
#include <iostream>
#include <cstring>

#ifndef TUXCARDS_NO_OPENSSL
#  include <openssl/evp.h>
#  include <openssl/rand.h>
#  include <openssl/err.h>
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

static const int BFISH_BUF_MOD = 8;


// =========================================================================
//                              Public dispatcher
// =========================================================================

void StringCrypter::encryptString( const QString& sInputString, const QString& sPassWd,
                                   QByteArray& encryptedData )
{
#ifndef TUXCARDS_NO_OPENSSL
   // Always write the current (AES-GCM) format.
   //
   // Legacy "Fh_enc:BF10" (Blowfish + MD5(password)) is only kept on
   // the read side. Re-writing it would offer no compatibility benefit
   // for newly-encrypted data, and would lock users into the weaker
   // primitive. On the first save after decrypting an old file the
   // payload is silently upgraded to AES-256-GCM.
   encryptStringAESGCM( sInputString, sPassWd, encryptedData );
#else
   // Build without OpenSSL — fall back to the legacy Blowfish + MD5
   // format. Files produced this way can still be read by builds that
   // do have OpenSSL (legacy format is auto-detected on decrypt).
   encryptStringLegacyBF( sInputString, sPassWd, encryptedData );
#endif
}

int StringCrypter::decryptString( const QByteArray& encryptedData,
                                  const QString& sPassWd, QString& sOutputString )
{
   if ( encryptedData.size() <= 0 )
      return ERROR_INVALID_ENCRYPTEDDATA;

   const int aesLen    = (int)std::strlen(AESGCM_MAGIC);
   const int legacyLen = (int)std::strlen(LEGACY_MAGIC);

   if ( encryptedData.size() >= aesLen &&
        0 == std::memcmp(encryptedData.constData(), AESGCM_MAGIC, aesLen) )
   {
#ifndef TUXCARDS_NO_OPENSSL
      return decryptStringAESGCM( encryptedData, sPassWd, sOutputString );
#else
      std::cerr << "StringCrypter: file is AES-256-GCM encrypted, but "
                   "this build was compiled without OpenSSL support."
                << std::endl;
      return ERROR_CRYPTO;
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
// OpenSSL fallbacks (Blowfish for writes, ERROR_CRYPTO for reads).

#ifndef TUXCARDS_NO_OPENSSL

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
   if ( 1 != RAND_bytes(salt, AESGCM_SALT_LEN) ||
        1 != RAND_bytes(iv,   AESGCM_IV_LEN) ) {
      std::cerr << "StringCrypter::encryptStringAESGCM: RAND_bytes failed" << std::endl;
      return;
   }

   unsigned char key[AESGCM_KEY_LEN];
   if ( 1 != PKCS5_PBKDF2_HMAC( passBytes.constData(), passBytes.size(),
                                salt, AESGCM_SALT_LEN,
                                AESGCM_PBKDF2_ITERS,
                                EVP_sha256(),
                                AESGCM_KEY_LEN, key ) ) {
      std::cerr << "StringCrypter::encryptStringAESGCM: PBKDF2 failed" << std::endl;
      return;
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
   if ( 1 != PKCS5_PBKDF2_HMAC( passBytes.constData(), passBytes.size(),
                                salt, AESGCM_SALT_LEN,
                                (int)iters,
                                EVP_sha256(),
                                AESGCM_KEY_LEN, key ) ) {
      return ERROR_CRYPTO;
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

#endif  // TUXCARDS_NO_OPENSSL


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
