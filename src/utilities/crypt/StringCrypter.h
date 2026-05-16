/***************************************************************************
                          StringCrypter.h  -  description
                             -------------------
    begin                : Fri Jul 18 2003
    copyright            : (C) 2003 by Alexander Theel
    email                : alex.theel@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef STRING_CRYPTER_H
#define STRING_CRYPTER_H

#include <QString>


class StringCrypter{
public:
   /**
    * Auto-detects the on-disk format by magic header:
    *   - "Fh_enc:AES256GCM-PBKDF2" v1 — AES-256-GCM, PBKDF2-HMAC-SHA256,
    *     200000 iterations, 16-byte salt, 12-byte IV. Requires the
    *     OpenSSL backend (TUXCARDS_BACKEND_OPENSSL).
    *   - "Fh_enc:XChaCha20Poly1305-Argon2i-v1" — XChaCha20-Poly1305 +
    *     Argon2i KDF, vendored via monocypher. Requires
    *     TUXCARDS_BACKEND_MONOCYPHER.
    *   - "Fh_enc:BF10" — legacy Blowfish + MD5 from TuxCards 2.0
    *     (read-only fallback for very old files).
    * Multiple backends may be linked simultaneously; encryptString
    * writes whichever backend was selected at build time via
    * TUXCARDS_WRITE_BACKEND.
    */
   static int  decryptString( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );
   static void encryptString( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );

   /**
    * Discard the in-process key cache (password + salt + derived key).
    * Used to bound how long the derived key sits in memory; not required
    * for correctness — the next encrypt/decrypt pays one KDF run.
    * Wipes the cache for every backend.
    */
   static void clearKeyCache();

   // Errorcodes
   enum {
      NO_ERROR                      =  0,
      ERROR_INVALID_FILEHEADER      = -1,
      ERROR_INVALID_PASSWD          = -2,
      ERROR_INVALID_ENCRYPTEDDATA   = -3,
      ERROR_CRYPTO                  = -4,   // OpenSSL operation failed
      ERROR_UNSUPPORTED_BACKEND     = -5    // blob format recognised but the
                                            // backend it needs wasn't linked
   };

   // Blob format identifiers. identifyBlobFormat() looks at the magic
   // header; isBackendAvailableFor() reports whether this build has
   // the code to decrypt that format.
   enum BlobFormat {
      BLOB_UNENCRYPTED = 0,
      BLOB_AESGCM      = 1,   // "Fh_enc:AES256GCM-PBKDF2"
      BLOB_XCHACHA     = 2,   // "Fh_enc:XChaCha20Poly1305-Argon2i-v1"
      BLOB_LEGACY_BF   = 3,   // "Fh_enc:BF10"
      BLOB_UNKNOWN     = 4    // recognised as encrypted-ish but unknown magic
   };
   static int  identifyBlobFormat( const QByteArray& blob );
   static bool isBackendAvailableFor( int blobFormat );

   // Push the user's "Encryption format" choice from Options into the
   // crypter, so subsequent encryptString() calls write that format.
   // blobFormat must be one of BLOB_AESGCM or BLOB_XCHACHA. Anything
   // else is ignored. The default is the compile-time
   // TUXCARDS_WRITE_BACKEND.
   static void setWriteBackend( int blobFormat );
   static int  getWriteBackend();

private:
   // --- legacy Blowfish + MD5 path ---
   static void encryptStringLegacyBF( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );
   static int  decryptStringLegacyBF( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );

   // --- AES-256-GCM + PBKDF2 path (OpenSSL backend) ---
   static void encryptStringAESGCM( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );
   static int  decryptStringAESGCM( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );

   // --- XChaCha20-Poly1305 + Argon2i path (monocypher backend) ---
   static void encryptStringXChaCha( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );
   static int  decryptStringXChaCha( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );

   static void getLeftBytes( QByteArray& dest, QByteArray& source, uint len );
   static int  min(int num1, int num2);

   union aInt
   {
      int theInt;
      struct {
         int byte3:8;
         int byte2:8;
         int byte1:8;
         int byte0:8;
     } i;
   };

   static const int BUFFER_SIZE;
};

#endif

