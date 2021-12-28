use clap::Parser;
use librevault_util::secret::Secret;
use std::path::PathBuf;
use url::Url;

pub mod controller {
    tonic::include_proto!("librevault.controller.v1"); // The string specified here must match the proto package name
}

use controller::controller_client::ControllerClient;
use controller::ReindexPathRequest;

#[derive(Parser)]
struct Opts {
    #[clap(short, long, default_value = "http://[::1]:42346")]
    connect: Url,

    #[clap(subcommand)]
    subcmd: SubCommand,
}

#[derive(Parser)]
enum SubCommand {
    GenerateSecret,
    ReindexPath(ReindexPath),
}

#[derive(Parser)]
struct ReindexPath {
    bucket_id: String,
    path: PathBuf,
}

#[tokio::main]
async fn main() {
    let opts = Opts::parse();

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
        SubCommand::ReindexPath(args) => {
            let request = tonic::Request::new(ReindexPathRequest {
                bucket_id: hex::decode(args.bucket_id).unwrap(),
                path: String::from(args.path.to_str().unwrap()),
            });

            let response = client.reindex_path(request).await.unwrap();
            println!("RESPONSE={:?}", response);
        }
        _ => {}
    }
}
