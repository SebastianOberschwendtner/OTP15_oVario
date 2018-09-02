/*
 * md5.c
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */
#include "md5.h"
#include "Variables.h"

//*********** Constants for hash calculation **************
const unsigned long k[64] = {
  // k[i] := floor(abs(sin(i)) * (2 pow 32))
  // RLD should be sin(i + 1) but want compatibility
  3614090360UL, // k=0
  3905402710UL, // k=1
  606105819UL, // k=2
  3250441966UL, // k=3
  4118548399UL, // k=4
  1200080426UL, // k=5
  2821735955UL, // k=6
  4249261313UL, // k=7
  1770035416UL, // k=8
  2336552879UL, // k=9
  4294925233UL, // k=10
  2304563134UL, // k=11
  1804603682UL, // k=12
  4254626195UL, // k=13
  2792965006UL, // k=14
  1236535329UL, // k=15
  4129170786UL, // k=16
  3225465664UL, // k=17
  643717713UL, // k=18
  3921069994UL, // k=19
  3593408605UL, // k=20
  38016083UL, // k=21
  3634488961UL, // k=22
  3889429448UL, // k=23
  568446438UL, // k=24
  3275163606UL, // k=25
  4107603335UL, // k=26
  1163531501UL, // k=27
  2850285829UL, // k=28
  4243563512UL, // k=29
  1735328473UL, // k=30
  2368359562UL, // k=31
  4294588738UL, // k=32
  2272392833UL, // k=33
  1839030562UL, // k=34
  4259657740UL, // k=35
  2763975236UL, // k=36
  1272893353UL, // k=37
  4139469664UL, // k=38
  3200236656UL, // k=39
  681279174UL, // k=40
  3936430074UL, // k=41
  3572445317UL, // k=42
  76029189UL, // k=43
  3654602809UL, // k=44
  3873151461UL, // k=45
  530742520UL, // k=46
  3299628645UL, // k=47
  4096336452UL, // k=48
  1126891415UL, // k=49
  2878612391UL, // k=50
  4237533241UL, // k=51
  1700485571UL, // k=52
  2399980690UL, // k=53
  4293915773UL, // k=54
  2240044497UL, // k=55
  1873313359UL, // k=56
  4264355552UL, // k=57
  2734768916UL, // k=58
  1309151649UL, // k=59
  4149444226UL, // k=60
  3174756917UL, // k=61
  718787259UL, // k=62
  3951481745UL  // k=63
};

const unsigned long r[64] = {
  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

/*
 * initialize md5 hash with custom key
 */
void md5_initialize(MD5_T* hash,unsigned long* key)
{
	//Set initial message length to 0
	hash->message_length = 0;
	//Copy the input keys to the state of the hash
	sys_memcpy(hash->state,key,16);
	//Initialize the buffer with all 0s
	for(unsigned char count = 0; count<64; count++)
		hash->buff512[count] = 0;
};

/*
 * leftrotate input bits.
 */
unsigned long md5_leftrotate(unsigned long x, unsigned long c)
{
    return (x << c) | (x >> (32 - c));
};

/*
 * Append character to hash
 */
void md5_append_char(MD5_T* hash, unsigned char character)
{
	//Get position of buffer to be written to
	unsigned char position = (unsigned char)(hash->message_length % 64);
	//Write character
	hash->buff512[position] = character;
	//Increase message length
	hash->message_length++;
	//If buffer is full, calculate hash
	if(position == 63)
		md5_process512(hash);
}
/*
 * Calculate hash.
 * Code adapted from XCSOAR project.
 * Only works on little endian MCUs!
 * (STM32 is little endian)
 */
void md5_process512(MD5_T* hash)
{
  // assume exactly 512 bits

  // copy the 64 chars into the 16 longs
  unsigned long w[16];
  sys_memcpy(w, hash->buff512, 64);

  // Initialize hash value for this chunk:
  unsigned long a = hash->state[0], b = hash->state[1], c = hash->state[2], d = hash->state[3];

  // Main loop:
  for (int i = 0; i < 64; i++)
  {
    unsigned long f, g;
    if (i <= 15)
    {
      f = (b & c) | ((~b) & d);
      g = i;
    }
    else if (i <= 31)
    {
      f = (d & b) | ((~d) & c);
      g = (5 * i + 1) % 16;
    }
    else if (i <= 47)
    {
      f = b ^ c ^ d;
      g = (3 * i + 5) % 16;
    }
    else
    {
      f = c ^ (b | (~d));
      g = (7 * i) % 16;
    }

    unsigned long temp = d;
    d = c;
    c = b;
    b += md5_leftrotate((a + f + k[i] + w[g]), r[i]);
    a = temp;
  }

  // Add this chunk's hash to result so far:
  hash->state[0] += a;
  hash->state[1] += b;
  hash->state[2] += c;
  hash->state[3] += d;
};

