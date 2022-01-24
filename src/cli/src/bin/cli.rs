use clap::{Parser, Subcommand};
use librevault_util::secret::Secret;
use std::path::PathBuf;
use url::Url;

use librevault_core::proto::controller::{controller_client::ControllerClient, ReindexPathRequest};

#[derive(Parser)]
struct Cli {
    #[clap(short, long, default_value = "http://[::1]:42346")]
    connect: Url,

    #[clap(subcommand)]
    subcmd: SubCommand,
}

#[derive(Subcommand)]
enum SubCommand {
    GenerateSecret,
    ReindexPath { bucket_id: String, path: PathBuf },
}

#[tokio::main]
async fn main() {
    let opts = Cli::parse();

    match opts.subcmd {
        SubCommand::GenerateSecret => {
            println!("{}", String::from(Secret::new()));
        }
        _ => {}
    }

    let mut client = ControllerClient::connect(opts.connect.to_string())
        .await
        .unwrap();

    match opts.subcmd {
        SubCommand::ReindexPath { bucket_id, path } => {
            let request = tonic::Request::new(ReindexPathRequest {
                bucket_id: hex::decode(bucket_id).unwrap(),
                path: String::from(path.to_str().unwrap()),
            });

            let response = client.reindex_path(request).await.unwrap();
            println!("RESPONSE={:?}", response);
        }
        _ => {}
    }
}
