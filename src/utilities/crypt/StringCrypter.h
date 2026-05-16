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
    *   - "Fh_enc:AES256GCM-PBKDF2" v1 — current format (AES-256-GCM,
    *     PBKDF2-HMAC-SHA256, 200000 iterations, 16-byte salt, 12-byte IV).
    *   - "Fh_enc:BF10" — legacy Blowfish + MD5 from TuxCards 2.0.
    * Both formats can be read; encryptString() always writes the
    * current format.
    */
   static int  decryptString( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );
   static void encryptString( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );

   // Errorcodes
   enum {
      NO_ERROR                      =  0,
      ERROR_INVALID_FILEHEADER      = -1,
      ERROR_INVALID_PASSWD          = -2,
      ERROR_INVALID_ENCRYPTEDDATA   = -3,
      ERROR_CRYPTO                  = -4    // OpenSSL operation failed
   };

private:
   // --- legacy Blowfish + MD5 path ---
   static void encryptStringLegacyBF( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );
   static int  decryptStringLegacyBF( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );

   // --- current AES-256-GCM + PBKDF2 path ---
   static void encryptStringAESGCM( const QString& sInputString, const QString& sPassWd, QByteArray& encryptedData );
   static int  decryptStringAESGCM( const QByteArray& encryptedData, const QString& sPassWd, QString& sOutputString );

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

