use std::net::Ipv4Addr;
use std::str::FromStr;
use tokio::net::UdpSocket;

pub async fn discover_mcast() {
    let sock = UdpSocket::bind("0.0.0.0:28914").await.unwrap();
    sock.join_multicast_v4(Ipv4Addr::from_str("239.192.152.144").unwrap(), Ipv4Addr::from_str("0.0.0.0").unwrap()).unwrap();
}
