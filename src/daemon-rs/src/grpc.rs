use tonic::{transport::Server, Request, Response, Status};

use controller::controller_server::{Controller, ControllerServer};
use controller::ReindexPathRequest;

pub mod controller {
    tonic::include_proto!("librevault.controller.v1"); // The string specified here must match the proto package name
}

const FILE_DESCRIPTOR_SET: &[u8] = tonic::include_file_descriptor_set!("librevault.controller.v1");

#[derive(Debug, Default)]
pub struct LibrevaultController {}

#[tonic::async_trait]
impl Controller for LibrevaultController {
    async fn reindex_path(
        &self,
        request: Request<ReindexPathRequest>,
    ) -> Result<Response<()>, Status> {
        // Return an instance of type HelloReply
        println!("Got a request: {:?}", request);

        Ok(Response::new(())) // Send back our formatted greeting
    }
}

pub async fn launch_grpc() {
    let addr = "[::1]:50051".parse().unwrap();
    let greeter = LibrevaultController::default();

    let reflection = tonic_reflection::server::Builder::configure()
        .register_encoded_file_descriptor_set(FILE_DESCRIPTOR_SET)
        .build()
        .unwrap();

    Server::builder()
        .add_service(ControllerServer::new(greeter))
        .add_service(reflection)
        .serve(addr)
        .await;
}
