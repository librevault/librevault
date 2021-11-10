use crate::bucket::manager::{BucketManager, BucketManagerEvent};
use crate::bucket::BucketEvent;
use crate::settings::ConfigManager;
use futures::StreamExt;
use libp2p::{
    core::{upgrade, Multiaddr},
    gossipsub::{
        Gossipsub, GossipsubConfigBuilder, GossipsubEvent, GossipsubMessage, IdentTopic as Topic,
        MessageAuthenticity, MessageId, ValidationMode,
    },
    identity,
    mdns::{Mdns, MdnsConfig, MdnsEvent},
    mplex,
    multiaddr::multiaddr,
    noise,
    swarm::{SwarmBuilder, SwarmEvent},
    tcp::TokioTcpConfig,
    yamux, NetworkBehaviour, PeerId, Transport,
};
use log::{debug, info, trace};
use prost::Message;
use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};
use std::net::IpAddr;
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::broadcast::Receiver;

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/librevault.protocol.v1.rs"));
}

enum ComposedEvent {
    Mdns(MdnsEvent),
    Gossipsub(GossipsubEvent),
}

impl From<MdnsEvent> for ComposedEvent {
    fn from(event: MdnsEvent) -> Self {
        ComposedEvent::Mdns(event)
    }
}

impl From<GossipsubEvent> for ComposedEvent {
    fn from(event: GossipsubEvent) -> Self {
        ComposedEvent::Gossipsub(event)
    }
}

// fn make_have_meta {
//
// }

pub async fn run_server(
    buckets: Arc<BucketManager>,
    config: Arc<ConfigManager>,
    mut receiver: Receiver<BucketManagerEvent>,
) {
    let id_keys = identity::Keypair::generate_ed25519();
    let peer_id = PeerId::from(id_keys.public());
    info!("Local peer id: {:?}", peer_id);

    let noise_keys = noise::Keypair::<noise::X25519Spec>::new()
        .into_authentic(&id_keys)
        .expect("Signing libp2p-noise static DH keypair failed.");

    let transport = TokioTcpConfig::new()
        .nodelay(true)
        .upgrade(upgrade::Version::V1)
        .authenticate(noise::NoiseConfig::xx(noise_keys).into_authenticated())
        .multiplex(upgrade::SelectUpgrade::new(
            yamux::YamuxConfig::default(),
            mplex::MplexConfig::default(),
        ))
        .boxed();

    #[derive(NetworkBehaviour)]
    #[behaviour(out_event = "ComposedEvent")]
    struct MyBehaviour {
        mdns: Mdns,
        gossipsub: Gossipsub,
    }

    let config_bind = &config.config().p2p.bind;
    let bound_multiaddrs: Vec<Multiaddr> = config_bind
        .iter()
        .map(|&sa| match sa.ip() {
            IpAddr::V4(a) => multiaddr!(Ip4(a), Tcp(sa.port())),
            IpAddr::V6(a) => multiaddr!(Ip6(a), Tcp(sa.port())),
        })
        .collect();

    let mut swarm = {
        let message_id_fn = |message: &GossipsubMessage| {
            let mut s = DefaultHasher::new();
            if let Some(peer_id) = message.source {
                s.write(&*peer_id.to_bytes());
            }
            s.write(&*message.data);
            MessageId::from(s.finish().to_string())
        };

        // Set a custom gossipsub
        let gossipsub_config = GossipsubConfigBuilder::default()
            .heartbeat_interval(Duration::from_secs(10)) // This is set to aid debugging by not cluttering the log space
            .validation_mode(ValidationMode::Strict) // This sets the kind of message validation. The default is Strict (enforce message signing)
            .message_id_fn(message_id_fn) // content-address messages. No two messages of the
            // same content will be propagated.
            .build()
            .expect("Valid config");
        // build a gossipsub network behaviour
        let gossipsub: Gossipsub =
            Gossipsub::new(MessageAuthenticity::Signed(id_keys), gossipsub_config)
                .expect("Correct configuration");

        let mdns = Mdns::new(MdnsConfig::default()).await.unwrap();
        let behaviour = MyBehaviour { mdns, gossipsub };
        SwarmBuilder::new(transport, behaviour, peer_id)
            .executor(Box::new(|fut| {
                tokio::spawn(fut);
            }))
            .build()
    };

    for bound_multiaddr in bound_multiaddrs {
        swarm.listen_on(bound_multiaddr).unwrap();
    }

    trace!("Everything before loop is completed");

    loop {
        tokio::select! {
            // Swarm events
            event = swarm.select_next_some() => {
                match event {
                    SwarmEvent::Behaviour(ComposedEvent::Mdns(MdnsEvent::Discovered(list))) => {
                        for (peer_id, multiaddr) in list {
                            debug!("Discovered peer using mDNS: {:?}, {:?}", peer_id, multiaddr);
                            swarm.dial_addr(multiaddr);
                        }
                    }
                    SwarmEvent::NewListenAddr { address, .. } => {println!("Listening on {:?}", address);},
                    _ => {}
                }
            },
            // Bucket events (useful for gossipsub)
            event = receiver.recv() => {
                info!("got event: {:?}", event);
                match event {
                    Ok(BucketManagerEvent::BucketAdded(bucket)) => {
                        let topic = Topic::new(bucket.get_id_hex());
                        swarm.behaviour_mut().gossipsub.subscribe(&topic).unwrap();
                    }
                    Ok(BucketManagerEvent::BucketRemoved(bucket)) => {
                        let topic = Topic::new(bucket.get_id_hex());
                        swarm.behaviour_mut().gossipsub.unsubscribe(&topic).unwrap();
                    }
                    Ok(BucketManagerEvent::BucketEvent {bucket, event}) => {
                        let topic = Topic::new(bucket.get_id_hex());
                        match event {
                            BucketEvent::MetaAdded {signed_meta} => {
                                let broadcast_msg = proto::HaveMeta {meta: signed_meta.meta, signature: signed_meta.signature};
                                swarm.behaviour_mut().gossipsub.publish(topic, broadcast_msg.encode_to_vec());
                            }
                        }
                    }
                    Err(_) => {}
                }
            }
        }
    }
}
