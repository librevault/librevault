use crate::WINSIZE;
use cached::proc_macro::cached;

#[derive(Clone)]
pub(crate) struct Tables {
    pub(crate) mod_table: [u64; 256],
    pub(crate) out_table: [u64; 256],
}

#[cached]
pub(crate) fn calc_tables(polynomial: u64) -> Tables {
    let mut tables = Tables {
        mod_table: [0; 256],
        out_table: [0; 256],
    };

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
        let mut hash = 0;

        hash = append_byte(hash, b, polynomial);
        for _i in 0..WINSIZE {
            hash = append_byte(hash, 0, polynomial);
        }
        tables.out_table[b as usize] = hash;
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

fn deg(p: u64) -> i8 {
    ((u64::BITS as i32) - 1 - (p.leading_zeros() as i32)) as i8
}

// Mod calculates the remainder of x divided by p.
fn xmod(mut x: u64, p: u64) -> u64 {
    while deg(x) >= deg(p) {
        let shift = deg(x) - deg(p);

        x ^= p << shift;
    }
    x
}

fn append_byte(mut hash: u64, b: u8, pol: u64) -> u64 {
    hash <<= 8;
    hash |= b as u64;

    xmod(hash, pol)
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
