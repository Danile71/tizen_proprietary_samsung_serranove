/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Elliptic Curve Cryptography library.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Dr Vipul Gupta <vipul.gupta@sun.com> and
 *   Douglas Stebila <douglas@stebila.ca>, Sun Microsystems Laboratories
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif


#include "blapi.h"
#include "prerr.h"
#include "secerr.h"
#include "secmpi.h"
#include "secitem.h"
#include "mplogic.h"
#include "ec.h"
#include "ecl.h"


/* Generates a new EC key pair. The private key is a supplied
 * value and the public key is the result of performing a scalar 
 * point multiplication of that value with the curve's base point.
 */
SECStatus 
ec_NewKey(ECParams *ecParams, ECPrivateKey **privKey, 
    const unsigned char *privKeyBytes, int privKeyLen)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);

    return rv;

}

/* Generates a new EC key pair. The private key is a supplied
 * random value (in seed) and the public key is the result of 
 * performing a scalar point multiplication of that value with 
 * the curve's base point.
 */
SECStatus 
EC_NewKeyFromSeed(ECParams *ecParams, ECPrivateKey **privKey, 
    const unsigned char *seed, int seedlen)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
    return rv;
}


/* Generates a new EC key pair. The private key is a random value and
 * the public key is the result of performing a scalar point multiplication
 * of that value with the curve's base point.
 */
SECStatus 
EC_NewKey(ECParams *ecParams, ECPrivateKey **privKey)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
    
    return rv;
}

/* Validates an EC public key as described in Section 5.2.2 of
 * X9.62. The ECDH primitive when used without the cofactor does
 * not address small subgroup attacks, which may occur when the
 * public key is not valid. These attacks can be prevented by 
 * validating the public key before using ECDH.
 */
SECStatus 
EC_ValidatePublicKey(ECParams *ecParams, SECItem *publicValue)
{
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
    return SECFailure;
}

/* 
** Performs an ECDH key derivation by computing the scalar point
** multiplication of privateValue and publicValue (with or without the
** cofactor) and returns the x-coordinate of the resulting elliptic
** curve point in derived secret.  If successful, derivedSecret->data
** is set to the address of the newly allocated buffer containing the
** derived secret, and derivedSecret->len is the size of the secret
** produced. It is the caller's responsibility to free the allocated
** buffer containing the derived secret.
*/
SECStatus 
ECDH_Derive(SECItem  *publicValue, 
            ECParams *ecParams,
            SECItem  *privateValue,
            PRBool    withCofactor,
            SECItem  *derivedSecret)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);

    return rv;
}

/* Computes the ECDSA signature (a concatenation of two values r and s)
 * on the digest using the given key and the random value kb (used in
 * computing s).
 */
SECStatus 
ECDSA_SignDigestWithSeed(ECPrivateKey *key, SECItem *signature, 
    const SECItem *digest, const unsigned char *kb, const int kblen)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);

   return rv;
}

/*
** Computes the ECDSA signature on the digest using the given key 
** and a random seed.
*/
SECStatus 
ECDSA_SignDigest(ECPrivateKey *key, SECItem *signature, const SECItem *digest)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);

    return rv;
}

/*
** Checks the signature on the given digest using the key provided.
*/
SECStatus 
ECDSA_VerifyDigest(ECPublicKey *key, const SECItem *signature, 
                 const SECItem *digest)
{
    SECStatus rv = SECFailure;
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);

    return rv;
}

