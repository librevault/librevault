use memoize::memoize;

pub const WINSIZE: usize = 64;

#[derive(Clone)]
struct Tables {
    mod_table: [u64; 256],
    out_table: [u64; 256],
}

#[memoize]
fn calc_tables(polynomial: u64) -> Tables {
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

pub struct Rabin {
    tables: Tables,

    window: [u8; WINSIZE],
    wpos: usize,
    count: usize,
    pos: u64,
    start: u64,
    digest: u64,
    chunk_start: u64,
    chunk_length: usize,
    polynomial_shift: u64,
    minsize: usize,
    maxsize: usize,
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

impl Rabin {
    pub fn append(&mut self, b: u8) {
        let index: u8 = (self.digest >> self.polynomial_shift) as u8;
        self.digest <<= 8;
        self.digest |= b as u64;
        self.digest ^= self.tables.mod_table[index as usize];
    }

    pub fn slide(&mut self, b: u8) {
        let out: u8 = self.window[self.wpos];
        self.window[self.wpos] = b;
        self.wpos = (self.wpos + 1) % WINSIZE;
        self.digest ^= self.tables.out_table[out as usize];
        self.append(b);
    }

    pub fn reset(&mut self) {
        self.window.fill(0);
        self.digest = 0;
        self.wpos = 0;
        self.count = 0;
        self.digest = 0;
        self.append(1);
    }

    pub fn rabin_next_chunk(&mut self, b: u8) -> bool {
        self.slide(b);
        self.count += 1;
        self.pos += 1;

        if (self.count >= self.minsize && ((self.digest & self.mask) == 0))
            || self.count >= self.maxsize
        {
            self.chunk_start = self.start;
            self.chunk_length = self.count;

            let pos = self.pos;
            self.reset();
            self.start = pos;
            self.pos = pos;

            return true;
        }
        false
    }

    pub fn finalize(&mut self) -> bool {
        if self.count == 0 {
            self.chunk_start = 0;
            self.chunk_length = 0;
            return false;
        }

        self.chunk_start = self.start;
        self.chunk_length = self.count;

        true
    }
}

impl Default for Rabin {
    fn default() -> Self {
        let polynomial = 0x3DA3358B4DC173;
        let mut rabin = Rabin {
            tables: calc_tables(polynomial),
            window: [0; WINSIZE],
            wpos: 0,
            count: 0,
            pos: 0,
            start: 0,
            digest: 0,
            chunk_start: 0,
            chunk_length: 0,
            polynomial_shift: 53 - 8,
            minsize: 1024 * 1024,
            maxsize: 8 * 1024 * 1024,
            mask: (1 << 20) - 1,
        };
        rabin.reset();
        rabin
    }
}

impl Iterator for Rabin {
    type Item = Vec<u8>;

    fn next(&mut self) -> Option<Self::Item> {
        // let mut chunk = vec![];
        // chunk.reserve(self.maxsize);
        todo!()
    }
}
