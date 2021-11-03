use std::path::Path;
use std::sync::Arc;
use tonic::{transport::Server, Request, Response, Status};

use crate::bucket::BucketManager;
use crate::settings::ConfigManager;
use log::debug;
use proto::controller_server::{Controller, ControllerServer};
use proto::ReindexPathRequest;

pub mod proto {
    tonic::include_proto!("librevault.controller.v1"); // The string specified here must match the proto package name
}

const FILE_DESCRIPTOR_SET: &[u8] = tonic::include_file_descriptor_set!("librevault.controller.v1");

pub struct LibrevaultController {
    buckets: Arc<BucketManager>,
}

#[tonic::async_trait]
impl Controller for LibrevaultController {
    async fn reindex_path(
        &self,
        request: Request<ReindexPathRequest>,
    ) -> Result<Response<()>, Status> {
        debug!("Got a request: {:?}", &request);

        let message = request.into_inner();
        let bucket = self.buckets.get_bucket_byid(&*message.bucket_id);

        match bucket {
            Some(bucket) => {
                tokio::spawn(async move {
                    bucket.index_path(Path::new(&message.path)).await;
                });
                Ok(Response::new(()))
            }
            None => Err(Status::not_found("Bucket not found")),
        }
    }
}

pub async fn run_grpc(buckets: Arc<BucketManager>, config: Arc<ConfigManager>) {
    let greeter = LibrevaultController { buckets };

    let reflection = tonic_reflection::server::Builder::configure()
        .register_encoded_file_descriptor_set(FILE_DESCRIPTOR_SET)
        .build()
        .unwrap();

    Server::builder()
        .add_service(ControllerServer::new(greeter))
        .add_service(reflection)
        .serve(config.config().controller.bind)
        .await
        .unwrap();
}
