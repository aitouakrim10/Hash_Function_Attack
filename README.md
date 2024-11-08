
# Second Preimage Attack on hs48 Hash Function

This project implements a generic second preimage attack for long messages on a toy hash function called hs48, which is based on the Speck48/96 block cipher using a Davies-Meyer construction.

## Overview

The attack exploits the Merkle-Damgård structure of the hash function to find a second preimage for a given long message. It consists of two main parts:

1. Finding an expandable message
2. Finding a collision with an intermediate chaining value of the target message

## Requirements

- C compiler (GCC recommended)
- xoshiro256** PRNG (header file provided)

## Compilation

Compile the code with optimizations enabled:

```
make
```

## Usage

Run the compiled program:

```
make run
```

The program will attempt to find a second preimage for a predefined message of 2^18 blocks, whose hash is 0x7CA651E182DBULL.

## Implementation Details

### Expandable Message (find_exp_mess function)

- Uses a meet-in-the-middle approach
- Generates N random first-block messages and their chaining values
- Computes fixed-points for random second-block messages until a collision is found
- Returns two one-block messages m1 and m2 that form the expandable message

### Full Attack (attack function)

1. Generates the original message (2^18 blocks)
2. Finds an expandable message using find_exp_mess
3. Searches for a collision block that connects the expandable message to an intermediate chaining value of the original message
4. Constructs the second preimage by:
   - Expanding the expandable message to the appropriate length
   - Adding the collision block
   - Copying the remaining blocks from the original message
5. Computes and verifies the hash of the second preimage

## Performance

- Finding an expandable message should take less than one minute on average
- The full attack should complete in less than ten minutes on average with optimizations enabled

## Output

The program provides verbose output detailing:
- The expandable message found
- The collision point and block
- The final hash of the second preimage

## Notes

- This implementation is for educational purposes and demonstrates the attack on a toy hash function
- The hs48 hash function is intentionally weak (48-bit output) to make the attack feasible
- Real-world hash functions use various techniques to prevent such attacks

## References

- [SIMON and SPECK Families of Lightweight Block Ciphers](http://eprint.iacr.org/2013/404)
- Kelsey and Schneier, "Second Preimages on n-Bit Hash Functions for Much Less than 2^n Work", EUROCRYPT 2005
