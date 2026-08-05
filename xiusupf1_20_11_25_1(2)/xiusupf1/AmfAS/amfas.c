#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "jansson.h"
#include "app_stack.h"
#include "app_endpoint.h"
#include "aero_tcp.h"
#include "ogs_fwk.h"
#include "ecc.h"
#include "kasumi.h"
#include "ogs-aes.h"
#include "milenage.h"
#include "ogs-aes-cmac.h"
#include "ogs-base64.h"
#include "ogs-sha1.h"
#include "ogs-sha1-hmac.h"
#include "ogs-sha2.h"
#include "ogs-sha2-hmac.h"
#include "snow-3g.h"
#include "zuc.h"
#include "snow-3g.h"
#include "snow3g.h"

#define MAX_NUM_OF_KDF_PARAM                    16

#define FC_FOR_CK_PRIME_IK_PRIME_DERIVATION     0x20
#define FC_FOR_5GS_ALGORITHM_KEY_DERIVATION     0x69
#define FC_FOR_KAUSF_DERIVATION                 0x6A
#define FC_FOR_RES_STAR_XRES_STAR_DERIVATION    0x6B
#define FC_FOR_KSEAF_DERIVATION                 0x6C
#define FC_FOR_KAMF_DERIVATION                  0x6D
#define FC_FOR_KGNB_KN3IWF_DERIVATION           0x6E
#define FC_FOR_NH_GNB_DERIVATION                0x6F

#define FC_FOR_KASME                            0x10
#define FC_FOR_KENB_DERIVATION                  0x11
#define FC_FOR_NH_ENB_DERIVATION                0x12
#define FC_FOR_EPS_ALGORITHM_KEY_DERIVATION     0x15
#define FC_FOR_CK_IK_DERIVATION_HANDOVER        0x16
#define FC_FOR_NAS_TOKEN_DERIVATION             0x17
#define FC_FOR_KASME_DERIVATION_IDLE_MOBILITY   0x19
#define FC_FOR_CK_IK_DERIVATION_IDLE_MOBILITY   0x1B


#define OGS_RAND_LEN                    16
#define OGS_AUTN_LEN                    16
#define OGS_AUTS_LEN                    14
#define OGS_MAX_RES_LEN                 16
#define OGS_SHA256_DIGEST_SIZE 			( 256 / 8)
#define OGS_KEYSTRLEN(x)                ((x*2)+1)




typedef struct kdf_param_s {
    const uint8_t *buf;
    uint16_t len;
} kdf_param_t[MAX_NUM_OF_KDF_PARAM];



/* KDF function : TS.33220 cluase B.2.0 */
static void ogs_kdf_common(const uint8_t *key, uint32_t key_size, uint8_t fc, kdf_param_t param, uint8_t *output)
{
    int i = 0, pos;
    uint8_t *s = NULL;

    pos = 1; /* FC Value */

    /* Calculate buffer length */
    for (i = 0; i < MAX_NUM_OF_KDF_PARAM && param[i].buf && param[i].len; i++) {
        pos += (param[i].len + 2);
    }

    s = ogs_calloc(1, pos);
    //ogs_assert(s);

    /* Copy buffer from param */
    pos = 0;
    s[pos++] = fc;
    for (i = 0; i < MAX_NUM_OF_KDF_PARAM && param[i].buf && param[i].len; i++) {
        uint16_t len;

        memcpy(&s[pos], param[i].buf, param[i].len);
        pos += param[i].len;
        len = htobe16(param[i].len);
        memcpy(&s[pos], &len, sizeof(len));
        pos += 2;
    }

    ogs_hmac_sha256(key, key_size, s, pos, output, OGS_SHA256_DIGEST_SIZE);

    ogs_free(s);
}



/* TS33.501 Annex A.2 : Kausf derviation function */
void ogs_kdf_kausf( uint8_t *ck, uint8_t *ik, char *serving_network_name, uint8_t *autn, uint8_t *kausf)
{
    kdf_param_t param;
    uint8_t key[OGS_KEY_LEN*2];

    memcpy(key, ck, OGS_KEY_LEN);
    memcpy(key+OGS_KEY_LEN, ik, OGS_KEY_LEN);

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)serving_network_name;
    param[0].len = strlen(serving_network_name);
    param[1].buf = autn;
    param[1].len = OGS_SQN_XOR_AK_LEN;

    ogs_kdf_common(key, OGS_KEY_LEN*2, FC_FOR_KAUSF_DERIVATION, param, kausf);
}



/* TS33.501 Annex A.4 : RES* and XRES* derivation function */
void ogs_kdf_xres_star( uint8_t *ck, uint8_t *ik, char *serving_network_name, uint8_t *rand, uint8_t *xres, size_t xres_len, uint8_t *xres_star)
{
    kdf_param_t param;
    uint8_t key[OGS_KEY_LEN*2];
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memcpy(key, ck, OGS_KEY_LEN);
    memcpy(key+OGS_KEY_LEN, ik, OGS_KEY_LEN);

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)serving_network_name;
    param[0].len = strlen(serving_network_name);
    param[1].buf = rand;
    param[1].len = OGS_RAND_LEN;
    param[2].buf = xres;
    param[2].len = xres_len;

    ogs_kdf_common(key, OGS_KEY_LEN*2, FC_FOR_RES_STAR_XRES_STAR_DERIVATION, param, output);

    memcpy(xres_star, output+OGS_KEY_LEN, OGS_KEY_LEN);
}



/* TS33.501 Annex A.5 : HRES* and HXRES* derivation function */
void ogs_kdf_hxres_star(uint8_t *rand, uint8_t *xres_star, uint8_t *hxres_star)
{
    uint8_t message[OGS_RAND_LEN + OGS_KEY_LEN];
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memcpy(message, rand, OGS_RAND_LEN);
    memcpy(message+OGS_RAND_LEN, xres_star, OGS_KEY_LEN);

    ogs_sha256(message, OGS_RAND_LEN+OGS_KEY_LEN, output);

    memcpy(hxres_star, output+OGS_KEY_LEN, OGS_KEY_LEN);
}



/* TS33.501 Annex A.6 : Kseaf derivation function */
void ogs_kdf_kseaf(char *serving_network_name, const uint8_t *kausf, uint8_t *kseaf)
{
    kdf_param_t param;

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)serving_network_name;
    param[0].len = strlen(serving_network_name);

    ogs_kdf_common(kausf, OGS_SHA256_DIGEST_SIZE, FC_FOR_KSEAF_DERIVATION, param, kseaf);
}



/* TS33.501 Annex A.7 : Kamf derivation function */
void ogs_kdf_kamf(const char *supi, const uint8_t *abba, uint8_t abba_len, const uint8_t *kseaf, uint8_t *kamf)
{
    kdf_param_t param;
    char *val;

    val = ogs_id_get_value(supi);
    memset(param, 0, sizeof(param));
    param[0].buf = (const uint8_t*) val;
    //ogs_assert(param[0].buf);
    param[0].len = strlen(val);
    param[1].buf = abba;
    param[1].len = abba_len;

    ogs_kdf_common(kseaf, OGS_SHA256_DIGEST_SIZE, FC_FOR_KAMF_DERIVATION, param, kamf);

    ogs_free(val);
}



/* TS33.501 Annex A.8 : Algorithm key derivation functions */
void ogs_kdf_nas_5gs(uint8_t algorithm_type_distinguishers, uint8_t algorithm_identity, const uint8_t *kamf, uint8_t *knas)
{
    kdf_param_t param;
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memset(param, 0, sizeof(param));
    param[0].buf = &algorithm_type_distinguishers;
    param[0].len = 1;
    param[1].buf = &algorithm_identity;
    param[1].len = 1;

    ogs_kdf_common(kamf, OGS_SHA256_DIGEST_SIZE, FC_FOR_5GS_ALGORITHM_KEY_DERIVATION, param, output);
    memcpy(knas, output+16, 16);
}



/* TS33.501 Annex A.9 KgNB and Kn3iwf derivation function */
void ogs_kdf_kgnb_and_kn3iwf(const uint8_t *kamf, uint32_t ul_count, uint8_t access_type_distinguisher, uint8_t *kgnb)
{
    kdf_param_t param;

    memset(param, 0, sizeof(param));
    ul_count = htobe32(ul_count);
    param[0].buf = (uint8_t *)&ul_count;
    param[0].len = 4;
    param[1].buf = &access_type_distinguisher;
    param[1].len = 1;

    ogs_kdf_common(kamf, OGS_SHA256_DIGEST_SIZE, FC_FOR_KGNB_KN3IWF_DERIVATION, param, kgnb);
}



/* TS33.501 Annex A.10 NH derivation function */
void ogs_kdf_nh_gnb(const uint8_t *kamf, uint8_t *sync_input, uint8_t *kgnb)
{
    kdf_param_t param;

    memset(param, 0, sizeof(param));
    param[0].buf = sync_input;
    param[0].len = OGS_SHA256_DIGEST_SIZE;

    ogs_kdf_common(kamf, OGS_SHA256_DIGEST_SIZE, FC_FOR_NH_GNB_DERIVATION, param, kgnb);
}



/*
 * TS33.501 Annex C.3.4.1 Profile A
 * TS33.501 Annex C.3.4.2 Profile B
 * ANSI-X9.63-KDF
 */
void ogs_kdf_ansi_x963( const uint8_t *z, size_t z_len, const uint8_t *info, size_t info_len, uint8_t *ek, uint8_t *icb, uint8_t *mk)
{
    uint8_t input[ECC_BYTES+4+ECC_BYTES+1];
    uint8_t output[OGS_KEY_LEN+OGS_IVEC_LEN+OGS_SHA256_DIGEST_SIZE];
    uint32_t counter = 0;
    size_t counter_len = sizeof(counter);

    memcpy(input, z, z_len);
    counter = htobe32(1);
    memcpy(input+z_len, &counter, counter_len);
    memcpy(input+z_len+counter_len, info, info_len);

    ogs_sha256(input, z_len+counter_len+info_len, output);
    memcpy(ek, output, OGS_KEY_LEN);
    memcpy(icb, output+OGS_KEY_LEN, OGS_IVEC_LEN);

    counter = htobe32(2);
    memcpy(input+z_len, &counter, counter_len);

    ogs_sha256(input, z_len+counter_len+info_len, mk);
}



/* TS33.401 Annex A.2 KASME derivation function */
void ogs_auc_kasme(const uint8_t *ck, const uint8_t *ik, const uint8_t *plmn_id, const uint8_t *sqn,  const uint8_t *ak, uint8_t *kasme)
{
    kdf_param_t param;
    int i;

    uint8_t key[OGS_KEY_LEN*2];
    uint8_t sqn_xor_ak[OGS_SQN_XOR_AK_LEN];

    memcpy(key, ck, OGS_KEY_LEN);
    memcpy(key + OGS_KEY_LEN, ik, OGS_KEY_LEN);

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)plmn_id;
    param[0].len = OGS_PLMN_ID_LEN;

    for (i = 0; i < 6; i++)
        sqn_xor_ak[i] = sqn[i] ^ ak[i];

    param[1].buf = sqn_xor_ak;
    param[1].len = OGS_SQN_XOR_AK_LEN;

    ogs_kdf_common(key, OGS_SHA256_DIGEST_SIZE, FC_FOR_KASME, param, kasme);
}



/* TS33.401 Annex A.3 KeNB derivation function */
void ogs_kdf_kenb(const uint8_t *kasme, uint32_t ul_count, uint8_t *kenb)
{
    kdf_param_t param;

    memset(param, 0, sizeof(param));
    ul_count = htobe32(ul_count);
    param[0].buf = (uint8_t *)&ul_count;
    param[0].len = 4;

    ogs_kdf_common(kasme, OGS_SHA256_DIGEST_SIZE, FC_FOR_KENB_DERIVATION, param, kenb);
}



/* TS33.401 Annex A.4 NH derivation function */
void ogs_kdf_nh_enb(const uint8_t *kasme, const uint8_t *sync_input, uint8_t *kenb)
{
    kdf_param_t param;

    memset(param, 0, sizeof(param));
    param[0].buf = sync_input;
    param[0].len = OGS_SHA256_DIGEST_SIZE;

    ogs_kdf_common(kasme, OGS_SHA256_DIGEST_SIZE, FC_FOR_NH_ENB_DERIVATION, param, kenb);
}


/* TS33.401 Annex A.7 Algorithm key derivation functions */
void ogs_kdf_nas_eps(uint8_t algorithm_type_distinguishers, uint8_t algorithm_identity, const uint8_t *kasme, uint8_t *knas)
{
    kdf_param_t param;
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memset(param, 0, sizeof(param));
    param[0].buf = &algorithm_type_distinguishers;
    param[0].len = 1;
    param[1].buf = &algorithm_identity;
    param[1].len = 1;

    ogs_kdf_common(kasme, OGS_SHA256_DIGEST_SIZE, FC_FOR_EPS_ALGORITHM_KEY_DERIVATION, param, output);
    memcpy(knas, output+16, 16);
}


/* TS33.401 Annex A.8: KASME to CK', IK' derivation at handover */
void ogs_kdf_ck_ik_handover( uint32_t dl_count, const uint8_t *kasme, uint8_t *ck, uint8_t *ik)
{
    kdf_param_t param;
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)&dl_count;
    param[0].len = 4;

    ogs_kdf_common(kasme, OGS_SHA256_DIGEST_SIZE, FC_FOR_CK_IK_DERIVATION_HANDOVER, param, output);
    memcpy(ck, output, 16);
    memcpy(ik, output+16, 16);
}


/* TS33.401 Annex A.9: NAS token derivation for inter-RAT mobility */
void ogs_kdf_nas_token( uint32_t ul_count, const uint8_t *kasme, uint8_t *nas_token)
{
    kdf_param_t param;
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)&ul_count;
    param[0].len = 4;

    ogs_kdf_common(kasme, OGS_SHA256_DIGEST_SIZE, FC_FOR_NAS_TOKEN_DERIVATION, param, output);
    memcpy(nas_token, output, 2);
}


/* TS33.401 Annex A.11 : K’ASME from CK, IK derivation during idle mode mobility */
void ogs_kdf_kasme_idle_mobility( const uint8_t *ck, const uint8_t *ik, uint32_t nonce_ue, uint32_t nonce_mme, uint8_t *kasme)
{
    kdf_param_t param;
    uint8_t key[OGS_KEY_LEN*2];

    memcpy(key, ck, OGS_KEY_LEN);
    memcpy(key+OGS_KEY_LEN, ik, OGS_KEY_LEN);

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)&nonce_ue;
    param[0].len = sizeof(nonce_ue);
    param[1].buf = (uint8_t *)&nonce_mme;
    param[1].len = sizeof(nonce_mme);

    ogs_kdf_common(key, OGS_KEY_LEN*2, FC_FOR_KASME_DERIVATION_IDLE_MOBILITY, param, kasme);
}


/* TS33.401 Annex A.13: KASME to CK', IK' derivation at idle mobility */
void ogs_kdf_ck_ik_idle_mobility( uint32_t ul_count, const uint8_t *kasme, uint8_t *ck, uint8_t *ik)
{
    kdf_param_t param;
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memset(param, 0, sizeof(param));
    param[0].buf = (uint8_t *)&ul_count;
    param[0].len = 4;

    ogs_kdf_common(kasme, OGS_SHA256_DIGEST_SIZE, FC_FOR_CK_IK_DERIVATION_IDLE_MOBILITY, param, output);
    memcpy(ck, output, 16);
    memcpy(ik, output+16, 16);
}



/*
 * TS33.401 Annex I Hash Functions
 * Use the KDF given in TS33.220
 */
void ogs_kdf_hash_mme( const uint8_t *message, uint32_t message_len, uint8_t *hash_mme)
{
    uint8_t key[32];
    uint8_t output[OGS_SHA256_DIGEST_SIZE];

    memset(key, 0, 32);
    ogs_hmac_sha256(key, 32, message, message_len, output, OGS_SHA256_DIGEST_SIZE);

    memcpy(hash_mme, output+24, OGS_HASH_MME_LEN);
}

/*
 * TS33.102
 * 6.3.3 Authentication and key agreement
 * Re-use and re-transmission of (RAND, AUTN)
 */
void ogs_auc_sqn( const uint8_t *opc, const uint8_t *k, const uint8_t *rand, const uint8_t *conc_sqn_ms, uint8_t *sqn_ms, uint8_t *mac_s)
{
    int i;
    uint8_t ak[OGS_AK_LEN];
    /*
     * The AMF used to calculate MAC-S assumes a dummy value of
     * all zeros so that it does not need to be transmitted in the clear
     * in the re-synch message.
     */
    uint8_t amf[2] = { 0, 0 };

    milenage_f2345(opc, k, rand, NULL, NULL, NULL, NULL, ak);
    for (i = 0; i < OGS_SQN_LEN; i++)
        sqn_ms[i] = ak[i] ^ conc_sqn_ms[i];
    milenage_f1(opc, k, rand, sqn_ms, amf, NULL, mac_s);
}


typedef struct sub_s {
	
	struct sub_s * Next;
	
	char 		imsi[17];
	uint64_t 	sqn;
	uint8_t  	opc[16];
	uint8_t  	k[16];
	uint8_t 	xres_star[ OGS_MAX_RES_LEN ];
	uint8_t 	kseaf[ OGS_SHA256_DIGEST_SIZE ];


} sub_t;


typedef struct amfas_s {
	app_logger_t * appLogger;
	
	sub_t * subHead;
	sub_t * subCurrent;
	
	app_data_region_t * mem_alloc;
} amfas_t;


amfas_t * __amfAs = NULL;

#pragma pack(4)
typedef struct sbi_header_s {
	uint32_t Length;
	uint32_t MessageType;
	uint32_t Operation;
	uint64_t PID;
} sbi_header_t;


#pragma pack(4)
typedef struct sbi_auth_request_s {

	sbi_header_t header;
	
    char MobileId[48];
    char MobileLength;
	uint32_t MobileIdType;
	char ServingNetworkName[48];
	uint32_t Auts;	
	char RAND[OGS_RAND_LEN * 2];
	char AUTS[OGS_AUTS_LEN * 2];
	
} sbi_auth_request_t;

#pragma pack(4)
typedef struct sbi_auth_response_s {

	sbi_header_t header;
	
	uint32_t Status;
	
	char udm_nf_id[18];
	char ausf_nf_id[18];
	char ue_context[24];

    // uint8_t         rand[OGS_RAND_LEN * 2];
    // uint8_t         autn[OGS_AUTN_LEN * 2];
    // uint8_t         hxres_star[OGS_MAX_RES_LEN * 2];
	
	char rand_string[OGS_KEYSTRLEN(OGS_RAND_LEN)];
	char autn_string[OGS_KEYSTRLEN(OGS_AUTN_LEN)];
	char hxres_star_string[OGS_KEYSTRLEN(OGS_MAX_RES_LEN)];
	
	// char kausf_string[OGS_KEYSTRLEN(OGS_SHA256_DIGEST_SIZE)];
	// char xres_star_string[OGS_KEYSTRLEN(OGS_MAX_RES_LEN)];

} sbi_auth_response_t;


#pragma pack(4)
typedef struct sbi_auth_delete_s {
	sbi_header_t header;
	char ue_context[24];
} sbi_auth_delete_t;


#pragma pack(4)
typedef struct sbi_auth_delete_response_s {
	sbi_header_t header;
	uint32_t Status;
} sbi_auth_delete_response_t;


#pragma pack(4)
typedef struct sbi_auth_confirm_s {
	sbi_header_t header;
	char xres_star_string[OGS_KEYSTRLEN(OGS_MAX_RES_LEN)];
	char ue_context[48];
} sbi_auth_confirm_t;


#pragma pack(4)
typedef struct sbi_auth_confirm_response_s {
	sbi_header_t header;
	uint32_t Status;
	
	char supi[22];
	uint8_t kseaf_string[OGS_KEYSTRLEN(OGS_SHA256_DIGEST_SIZE)];
	
} sbi_auth_confirm_response_t;


#define 	MAX_SUCI_TOKEN 					16

char * ogs_supi_from_suci( char * suci)
{
    char * array[MAX_SUCI_TOKEN];
    char * p, * tmp;
    int i;
    char * supi = NULL;

    tmp = ogs_strdup(suci);
    if (!tmp) 
	{
        printf("ogs_strdup() failed");
        return NULL;
    }
	
    for (i = 0; i < MAX_SUCI_TOKEN; i++) 
	{
        array[i] = NULL;
    }
	
    p = tmp;
    i = 0;
    while((array[i++] = strsep(&p, "-"))) 
	{
        /* Empty Body */
    }

	if(array[0])
	{
		if( memcmp( array[0], "suci", 4) == 0)
		{
			if( array[1])
			{
				if( array[1][0] == '0')
				{
					if (array[2] && array[3] && array[5] && array[6] && array[7]) 
					{
						uint8_t protection_scheme_id 	= atoi(array[5]);
						
						
						if (protection_scheme_id == OGS_PROTECTION_SCHEME_NULL) 
						{
							supi = ogs_msprintf("imsi-%s%s%s", array[2], array[3], array[7]);
						}
						else if (protection_scheme_id == OGS_PROTECTION_SCHEME_PROFILE_A || protection_scheme_id == OGS_PROTECTION_SCHEME_PROFILE_B) 
						{
							// // open5gs-main\open5gs-main\lib\sbi\conv.c
						
							// uint8_t home_network_pki_value 	= atoi(array[6]);
							// ogs_datum_t pubkey;
							// ogs_datum_t cipher_text;
							// ogs_datum_t plain_text;
							// char *plain_bcd;
							// uint8_t mactag1[OGS_MACTAG_LEN], mactag2[OGS_MACTAG_LEN];

							// uint8_t z[OGS_ECCKEY_LEN];

							// uint8_t ek[OGS_KEY_LEN];
							// uint8_t icb[OGS_IVEC_LEN];
							// uint8_t mk[OGS_SHA256_DIGEST_SIZE];
							
							
							// // if ( home_network_pki_value < OGS_HOME_NETWORK_PKI_VALUE_MIN || home_network_pki_value > OGS_HOME_NETWORK_PKI_VALUE_MAX) 
							// // {
								// // ogs_error("Invalid HNET PKI Value [%s]", array[6]);
								// // break;
							// // }

							// // if (!ogs_sbi_self()->hnet[home_network_pki_value].avail) 
							// // {
								// // ogs_error("HNET PKI Value Not Avaiable [%s]", array[6]);
								// // break;
							// // }

							// // if (ogs_sbi_self()->hnet[home_network_pki_value].scheme != protection_scheme_id) 
							// // {
								// // ogs_error("Scheme Not Matched [%d != %s]", ogs_sbi_self()->hnet[protection_scheme_id].scheme, array[5]);
								// // break;
							// // }

							// // if (parse_scheme_output( array[5], array[7], &pubkey, &cipher_text, mactag1) != OGS_OK) 
							// // {
								// // ogs_error("parse_scheme_output[%s] failed", array[7]);
								// // break;
							// // }
							
							// if (protection_scheme_id == OGS_PROTECTION_SCHEME_PROFILE_A) 
							// {
								// //curve25519_donna( z, ogs_sbi_self()->hnet[home_network_pki_value].key, pubkey.data);
							// } 
							// else if (protection_scheme_id == OGS_PROTECTION_SCHEME_PROFILE_B) 
							// {
								// // if (ecdh_shared_secret( pubkey.data, ogs_sbi_self()->hnet[home_network_pki_value].key, z) != 1) 
								// // {
									// // // ogs_error("ecdh_shared_secret() failed");
									// // // ogs_log_hexdump(OGS_LOG_ERROR, pubkey.data, OGS_ECCKEY_LEN);
									// // // ogs_log_hexdump(OGS_LOG_ERROR, ogs_sbi_self()->hnet[home_network_pki_value].key, OGS_ECCKEY_LEN);
									// // // goto cleanup;
								// // }
							// } 
							// else
							// {
								
							// }

							// ogs_kdf_ansi_x963( z, OGS_ECCKEY_LEN, pubkey.data, pubkey.size, ek, icb, mk);

							// ogs_hmac_sha256( mk, OGS_SHA256_DIGEST_SIZE, cipher_text.data, cipher_text.size, mactag2, OGS_MACTAG_LEN);

							// if (memcmp(mactag1, mactag2, OGS_MACTAG_LEN) != 0) 
							// {
								// // ogs_error("MAC-tag not matched");
								// // ogs_log_hexdump(OGS_LOG_ERROR, mactag1, OGS_MACTAG_LEN);
								// // ogs_log_hexdump(OGS_LOG_ERROR, mactag2, OGS_MACTAG_LEN);
								// // goto cleanup;
							// }

							// plain_text.size = cipher_text.size;
							// plain_text.data = ogs_calloc(1, plain_text.size);

							// ogs_aes_ctr128_encrypt( ek, icb, cipher_text.data, cipher_text.size, plain_text.data);

							// plain_bcd = ogs_calloc(1, plain_text.size*2+1);

							// ogs_buffer_to_bcd( plain_text.data, plain_text.size, plain_bcd);

							// supi = ogs_msprintf("imsi-%s%s%s", array[2], array[3], plain_bcd);

							// if (plain_text.data)
								// ogs_free(plain_text.data);

							// ogs_free(plain_bcd);

							// cleanup:
								// if (pubkey.data)
									// ogs_free(pubkey.data);
								// if (cipher_text.data)
									// ogs_free(cipher_text.data);
					
						}
						else 
						{
							printf("Invalid Protection Scheme [%s]\n", array[5]);
						}
					}
				}
			}	
		}
	}
	
	ogs_free(tmp);
    return supi;
}


uint32_t app_ep__get_u32( unsigned char * buff);

int app_amfas__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	int sts = app_ep_stack__tcp_read( client, tcp_buffer, 12);
	
	//printf( "sts=%d isActive=%d  %s|%d\n", sts, client->isActive, __FILE__, __LINE__);
	
	if( sts <= 0)
		return sts;
	
	uint32_t u32 = app_ep__get_u32( tcp_buffer->buffer);

	//printf( "u32=%d sts=%d isActive=%d   %s|%d\n", u32, sts, client->isActive, __FILE__, __LINE__);
	//app_printf_buf( "", tcp_buffer->buffer, 12);
	
	if( u32 > 12)
	{
		sts = app_ep_stack__tcp_read( client, tcp_buffer, u32 - 12);
		//printf( "u32=%d sts=%d isActive=%d   %s|%d\n", u32, sts, client->isActive, __FILE__, __LINE__);
		
		if( sts <= 0)
			return sts;
	}
	
	return u32;
}


char * temp = "abcdef0123456789";
int templen = 16;

void app_amfas__tempdata( char * data, int leng)
{
	int i = 0;
	for( i = 0; i < leng; i++)
	{
		data[i] = temp[get_rand_number( 0, templen)];
	}
}

sub_t * app_amfas__find( char * imsi)
{
	//printf( "imsi=%s\n", imsi);
	
	sub_t * sub = __amfAs->subHead;
	while( sub) 
	{
		//printf( "imsi=%s == %s -> %d \n", imsi, sub->imsi, memcmp( sub->imsi, imsi, 15));
		
		if( memcmp( imsi, "imsi-", 5) == 0)
		{
			if( memcmp( sub->imsi, &imsi[5], 15) == 0) 
			{
				return sub;
			}
		}
		else
		{
			if( memcmp( sub->imsi, imsi, 15) == 0) 
			{
				return sub;
			}
		}
		sub = sub->Next;
	}
	return sub;
}

void app_amfas__handle_request( sbi_header_t * mess, app_ep_stack__tcp_client_t * client)
{
	sbi_header_t * resp = NULL;
	char amf[2] = {0x80, 0x00};
	

	switch( mess->MessageType)
	{
		case 30001:	// AUTH REQUEST
			{
				sbi_auth_request_t * req = (sbi_auth_request_t *)mess;
				
				sbi_auth_response_t auth_response;
				memset( &auth_response, 0, sizeof(sbi_auth_response_t));
				
				sub_t * sub = NULL;
				
				if(req->MobileIdType == 1)
				{
					char * supi = ogs_supi_from_suci( req->MobileId);
					sub = app_amfas__find( supi);
					ogs_free(supi);
				}
				else if( req->MobileIdType == 3)
				{
					sub = app_amfas__find( req->MobileId);
				}
				else
				{
					printf("unknown mobile-id\n");
				}
				
				if( sub)
				{
					uint8_t 	autn[OGS_AUTN_LEN];
					uint8_t 	ik[OGS_KEY_LEN];
					uint8_t 	ck[OGS_KEY_LEN];
					uint8_t 	ak[OGS_AK_LEN];
					uint8_t 	xres[OGS_MAX_RES_LEN];
					size_t 		xres_len = 8;
					uint8_t 	xres_star[OGS_MAX_RES_LEN];
					uint8_t 	kausf[OGS_SHA256_DIGEST_SIZE];
					uint8_t    	hxres_star[OGS_MAX_RES_LEN];
					//uint8_t 	kseaf[OGS_SHA256_DIGEST_SIZE];
					
					char 		rand[OGS_RAND_LEN];
					app_amfas__tempdata( rand, OGS_RAND_LEN);
					
					char sqn[OGS_SQN_LEN];
					char csqn[OGS_SQN_LEN];
					char sqn_string[OGS_KEYSTRLEN(OGS_SQN_LEN)];

					ogs_uint64_to_buffer( sub->sqn, OGS_SQN_LEN, csqn);
					ogs_hex_to_ascii( csqn, sizeof(csqn), sqn_string, sizeof(sqn_string));
					ogs_ascii_to_hex( sqn_string, strlen(sqn_string), sqn, 6);
					 
					
					milenage_generate( sub->opc, amf, sub->k, sqn, rand, autn, ik, ck, ak, xres, &xres_len);
					ogs_kdf_kausf( ck, ik, req->ServingNetworkName, autn, kausf);
					ogs_kdf_xres_star( ck, ik, req->ServingNetworkName, rand, xres, xres_len, xres_star);
					ogs_kdf_hxres_star( rand, xres_star, hxres_star);
					ogs_kdf_kseaf( req->ServingNetworkName, kausf, sub->kseaf);
					//ogs_hex_to_ascii( xres_star, 	sizeof( xres_star), auth_response.xres_star_string, 	sizeof(auth_response.xres_star_string));
					//ogs_hex_to_ascii( kausf, 		sizeof( kausf), 	auth_response.kausf_string, 		sizeof(auth_response.kausf_string));

					ogs_hex_to_ascii( rand, 		sizeof( rand), 			auth_response.rand_string, 			sizeof(auth_response.rand_string));
					ogs_hex_to_ascii( autn, 		sizeof( autn), 			auth_response.autn_string, 			sizeof(auth_response.autn_string));
					ogs_hex_to_ascii( hxres_star, 	sizeof( hxres_star), 	auth_response.hxres_star_string, 	sizeof(auth_response.hxres_star_string));
					memcpy( sub->xres_star, xres_star, OGS_MAX_RES_LEN);

					// app_printf_buf( "SQN", (uint8_t*)sqn, 6);
					// app_printf_buf( "rand", (uint8_t*)rand, OGS_RAND_LEN);
					// app_printf_buf( "xres_star", (uint8_t*)xres_star, OGS_MAX_RES_LEN);
					// app_printf_buf( "hxres_star", (uint8_t*)hxres_star, OGS_MAX_RES_LEN);
					// app_printf_buf( "autn", (uint8_t*)autn, OGS_AUTN_LEN);
					// app_printf_buf( "kausf", (uint8_t*)kausf, 		OGS_SHA256_DIGEST_SIZE);
					// app_printf_buf( "kseaf", (uint8_t*)sub->kseaf, 	OGS_SHA256_DIGEST_SIZE);
					// printf( "SNN=%s\n", req->ServingNetworkName);
					
					// exit(0);
			
					auth_response.Status = 1;

					app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "AUTH REQUEST: Generated AuthVectors and sending response for SUCI=%s MobileIdType=%d    %s|%s|%d", 
						req->MobileId, req->MobileIdType, __FILE__, __FUNCTION__, __LINE__);
				}
				else
				{
					auth_response.Status = 101;
					
					app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "AUTH REQUEST: Subscriber Not Found with SUCI=%s MobileIdType=%d    %s|%s|%d", 
						req->MobileId, req->MobileIdType, __FILE__, __FUNCTION__, __LINE__);
				}
				
				auth_response.header.Length = sizeof(sbi_auth_response_t);
				resp = &auth_response.header;
			}
			break;
		case 30003:	// AUTH DELETE
			{
				sbi_auth_delete_response_t auth_delete_response;
				memset( &auth_delete_response, 0, sizeof(sbi_auth_delete_response_t));
				
				auth_delete_response.header.MessageType = 4;
				auth_delete_response.header.Length = sizeof(sbi_auth_delete_response_t);
				
				resp = &auth_delete_response.header;
			}
			break;
		case 30005:	// AUTH CONFIRM
			{
				//printf( "received auth-confirm request  MessageType=%d    %s|%s|%d\n", mess->MessageType, __FILE__, __FUNCTION__, __LINE__);


				sbi_auth_confirm_t * auth_confirm = (sbi_auth_confirm_t *)mess; 

				char * supi = ogs_supi_from_suci( auth_confirm->ue_context);
				sub_t * sub = app_amfas__find( supi);
				ogs_free( supi);


				sbi_auth_confirm_response_t auth_confirm_response;
				memset( &auth_confirm_response, 0, sizeof(sbi_auth_confirm_response_t));

				if( sub)
				{
					uint8_t res_star[OGS_MAX_RES_LEN];
					ogs_ascii_to_hex( auth_confirm->xres_star_string, strlen( auth_confirm->xres_star_string), res_star, OGS_MAX_RES_LEN);
					
					if (memcmp( res_star, sub->xres_star, OGS_MAX_RES_LEN) != 0) 
					{
						app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_DEBUG, "AUTH CONFIRM: res_star match failed SUCI=%s   %s|%s|%d", 
							auth_confirm->ue_context, __FILE__, __FUNCTION__, __LINE__);
						
						auth_confirm_response.Status = 102;
					}
					else
					{
						app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_DEBUG, "AUTH CONFIRM: res_star matched, sending success response for SUCI=%s   %s|%s|%d", 
							auth_confirm->ue_context, __FILE__, __FUNCTION__, __LINE__);
							
						sprintf( auth_confirm_response.supi, "imsi-%s", sub->imsi);
						ogs_hex_to_ascii( sub->kseaf, OGS_SHA256_DIGEST_SIZE, auth_confirm_response.kseaf_string, sizeof(auth_confirm_response.kseaf_string));

						auth_confirm_response.Status = 1;
					}
				}
				else
				{
					app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "AUTH CONFIRM: Subscriber Not Found with SUCI=%s   %s|%s|%d", 
						auth_confirm->ue_context, __FILE__, __FUNCTION__, __LINE__);
					
					//printf( "not found sub=%p  supi=%s\n", sub, auth_confirm->ue_context);
					auth_confirm_response.Status = 101;
				}
				
				
				auth_confirm_response.header.Length = sizeof(sbi_auth_confirm_response_t);
				
				
				resp = &auth_confirm_response.header;
			}
			break;
		default:
			{
				app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "unhandled MessageType=%d, stopped processing=%d   %s|%s|%d", 
					mess->MessageType, __FILE__, __FUNCTION__, __LINE__);
			}
			break;
	}
	
	
	if( client && resp)
	{
		resp->MessageType 		= mess->MessageType;
		resp->PID 				= mess->PID;
		resp->Operation			= mess->Operation;
		uint32_t len 			= resp->Length;
		resp->Length 			= ntohl( len);


		int sentBytes = app_ep_stack__send_message( client, (char*)resp, len);
		
		if( sentBytes < 0)
		{
			app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
				"sending response failed  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		}
		else
		{
			app_logger__log( __amfAs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "sent response with Length=%u SentBytes=%d MessageType=%d   %s|%s|%d", 
				len, sentBytes, resp->MessageType, __FILE__, __FUNCTION__, __LINE__);
		}
	}
	
}


void app_amfas__tcp_qhandler( uint8_t * Data, int tIndex)
{
	app_ep_stack__tcp_buffer_t * tcp_buffer = (app_ep_stack__tcp_buffer_t *)Data;

	//printf("tcp_buffer=%p   %s|%s|%d\n", tcp_buffer, __FILE__, __FUNCTION__, __LINE__);
	
	sbi_header_t * mess = (sbi_header_t *)tcp_buffer->buffer;
	app_amfas__handle_request( mess, tcp_buffer->client);
	
	// jet_message_buffer_t * mbuff = app_jet_message__decode( tcp_buffer);
	// app_aero_message__execute( mbuff);
	// app_printf_buf( "buff", tcp_buffer->buffer, tcp_buffer->Length);
	
	app_ep_stack__release_tcp_buffer( tcp_buffer);	
}

ogs_pkbuf_t * amfd__pfbuf_allc( unsigned int size)
{
	ogs_pkbuf_t * pkbuf = (ogs_pkbuf_t *)app_region__allocate_fr( __amfAs->mem_alloc, sizeof(ogs_pkbuf_t));
	if( pkbuf) 
	{
		pkbuf->len 	= 0;
		pkbuf->data = app_region__allocate_fr( __amfAs->mem_alloc, size);
		
		if(!pkbuf->data)
		{
			ogs_pkbuf_free( pkbuf);
			return NULL;
		}
		
		pkbuf->tail = pkbuf->data;
		pkbuf->head = pkbuf->data;
		pkbuf->end 	= pkbuf->head + size;
	}
	return pkbuf;
}


uint8_t * amfd__malloc( size_t sz)
{
	return app_region__allocate_fr( __amfAs->mem_alloc, sz);
}

uint8_t * amfd__realloc( uint8_t * b, size_t sz)
{
	if(!b)
	{
		uint8_t * dPtr = app_region__allocate_fr( __amfAs->mem_alloc, sz);
		return dPtr;
	}
	
	int dlen = app_region__datalen( b);

	uint8_t * dPtr = app_region__allocate_fr( __amfAs->mem_alloc, sz);
	
	if( dlen > 0 && dlen < sz)
	{
		memcpy( dPtr, b, dlen);
	}
	
	app_region__free(b);
	return dPtr;
}

void amfd__free( uint8_t * b)
{
	app_region__free(b);
}

int main( int argc, char* argv[])
{
	if(!__amfAs)
	{
		init_rand();
		
		
		
		__amfAs = (amfas_t*)malloc(sizeof(amfas_t));
		memset( __amfAs, 0, sizeof(amfas_t));
		
		
		// typedef struct sub_s {
			
			// struct sub_s * Next;
			
			// char imsi[17];
			// uint64_t sqn;
			// uint8_t  opc[16];
			// uint8_t  k[16];
			
		// } sub_t;


		// typedef struct amfas_s {
			// app_logger_t * appLogger;
			
			// sub_t * subHead;
			// sub_t * subCurrent;
		// } amfas_t;		
		
		
		int poolsize_multiplier = 2; 
		
		if( poolsize_multiplier == 0)
			poolsize_multiplier = 1;
		
		__amfAs->mem_alloc = app_region__create();
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m20",  20,    poolsize_multiplier * 10000);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m50",  50,    poolsize_multiplier *  5000);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m100", 100,   poolsize_multiplier *  5000);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m200", 200,   poolsize_multiplier *  5000);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m500", 500,   poolsize_multiplier *  5000);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m1024", 1024, poolsize_multiplier *  2000);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m2048", 2048, poolsize_multiplier *   500);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m2048", 3072, poolsize_multiplier *   500);
		app_region__add_pool( __amfAs->mem_alloc, (unsigned char*)"m4096", 4096, poolsize_multiplier *   500);
		
		
		set_malloc_func( amfd__malloc);
		set_realloc_func( amfd__realloc);
		set_free_func( amfd__free);
		
		ogs_pkbuf_set_allocator( amfd__pfbuf_allc);
		
		
		__amfAs->subHead = NULL;
		__amfAs->subCurrent = NULL;
		
		
		
		long imsi = 987650000000001;
		int i = 0;
		sub_t * subI = NULL;
		;
		for( i = 0; i < 100; i++)
		{
			subI = (sub_t*)malloc(sizeof(sub_t));
			memset( subI, 0, sizeof(sub_t));
			
			subI->sqn = 0;
			sprintf( subI->imsi, "%ld", imsi + i);
			
			//memcpy( subI->opc, "\x46\x5B\x5C\xE8\xB1\x99\xB4\x9F\xAA\x5F\x0A\x2E\xE2\x38\xA6\xBC", 16);
			//memcpy( subI->k, "\xE8\xED\x28\x9D\xEB\xA9\x52\xE4\x28\x3B\x54\xE8\x8E\x61\x83\xCA", 16);

			memcpy( subI->opc, 	"\xc7\xea\xe3\xa9\x1a\xa8\x76\x05\x94\x2b\xcc\xb8\x89\xc5\xd7\x03", 16);
			memcpy( subI->k, 	"\x4a\x44\x8b\xc4\x42\x91\x8e\xf5\x30\x18\x9a\xc7\xfb\xc0\x50\x20", 16);
			
			if(!__amfAs->subHead)
			{
				__amfAs->subHead = __amfAs->subCurrent = subI;
			}
			else
			{
				__amfAs->subCurrent->Next = subI;
				__amfAs->subCurrent = subI;
			}
		}
		
		
		
		// sbi_auth_request_t req;
		// memset( &req, 0, sizeof(sbi_auth_request_t));
		// req.header.MessageType = 1;
		// memcpy( req.MobileId, "987650000000001", 15);
		
		// app_amfas__handle_request( (sbi_header_t*)&req, NULL);

		
		__amfAs->appLogger = app_logger__create( "./logs/", "AMFAS", ".log", 500000, 5);
		
		char * ip 	= "192.168.144.12";
		int port 	= 8955;
		
		app_ep_stack__tcp_stack_init( 		app_amfas__tcp_qhandler);
		app_ep_stack__tcp_stack_set_logger( __amfAs->appLogger);
		app_ep_stack__start_server( ip, port, APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK, 0, app_amfas__tcp_read, 5000, 1024);
		
		printf("application started, listening on IP=%s and port=%d \n", ip, port);
		
		
		while(1)
		{
			sleep(1);
		}
	}
	return 0;
}