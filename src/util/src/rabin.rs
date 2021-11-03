pub const WINSIZE: usize = 64;

#[repr(C)]
pub struct Rabin {
    mod_table: [u64; 256],
    out_table: [u64; 256],

    window: [u8; WINSIZE],
    wpos: usize,
    count: u64,
    pos: u64,
    start: u64,
    digest: u64,
    chunk_start: u64,
    chunk_length: u64,
    chunk_cut_fingerprint: u64,
    polynomial: u64,
    polynomial_degree: u64,
    polynomial_shift: u64,
    average_bits: u64,
    minsize: u64,
    pub(crate) maxsize: u64,
    mask: u64,
}

fn deg(p: u64) -> i8 {
    let mut mask: u64 = 0x8000000000000000;

    for i in 0..64 {
        if (mask & p) > 0 {
            return 63 - i;
        }

        mask >>= 1;
    }

    -1
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

fn calc_tables(h: &mut Rabin) {
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
    for b in 0..255 {
        let mut hash = 0;

        hash = append_byte(hash, b, h.polynomial);
        for _i in 0..WINSIZE - 1 {
            hash = append_byte(hash, 0, h.polynomial);
        }
        h.out_table[b as usize] = hash;
    }

    // calculate table for reduction mod Polynomial
    let k = deg(h.polynomial);
    for b in 0..255 {
        // mod_table[b] = A | B, where A = (b(x) * x^k mod pol) and  B = b(x) * x^k
        //
        // The 8 bits above deg(Polynomial) determine what happens next and so
        // these bits are used as a lookup to this table. The value is split in
        // two parts: Part A contains the result of the modulus operation, part
        // B is used to cancel out the 8 top bits so that one XOR operation is
        // enough to reduce modulo Polynomial
        h.mod_table[b as usize] = xmod((b as u64) << k, h.polynomial) | ((b as u64) << k);
    }
}

#[no_mangle]
pub extern "C" fn rabin_append(h: &mut Rabin, b: u8) {
    let index: u8 = (h.digest >> h.polynomial_shift) as u8;
    h.digest <<= 8;
    h.digest |= b as u64;
    h.digest ^= h.mod_table[index as usize];
}

#[no_mangle]
pub extern "C" fn rabin_slide(h: &mut Rabin, b: u8) {
    let out: u8 = h.window[h.wpos];
    h.window[h.wpos] = b;
    h.digest ^= h.out_table[out as usize];
    h.wpos = (h.wpos + 1) % WINSIZE;
    rabin_append(h, b);
}

#[no_mangle]
pub extern "C" fn rabin_reset(h: &mut Rabin) {
    for i in 0..WINSIZE - 1 {
        h.window[i] = 0
    }
    h.digest = 0;
    h.wpos = 0;
    h.count = 0;
    h.digest = 0;
    rabin_append(h, 1);
}

#[no_mangle]
pub extern "C" fn rabin_next_chunk(h: &mut Rabin, b: u8) -> bool {
    rabin_slide(h, b);
    h.count += 1;
    h.pos += 1;

    if (h.count >= h.minsize && ((h.digest & h.mask) == 0)) || h.count >= h.maxsize {
        h.chunk_start = h.start;
        h.chunk_length = h.count;
        h.chunk_cut_fingerprint = h.digest;

        let pos = h.pos;
        rabin_reset(h);
        h.start = pos;
        h.pos = pos;

        return true;
    }
    false
}

#[no_mangle]
pub extern "C" fn rabin_init(h: &mut Rabin) {
    calc_tables(h);
    h.pos = 0;
    h.start = 0;
    rabin_reset(h);
}

#[no_mangle]
pub extern "C" fn rabin_finalize(h: &mut Rabin) -> bool {
    if h.count == 0 {
        h.chunk_start = 0;
        h.chunk_length = 0;
        h.chunk_cut_fingerprint = 0;
        return false;
    }

    h.chunk_start = h.start;
    h.chunk_length = h.count;
    h.chunk_cut_fingerprint = h.digest;

    true
}

impl Default for Rabin {
    fn default() -> Self {
        Rabin {
            mod_table: [0; 256],
            out_table: [0; 256],
            window: [0; WINSIZE],
            wpos: 0,
            count: 0,
            pos: 0,
            start: 0,
            digest: 0,
            chunk_start: 0,
            chunk_length: 0,
            chunk_cut_fingerprint: 0,
            polynomial: 0x3DA3358B4DC173,
            polynomial_degree: 53,
            polynomial_shift: 53 - 8,
            average_bits: 20,
            minsize: 1024 * 1024,
            maxsize: 8 * 1024 * 1024,
            mask: (1 << 20) - 1,
        }
    }
}
