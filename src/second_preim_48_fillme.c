#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "struct.h"
#include "xoshiro.h"

#define ROTL24_16(x) ((((x) << 16) ^ ((x) >> 8)) & 0xFFFFFF)
#define ROTL24_3(x) ((((x) << 3) ^ ((x) >> 21)) & 0xFFFFFF)

#define ROTL24_8(x) ((((x) << 8) ^ ((x) >> 16)) & 0xFFFFFF)
#define ROTL24_21(x) ((((x) << 21) ^ ((x) >> 3)) & 0xFFFFFF)

//#define 
uint64_t IV = 0x010203040506ULL ;

/*
 * the 96-bit key is stored in four 24-bit chunks in the low bits of k[0]...k[3]
 * the 48-bit plaintext is stored in two 24-bit chunks in the low bits of p[0], p[1]
 * the 48-bit ciphertext is written similarly in c
 */
void speck48_96(const uint32_t k[4], const uint32_t p[2], uint32_t c[2])
{
	uint32_t rk[23];
	uint32_t ell[3] = {k[2], k[1], k[0]};

	rk[0] = k[3];

	c[0] = p[0];
	c[1] = p[1];

	/* full key schedule */
	for (unsigned i = 0; i < 22; i++)
	{
		uint32_t new_ell = ((ROTL24_16(ell[0]) + rk[i]) ^ i) & 0xFFFFFF; // addition (+) is done mod 2**24
		rk[i+1] = ROTL24_3(rk[i]) ^ new_ell;
		ell[0] = ell[1];
		ell[1] = ell[2];
		ell[2] = new_ell;
	}

	
	for (unsigned i = 0; i < 23; i++)
	{
		c[0] = ((ROTL24_16(c[0]) + c[1]) ^ rk[i]) & 0xFFFFFF;
		c[1] = (ROTL24_3(c[1]) ^ c[0]) & 0xFFFFFF;
	}

}

/* the inverse cipher */
void speck48_96_inv(const uint32_t k[4], const uint32_t c[2], uint32_t p[2])
{
	uint32_t rk[23];
	uint32_t ell[3] = {k[2], k[1], k[0]};

	rk[0] = k[3];

	uint32_t x = c[0];
    uint32_t y = c[1];

	/* full key schedule */
	for (unsigned i = 0; i < 22; i++)
	{
		uint32_t new_ell = ((ROTL24_16(ell[0]) + rk[i]) ^ i) & 0xFFFFFF; // addition (+) is done mod 2**24
		rk[i+1] = ROTL24_3(rk[i]) ^ new_ell;
		ell[0] = ell[1];
		ell[1] = ell[2];
		ell[2] = new_ell;
	}

	for (int8_t i = 22; i >= 0; i--)
	{	
		y = ROTL24_21(x ^ y) & 0xFFFFFF;
		uint32_t tmp = ((x ^ rk[i]) - y) & 0xFFFFFF ;
		x = (ROTL24_8(tmp)) & 0xFFFFFF;
	}
	
	p[0] = x;
	p[1] = y;
	
}

/* Test against EP 2013/404, App. C */
bool test_vector_okay()
{
    uint32_t k[4] = {0x1a1918, 0x121110, 0x0a0908, 0x020100};
    uint32_t p[2] = {0x6d2073, 0x696874};
    uint32_t c[2];
	printf("p0 = %X , p1 = %X\n", p[0], p[1]);
    speck48_96(k, p, c);
    printf("c0 = %X , c1 = %X\n", c[0], c[1]);
	if((c[0] == 0x735E10) && (c[1] == 0xB6445D)){
		printf("test _ speck48_96 _ passed\n");
	}
	speck48_96_inv(k, c, p);
	if((p[0] == 0x6d2073) && (p[1] == 0x696874)){
		printf("test _ speck48_96_inv _ passed\n");
	}
    return (c[0] == 0x735E10) && (c[1] == 0xB6445D);
}

/* The Davies-Meyer compression function based on speck48_96,
 * using an XOR feedforward
 * The input/output chaining value is given on the 48 low bits of a single 64-bit word,
 * whose 24 lower bits are set to the low half of the "plaintext"/"ciphertext" (p[0]/c[0])
 * and whose 24 higher bits are set to the high half (p[1]/c[1])
 */
uint64_t cs48_dm(const uint32_t m[4], const uint64_t h)
{
	uint32_t p[2], c[2];
    
    // extract the 48 bits of h into p[0] and p[1]
    p[0] = h & 0xFFFFFF;
    p[1] = (h >> 24) & 0xFFFFFF;
    // apply Speck48/96 with m as key and p as input
    speck48_96(m, p, c);
    // combine c[0] and c[1] into a single 64-bit word
    uint64_t result = ((uint64_t)c[1] << 24) | c[0];
    
    return result ^ h;
}

int test_cs48_dm(void) {
    // Initialization vector and message to test
    uint64_t iv = IV;
    uint32_t message[4] = {0, 1, 2, 3};
    
    // Expected result after applying cs48_dm
    uint64_t expected_output = 0x5DFD97183F91ULL;
    
    // Call cs48_dm and get the result
    uint64_t output = cs48_dm(message, iv);
    
    // Check if output matches the expected result
    if (output == expected_output) {
        printf("Test passed: output = 0x%012llX\n", output);
        return 1; // Success
    } else {
        printf("Test failed: output = 0x%012llX, expected = 0x%012llX\n", output, expected_output);
        return 0; // Failure
    }
}


/* Assumes message length is fourlen * four blocks of 24 bits, each stored as the low bits of 32-bit words
 * fourlen is stored on 48 bits (as the 48 low bits of a 64-bit word)
 * when padding is included, simply adds one block (96 bits) of padding with fourlen and zeros on higher bits
 * (The fourlen * four blocks must be considered “full”, otherwise trivial collisions are possible)
 */
uint64_t hs48(const uint32_t *m, uint64_t fourlen, int padding, int verbose)
{
	uint64_t h = IV;
	const uint32_t *mp = m;

	for (uint64_t i = 0; i < fourlen; i++)
	{
		h = cs48_dm(mp, h);
		if (verbose){
			printf("@%llu : %06X %06X %06X %06X => %06llX\n", i, mp[0], mp[1], mp[2], mp[3], h);
            // usefule for the attack
            store_hash(h, mp);
        }
		mp += 4;
	}
	if (padding)
	{
		uint32_t pad[4];
		pad[0] = fourlen & 0xFFFFFF;
		pad[1] = (fourlen >> 24) & 0xFFFFFF;
		pad[2] = 0;
		pad[3] = 0;
		h = cs48_dm(pad, h);
		if (verbose)
			printf("@%llu : %06X %06X %06X %06X => %06llX\n", fourlen, pad[0], pad[1], pad[2], pad[3], h);
	}

	return h;
}

/* Computes the unique fixed-point for cs48_dm for the message m */
uint64_t get_cs48_dm_fp(uint32_t m[4])
{
	uint32_t c[2] = {0, 0};  // Ciphertext that we want to decrypt to find the fixed point
    uint32_t p[2];
    // we use the inverse to find the plaintext that maps to ciphertext {0, 0}
    speck48_96_inv(m, c, p);
    //combine p[0] and p[1] into a 48-bit result in a 64-bit variable
    uint64_t fp = ((uint64_t)p[1] << 24) | p[0];
    return fp;
}

// test function for get_cs48_dm_fp
int test_cs48_dm_fp(void) {
    uint32_t message[4] = {0, 1, 2, 3};
    // get the fixed point
    uint64_t fp = get_cs48_dm_fp(message);
    // verify that cs48_dm(m, fp) == fp
    if (cs48_dm(message, fp) == fp) {
        printf("Test passed: Fixed point fp = 0x%012llX\n", fp);
        return 1; // Success
    } else {
        printf("Test failed: cs48_dm(m, fp) != fp\n");
        return 0; // Failure
    }
}

/* Finds a two-block expandable message for hs48, using a fixed-point
 * That is, computes m1, m2 s.t. hs48_nopad(m1||m2) = hs48_nopad(m1||m2^*),
 * where hs48_nopad is hs48 with no padding */
void find_exp_mess(uint32_t m1[4], uint32_t m2[4])
{
	// initialize random generator
	uint64_t seed[4]; // = { 12345689, 392436069, 520088629, 88005123 }; 
	uint64_t t = (uint64_t)time(NULL);
    seed[0] = t ^ (t << 13);
    seed[1] = t ^ (t >> 7);
    seed[2] = t ^ (t << 17);
    seed[3] = t ^ (t >> 31);
    
    xoshiro256starstar_random_set(seed);
    
    // step 1: Populate hash table with chaining values of m1 blocks
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
      uint32_t base_random = xoshiro256starstar_random();
        uint32_t m1_candidate[4] = {
            (base_random >> 0) & 0xFFFFFF,
            (base_random >> 8) & 0xFFFFFF,
            (base_random >> 16) & 0xFFFFFF,
            (base_random >> 24) & 0xFFFFFF
    };
        uint64_t h = cs48_dm(m1_candidate, IV); // Assuming IV is defined elsewhere
        store_hash(h, m1_candidate);
    }
    // step 2: Find a matching m2 block
    while (1) {
        uint32_t m2_candidate[4] = {xoshiro256starstar_random() & 0xFFFFFF, xoshiro256starstar_random() & 0xFFFFFF,
                                    xoshiro256starstar_random() & 0xFFFFFF, xoshiro256starstar_random() & 0xFFFFFF};
        uint64_t h_fp = get_cs48_dm_fp(m2_candidate);

        // check if this h_fp exists in our hash table
        const uint32_t *matched_m1 = retrieve_hash(h_fp);
        if (matched_m1) {
            // Found a match, copy the values to m1 and m2
            memcpy(m1, matched_m1, 4 * sizeof(uint32_t));
            memcpy(m2, m2_candidate, 4 * sizeof(uint32_t));
            clear_hash_table();
            return;
        }
    }
}

// test function for find_exp_mess
int test_em(void) {
    uint32_t m1[4], m2[4];
    find_exp_mess(m1, m2);

    // calculate h from cs48_dm(m1, IV) and get fixed point for m2
    uint64_t h1 = cs48_dm(m1, IV);
    uint64_t h2 = get_cs48_dm_fp(m2);

    if (h1 == h2) {
        printf("Found expandable message.\n");
        printf("m1: %06X %06X %06X %06X\n", m1[0], m1[1], m1[2], m1[3]);
        printf("m2: %06X %06X %06X %06X\n", m2[0], m2[1], m2[2], m2[3]);
        return 1; // Success
    } else {
        printf("Test failed: No match between m1 and m2 chaining values.\n");
        return 0; // Failure
    }
}


void attack(void)
{   
    // make sure that hash table is empty
    clear_hash_table();
    // Step 1: Generate chaining values of `mess` that whose hash is equal to 0x7CA651E182DBULL.
    uint64_t l = 1 << 20;
    uint32_t mess[l]; // large enough for 2^18 blocks
    for (int i = 0; i < (l); i+=4){
        mess[i + 0] = i;
        mess[i + 1] = 0;
        mess[i + 2] = 0;
        mess[i + 3] = 0;
    }
    // put (hi, mi) inside the hash table
    hs48(mess, l >>2, 1, 1);

    printf("waiting...\n");
    // Step 2: Find expandable message
    uint32_t m1[4], m2[4];
    uint64_t h_fp ;
    bool b = true;
    uint64_t seed[4] = { 12345689, 392436069, 520088629, 88005123 }; 
    xoshiro256starstar_random_set(seed);
    while (b) {
        uint32_t m2_candidate[4] = {xoshiro256starstar_random() & 0xFFFFFF, xoshiro256starstar_random() & 0xFFFFFF,
                                    xoshiro256starstar_random() & 0xFFFFFF, xoshiro256starstar_random() & 0xFFFFFF};
        h_fp = get_cs48_dm_fp(m2_candidate);

        // check if this h_fp exists in our hash table
        const uint32_t *matched_m1 = retrieve_hash(h_fp);
        if (matched_m1) {
            // Found a match, copy the values to m1 and m2
            memcpy(m1, matched_m1, 4 * sizeof(uint32_t));
            memcpy(m2, m2_candidate, 4 * sizeof(uint32_t));
            b = false;
            clear_hash_table();
        }
    }
    int i = m1[0];
    printf("Found expandable message.\n");
    printf("mess[%lu]: %06X %06X %06X %06X\n", m1[0], m1[0], m1[1], m1[2], m1[3]);
    printf("m2: %06X %06X %06X %06X\n", m2[0], m2[1], m2[2], m2[3]);
    printf("Test  expandable message.\n");
    int new_l = l - i;
    int mess2[new_l];
    mess2[0] = m2[0]; mess2[1] = m2[1]; mess2[2] = m2[2]; mess2[3] = m2[3];
    for (int j = 4; j < (new_l); j+=4){
        mess2[j + 0] = j + i;
        mess2[j + 1] = 0;
        mess2[j + 2] = 0;
        mess2[j + 3] = 0;
    }

    uint64_t h_m1 = hs48(mess, l >>2, 1, 1);
    printf("hash mess 0 :=> %06llX\n", h_m1);
    IV = h_fp;
    uint64_t h_m2 = hs48(mess2, new_l >>2, 0, 0);
    printf("hash mess 1 :=> %06llX\n", h_m2);
}

int main()
{
	//test_vector_okay();
	//test_cs48_dm();
	//test_cs48_dm_fp();
	//test_em();
	attack();

	return 0;
}
