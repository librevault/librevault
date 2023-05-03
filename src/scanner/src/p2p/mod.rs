mod composed_event;

// use crate::node::p2p::codec::{MeshlanCodec, MeshlanProtocol};
// use crate::node::p2p::composed_event::ComposedEvent;
use std::net::{Ipv4Addr, Ipv6Addr};
use std::sync::Arc;
use std::time::Duration;

use actix::{Actor, Addr, AsyncContext, Context, Handler, Message, WeakRecipient};
use composed_event::ComposedEvent;
use libp2p::futures::{Stream, StreamExt, TryStreamExt};
use libp2p::kad::KademliaConfig;
use libp2p::multiaddr::multiaddr;
use libp2p::request_response::{ProtocolSupport, RequestResponse, RequestResponseConfig};
use libp2p::swarm::{ConnectionHandler, NetworkBehaviour, SwarmEvent};
use libp2p::{
    core::upgrade,
    gossipsub::{
        Behaviour as Gossipsub, ConfigBuilder as GossipsubConfigBuilder, MessageAuthenticity,
        ValidationMode,
    },
    identify::{Behaviour as Identify, Config as IdentifyConfig},
    identity::Keypair,
    kad::{store::MemoryStore, Kademlia},
    mdns::{tokio::Behaviour as TokioMdns, Config as MdnsConfig},
    noise,
    swarm::SwarmBuilder,
    tcp::{tokio::Transport as TokioTcpTransport, Config as TcpConfig},
    yamux, Multiaddr, PeerId, Swarm, Transport,
};
use tracing::{info, trace, warn};

const PROTOCOL_FAMILY: &str = "meshlan/1.0.0";

#[derive(NetworkBehaviour)]
#[behaviour(out_event = "ComposedEvent")]
pub(crate) struct NodeBehaviour {
    pub(crate) mdns: TokioMdns,
    pub(crate) gossipsub: Gossipsub,
    pub(crate) identify: Identify,
    pub(crate) kad: Kademlia<MemoryStore>,
    // pub(crate) request_response: RequestResponse<MeshlanCodec>,
}

impl NodeBehaviour {
    fn new(id_keys: Keypair) -> Self {
        let public_key = id_keys.public();
        let peer_id = PeerId::from(public_key.clone());

        let gossipsub = {
            let gossipsub_config = GossipsubConfigBuilder::default()
                .heartbeat_interval(Duration::from_secs(60)) // This is set to aid debugging by not cluttering the log space
                .validation_mode(ValidationMode::Strict) // This sets the kind of message validation. The default is Strict (enforce message signing)
                .build()
                .expect("invalid config");
            Gossipsub::new(
                MessageAuthenticity::Signed(id_keys.clone()),
                gossipsub_config,
            )
            .expect("incorrect configuration")
        };

        let mdns =
            TokioMdns::new(MdnsConfig::default(), peer_id).expect("mdns initialization failed");

        let identify = Identify::new(IdentifyConfig::new(PROTOCOL_FAMILY.into(), public_key));

        let kad = {
            let mut config = KademliaConfig::default();
            config.set_provider_publication_interval(Some(Duration::from_secs(10)));
            Kademlia::with_config(peer_id, MemoryStore::new(peer_id), config)
        };

        // let request_response = RequestResponse::new(
        //     MeshlanCodec::default(),
        //     vec![(MeshlanProtocol {}, ProtocolSupport::Full)],
        //     RequestResponseConfig::default(),
        // );

        Self {
            mdns,
            gossipsub,
            identify,
            kad,
            // request_response,
        }
    }
}

#[derive(Debug, Message)]
#[rtype(result = "()")]
struct WrappedEvent(<Swarm<NodeBehaviour> as Stream>::Item);

pub struct P2PNetwork {
    swarm: Option<Swarm<NodeBehaviour>>,
}

impl Actor for P2PNetwork {
    type Context = Context<Self>;

    fn started(&mut self, ctx: &mut Context<Self>) {
        let mut swarm = self.swarm.take().unwrap();

        let bound_multiaddrs = vec![
            multiaddr!(Ip4(Ipv4Addr::UNSPECIFIED), Tcp(0u16)),
            multiaddr!(Ip6(Ipv6Addr::UNSPECIFIED), Tcp(0u16)),
        ];
        for bound_multiaddr in bound_multiaddrs {
            swarm.listen_on(bound_multiaddr).unwrap();
        }

        let myself = ctx.address();

        tokio::spawn(async move {
            loop {
                tokio::select! {
                    event = swarm.select_next_some() => {
                        myself.do_send(WrappedEvent(event));
                    }
                }
            }
        });
    }
}

impl P2PNetwork {
    pub fn new(id_keys: Keypair) -> Self {
        let peer_id = PeerId::from(id_keys.public());
        info!("Local peer id: {:?}", peer_id);

        let noise_keys = noise::Keypair::<noise::X25519Spec>::new()
            .into_authentic(&id_keys)
            .expect("Signing libp2p-noise static DH keypair failed.");

        let transport_conf = TcpConfig::default().nodelay(true);
        let transport = TokioTcpTransport::new(transport_conf)
            .upgrade(upgrade::Version::V1)
            .authenticate(noise::NoiseConfig::xx(noise_keys).into_authenticated())
            .multiplex(yamux::YamuxConfig::default())
            .boxed();

        let behaviour = NodeBehaviour::new(id_keys);

        let swarm = SwarmBuilder::with_tokio_executor(transport, behaviour, peer_id).build();

        Self { swarm: Some(swarm) }
    }
}

impl Handler<WrappedEvent> for P2PNetwork {
    type Result = ();

    fn handle(&mut self, msg: WrappedEvent, ctx: &mut Self::Context) -> Self::Result {
        let event = msg.0;
        warn!("{event:?}");
    }
}
