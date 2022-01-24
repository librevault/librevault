use clap::{Parser, Subcommand};
use common_rs::encryption::decrypt;
use common_rs::index::Index;
use common_rs::indexer::{make_revision, sign_revision};
use common_rs::meta_ext::{ObjectMetaExt, RevisionExt};
use common_rs::proto::meta;
use common_rs::proto::meta::{revision::EncryptedPart, ObjectKind, Revision, SignedRevision};
use librevault_util::secret::Secret;
use log::{debug, info, trace};
use prost::Message;
use std::env;
use std::path::Path;
use walkdir::WalkDir;

#[derive(Parser)]
struct Cli {
    #[clap(subcommand)]
    subcmd: SubCommand,
}

#[derive(Subcommand)]
enum SubCommand {
    GenerateSecret,
    MakeRevision {
        #[clap(long)]
        dry_run: bool,
        secret: String,
    },
    Init,
    List,
    RevisionInfo {
        revision_hash: String,
        secret: String,
    },
}

fn make_rev(root: &Path, secret: &Secret) -> meta::Revision {
    let mut entries = vec![];

    for entry in WalkDir::new(root).min_depth(1) {
        debug!("{:?}", entry);
        entries.push(entry.unwrap().into_path())
    }

    let paths: Vec<&Path> = entries.iter().map(|f| f.as_path()).collect();

    make_revision(paths.as_slice(), root, secret)
}

#[tokio::main]
async fn main() {
    env_logger::init();

    let root = env::current_dir().unwrap();

    let opts = Cli::parse();

    match opts.subcmd {
        SubCommand::GenerateSecret => {
            println!("{}", String::from(Secret::new()));
        }
        SubCommand::MakeRevision { dry_run, secret } => {
            let secret: Secret = secret.parse().unwrap();

            let rev = make_rev(root.as_path(), &secret);
            let rev_signed = sign_revision(&rev, &secret);
            let rev_size: Vec<u8> = rev.encode_to_vec();
            info!("Revision size: {}", rev_size.len());
            trace!("{rev:?}");

            if !dry_run {
                let index = Index::init(Index::default_index_file(root.as_path()).as_path());
                index.put_head_revision(&rev_signed);
            }
        }
        SubCommand::Init => {
            Index::init(Index::default_index_file(root.as_path()).as_path());
        }
        SubCommand::List => {
            let index = Index::init(Index::default_index_file(root.as_path()).as_path());
            index.print_info()
        }
        SubCommand::RevisionInfo {
            revision_hash,
            secret,
        } => {
            let secret: Secret = secret.parse().unwrap();

            let index = Index::init(Index::default_index_file(root.as_path()).as_path());
            let revision_s = index
                .get_revision_by_hash(&*hex::decode(&revision_hash).unwrap())
                .unwrap();

            let signed_rev = SignedRevision::decode(&*revision_s).unwrap();
            println!("Revision {}", revision_hash);

            let rev = Revision::decode(&*signed_rev.revision.to_vec()).unwrap();
            println!("Chunks: {}, Size: {}", rev.chunks.len(), rev.chunks_size());

            let encrypted =
                decrypt(&rev.encrypted.unwrap(), secret.get_symmetric_key().unwrap()).unwrap();

            let encrypted_revision = EncryptedPart::decode(&*encrypted).unwrap();
            println!("Objects: {}", encrypted_revision.objects.len());
            for object in encrypted_revision.objects {
                let chunk_count: usize = object.chunks_count();
                println!(
                    "Name: \"{}\", kind: {:?}, chunks: {chunk_count}",
                    String::from_utf8(object.name).unwrap(),
                    ObjectKind::from_i32(object.kind).unwrap()
                );
            }
        }
    }
}
