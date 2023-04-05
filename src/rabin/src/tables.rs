use crate::WINSIZE;
use cached::proc_macro::cached;
use core::hash::Hasher;

#[derive(Clone)]
pub(crate) struct Tables {
    pub(crate) mod_table: [u64; 256],
    pub(crate) out_table: [u64; 256],
}

impl Default for Tables {
    fn default() -> Self {
        Self {
            mod_table: [0; 256],
            out_table: [0; 256],
        }
    }
}

struct RabinHash {
    state: u64,
    polynomial: u64,
}

impl Hasher for RabinHash {
    #[inline]
    fn finish(&self) -> u64 {
        self.state
    }

    fn write(&mut self, bytes: &[u8]) {
        for &b in bytes {
            self.write_u8(b)
        }
    }

    fn write_u8(&mut self, i: u8) {
        self.state <<= 8;
        self.state |= i as u64;

        self.state = xmod(self.state, self.polynomial)
    }
}

impl RabinHash {
    pub const fn new(polynomial: u64) -> Self {
        Self {
            state: 0,
            polynomial,
        }
    }
}

#[cached]
pub(crate) fn calc_tables(polynomial: u64) -> Tables {
    let mut tables = Tables::default();

    // calculate table for sliding out bytes. The byte to slide out is used as
    // the index for the table, the value contains the following:
    // out_table[b] = Hash(b || 0 ||        ...        || 0)
    //                          \ windowsize-1 zero bytes /
    // To slide out byte b_0 for window size w with known hash
    // H := H(b_0 || ... || b_w), it is sufficient to add out_table[b_0]:
    //    H(b_0 || ... || b_w) + H(b_0 || 0 || ... || 0)
    //  = H(b_0 + b_0 || b_1 + 0 || ... || b_w + 0)
    //  = H(    0     || b_1 || ...     || b_w)
    //
    // Afterwards a new byte can be shifted in.
    for b in 0..=255 {
        let mut hash = RabinHash::new(polynomial);
        hash.write_u8(b);
        hash.write(&[0; WINSIZE - 1]);
        tables.out_table[b as usize] = hash.finish();
    }

    // calculate table for reduction mod Polynomial
    let k = deg(polynomial);
    for b in 0..=255u64 {
        // mod_table[b] = A | B, where A = (b(x) * x^k mod pol) and  B = b(x) * x^k
        //
        // The 8 bits above deg(Polynomial) determine what happens next and so
        // these bits are used as a lookup to this table. The value is split in
        // two parts: Part A contains the result of the modulus operation, part
        // B is used to cancel out the 8 top bits so that one XOR operation is
        // enough to reduce modulo Polynomial
        tables.mod_table[b as usize] = xmod(b << k, polynomial) | (b << k);
    }

    tables
}

const fn deg(p: u64) -> i8 {
    ((u64::BITS as i32) - 1 - (p.leading_zeros() as i32)) as i8
}

// Mod calculates the remainder of x divided by p.
const fn xmod(mut x: u64, p: u64) -> u64 {
    while deg(x) >= deg(p) {
        let shift = deg(x) - deg(p);

        x ^= p << shift;
    }
    x
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_deg() {
        fn old_deg(p: u64) -> i8 {
            let mut mask: u64 = 0x8000000000000000;

            for i in 0..64 {
                if (mask & p) > 0 {
                    return 63 - i;
                }

                mask >>= 1;
            }

            -1
        }

        assert_eq!(old_deg(0), deg(0));
        assert_eq!(old_deg(0x6a2c48091e6a2c48), deg(0x6a2c48091e6a2c48));
        assert_eq!(old_deg(0x8000000000000000), deg(0x8000000000000000));
        assert_eq!(old_deg(0x3DA3358B4DC173), deg(0x3DA3358B4DC173));
        assert_eq!(old_deg(u64::MAX), deg(u64::MAX));
    }
}
