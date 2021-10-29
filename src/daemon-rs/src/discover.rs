use bytes::BytesMut;
use log::debug;
use serde::{Deserialize, Serialize};
use std::net::Ipv4Addr;
use std::str::FromStr;
use std::sync::Arc;
use std::time::Duration;
use tokio::net::UdpSocket;

#[derive(Serialize, Deserialize, Debug)]
struct McastMessage {
    port: u16,
    peer_id: String,
    bucket_id: String,
}

pub async fn discover_mcast() {
    let sock = Arc::new(UdpSocket::bind("0.0.0.0:28914").await.unwrap());
    sock.join_multicast_v4(
        Ipv4Addr::from_str("239.192.152.144").unwrap(),
        Ipv4Addr::from_str("0.0.0.0").unwrap(),
    )
    .unwrap();

    let sock2 = sock.clone();

    tokio::spawn(async move {
        loop {
            let msg = McastMessage {
                port: 1234,
                peer_id: "asdasd".to_string(),
                bucket_id: "sdaasfv".to_string(),
            };
            debug!("Sending multicast message: {:?}", msg);
            let len = sock2
                .send_to(&*serde_json::to_vec(&msg).unwrap(), "239.192.152.144:28914")
                .await
                .unwrap();
            debug!("Sent {} bytes", len);
            tokio::time::sleep(Duration::from_secs(1)).await;
        }
    });

    loop {
        let mut buf = [0u8; 65535];
        let b = sock.recv(buf.as_mut()).await.unwrap();
        let mut msg = buf.to_vec();
        msg.truncate(b);
        let msg: McastMessage = serde_json::from_slice(msg.as_slice()).unwrap();
        debug!("Got message: {:?}", msg);
    }
}
