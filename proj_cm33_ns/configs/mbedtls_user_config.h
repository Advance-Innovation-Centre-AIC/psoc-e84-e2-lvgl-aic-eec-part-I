/**
 * \file config.h
 *
 * \brief Configuration options (set of defines)
 *
 *  This set of compile-time options may be used to enable
 *  or disable features selectively, and reduce the global
 *  memory footprint.
 */
/*
 *  Copyright (C) 2006-2018, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#ifndef MBEDTLS_USER_CONFIG_HEADER
#define MBEDTLS_USER_CONFIG_HEADER

#if !defined(COMPONENT_4390X)
#include "cy_syslib.h"
#endif

/**
 * Compiling Mbed TLS for Cortex-M0/0+/1/M23 cores with optimization enabled and on ARMC6 compiler results in errors.
 * These cores lack the required full Thumb-2 support, causing the inline assembly to require more registers than available.
 * The workaround is to use 'MULADDC_CANNOT_USE_R7' compilation flag, or without optimization flag,
 * but note that this will compile without the assmebly optimization.
 */

#if defined(COMPONENT_CM0P) && defined(COMPONENT_ARM)
#define MULADDC_CANNOT_USE_R7
#endif

/* Currently there is a bug with MBEDTLS 3.4.0 compilation with IAR compiler when assembly instructions are enabled. Hence
 * disabling assembly instructions for IAR. This will be fixed in future MBEDTLS releases.
 */
#if defined (__IAR_SYSTEMS_ICC__)
#undef MBEDTLS_HAVE_ASM
#endif

#undef MBEDTLS_HAVE_TIME_DATE

#define MBEDTLS_PLATFORM_TIME_ALT

#define MBEDTLS_ENTROPY_HARDWARE_ALT

#undef MBEDTLS_ECP_DP_SECP192R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP224R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP384R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP521R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP192K1_ENABLED
#undef MBEDTLS_ECP_DP_SECP224K1_ENABLED
#undef MBEDTLS_ECP_DP_SECP256K1_ENABLED
#undef MBEDTLS_ECP_DP_BP256R1_ENABLED
#undef MBEDTLS_ECP_DP_BP384R1_ENABLED
#undef MBEDTLS_ECP_DP_BP512R1_ENABLED
#undef MBEDTLS_ECP_DP_CURVE448_ENABLED

#undef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#undef MBEDTLS_PK_PARSE_EC_EXTENDED

#undef MBEDTLS_FS_IO

#define MBEDTLS_NO_PLATFORM_ENTROPY

#define MBEDTLS_ENTROPY_FORCE_SHA256

#undef MBEDTLS_SELF_TEST

#undef MBEDTLS_SSL_FALLBACK_SCSV

#undef MBEDTLS_SSL_CBC_RECORD_SPLITTING

#undef MBEDTLS_SSL_RENEGOTIATION

#undef MBEDTLS_SSL_PROTO_TLS1

#undef MBEDTLS_SSL_PROTO_TLS1_1

#undef MBEDTLS_SSL_PROTO_DTLS

#undef MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT

#undef MBEDTLS_SSL_DTLS_CONNECTION_ID

#undef MBEDTLS_SSL_DTLS_ANTI_REPLAY

#undef MBEDTLS_SSL_DTLS_HELLO_VERIFY

#undef MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE

#undef MBEDTLS_SSL_DTLS_BADMAC_LIMIT

#undef MBEDTLS_SSL_EXPORT_KEYS

#undef MBEDTLS_SSL_TRUNCATED_HMAC

#undef MBEDTLS_AESNI_C

#undef MBEDTLS_NET_C

#undef MBEDTLS_SSL_COOKIE_C

#undef MBEDTLS_TIMING_C

#undef MBEDTLS_X509_CRL_PARSE_C

#undef MBEDTLS_X509_CSR_PARSE_C

#undef MBEDTLS_X509_CREATE_C

#undef MBEDTLS_X509_CSR_WRITE_C

#undef MBEDTLS_X509_CRT_WRITE_C

#undef MBEDTLS_CERTS_C

#undef MBEDTLS_ERROR_C

#undef MBEDTLS_PADLOCK_C

#undef MBEDTLS_RIPEMD160_C

#undef MBEDTLS_ARC4_C

#undef MBEDTLS_XTEA_C

#undef MBEDTLS_BLOWFISH_C

#undef MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED

#undef MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED

#undef MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED

#undef MBEDTLS_PSA_CRYPTO_STORAGE_C

#undef MBEDTLS_PSA_ITS_FILE_C

#define MBEDTLS_SSL_PROTO_TLS1_3

#ifndef MBEDTLS_SSL_PROTO_TLS1_3
#undef MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
#endif

/* When TLS1.3 is enabled, session ticket flag must be enabled */
#ifndef MBEDTLS_SSL_PROTO_TLS1_3
#undef MBEDTLS_SSL_SESSION_TICKETS
#endif

#ifdef MBEDTLS_SSL_PROTO_TLS1_3
#define MBEDTLS_PK_RSA_ALT_SUPPORT
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE
#endif

#ifdef CY_TFM_PSA_SUPPORTED
#ifndef CY_SECURE_SOCKETS_PKCS_SUPPORT
#undef MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_PSA_CRYPTO_CLIENT
#endif
#endif

#define MBEDTLS_DEPRECATED_REMOVED

#define MBEDTLS_VERBOSE 0

#undef MBEDTLS_DEBUG_C

#undef MBEDTLS_LMS_C

#undef MBEDTLS_PKCS7_C

/* Force TLS 1.3 for server side */
#define FORCE_TLS_VERSION MBEDTLS_SSL_VERSION_TLS1_3

/* Platform time alt */
#define MBEDTLS_PLATFORM_MS_TIME_ALT

/* Enable MXCRYPTO PSA driver */
#define IFX_PSA_MXCRYPTO_PRESENT
#define PSA_CRYPTO_ACCELERATOR_DRIVER_PRESENT
#define MBEDTLS_PSA_CRYPTO_DRIVERS
#define MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS

#define MBEDTLS_USE_PSA_CRYPTO

#define MBEDTLS_PSA_CRYPTO_CONFIG

/* Enable alternate crypto implementations for hardware acceleration */
#ifndef DISABLE_MBEDTLS_ACCELERATION
#include "mbedtls_alt_config.h"

/* MBEDTLS defines for Dcache supported platforms */
#if !defined (CY_DISABLE_XMC7000_DATA_CACHE) && defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_THREADING_ALT
#define MBEDTLS_THREADING_C
#endif

#ifdef MBEDTLS_ECP_DP_SECP192K1_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif
#ifdef MBEDTLS_ECP_DP_SECP224K1_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif
#ifdef MBEDTLS_ECP_DP_SECP256K1_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif
#ifdef MBEDTLS_ECP_DP_BP256R1_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif
#ifdef MBEDTLS_ECP_DP_BP384R1_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif
#ifdef MBEDTLS_ECP_DP_BP512R1_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif
#ifdef MBEDTLS_ECP_DP_CURVE25519_ENABLED
#undef MBEDTLS_ECP_ALT
#undef MBEDTLS_ECDH_GEN_PUBLIC_ALT
#undef MBEDTLS_ECDSA_SIGN_ALT
#undef MBEDTLS_ECDSA_VERIFY_ALT
#endif

#endif /* DISABLE_MBEDTLS_ACCELERATION */

#endif /* MBEDTLS_USER_CONFIG_HEADER */
