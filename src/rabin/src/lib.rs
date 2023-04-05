use std::io;
use std::io::{Bytes, Read};

use tables::Tables;

mod tables;

pub const WINSIZE: usize = 64;

#[derive(Clone)]
pub struct RabinParams {
    pub polynomial: u64,
    pub polynomial_shift: u64,
    pub minsize: usize,
    pub maxsize: usize,
    pub mask: u64,
}

impl Default for RabinParams {
    fn default() -> Self {
        Self {
            polynomial: 0x3DA3358B4DC173,
            polynomial_shift: 53 - 8,
            minsize: 1024 * 1024,
            maxsize: 8 * 1024 * 1024,
            mask: (1 << 20) - 1,
        }
    }
}

struct Accumulator<'a> {
    params: &'a RabinParams,
    tables: &'a Tables,

    window: [u8; WINSIZE],
    wpos: usize,

    count: usize,
    digest: u64,
}

impl<'a> Accumulator<'a> {
    fn append(&mut self, b: u8) {
        let index: u8 = (self.digest >> self.params.polynomial_shift) as u8;
        self.digest <<= 8;
        self.digest |= b as u64;
        self.digest ^= self.tables.mod_table[index as usize];
    }

    fn slide(&mut self, b: u8) {
        let out: u8 = self.window[self.wpos];
        self.window[self.wpos] = b;
        self.wpos = (self.wpos + 1) % WINSIZE;
        self.digest ^= self.tables.out_table[out as usize];
        self.append(b);

        self.count += 1;
    }

    const fn is_consumed(&self) -> bool {
        (self.count >= self.params.minsize && ((self.digest & self.params.mask) == 0))
            || self.count >= self.params.maxsize
    }

    fn new(params: &'a RabinParams, tables: &'a Tables) -> Self {
        let mut rabin = Self {
            params,
            tables,
            window: [0; WINSIZE],
            wpos: 0,
            count: 0,
            digest: 0,
        };
        rabin.append(1);
        rabin
    }
}

pub struct Rabin<R> {
    params: RabinParams,
    tables: Tables,
    reader: Bytes<R>,
}

impl<R> Rabin<R>
where
    R: Read,
{
    pub fn new(reader: R, params: RabinParams) -> Self {
        let tables = tables::calc_tables(params.polynomial);
        Self {
            params,
            tables,
            reader: reader.bytes(),
        }
    }

    pub fn min_size(&self) -> usize {
        self.params.minsize
    }
}

impl<R> Iterator for Rabin<R>
where
    R: Read,
{
    type Item = io::Result<Vec<u8>>;

    fn next(&mut self) -> Option<Self::Item> {
        let mut accum = Accumulator::new(&self.params, &self.tables);

        let mut chunk = Vec::with_capacity(self.params.maxsize);

        for b in &mut self.reader {
            match b {
                Ok(b) => {
                    chunk.push(b);
                    accum.slide(b);
                    if accum.is_consumed() {
                        return Some(Ok(chunk));
                    }
                }
                Err(e) => {
                    return Some(Err(e));
                }
            }
        }

        if !chunk.is_empty() {
            Some(Ok(chunk))
        } else {
            None
        }
    }
}
