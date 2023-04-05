// use crate::node::p2p::codec::{MeshlanCodec, MeshlanProtocol};
// use crate::node::p2p::composed_event::ComposedEvent;
use crate::composed_event::ComposedEvent;
use libp2p::kad::KademliaConfig;
use libp2p::request_response::{ProtocolSupport, RequestResponse, RequestResponseConfig};
use libp2p::swarm::NetworkBehaviour;
use libp2p::{
    core::upgrade,
    gossipsub::{Gossipsub, GossipsubConfigBuilder, MessageAuthenticity, ValidationMode},
    identify::{Behaviour as Identify, Config as IdentifyConfig},
    identity::Keypair,
    kad::{store::MemoryStore, Kademlia},
    mdns::{tokio::Behaviour as TokioMdns, Config as MdnsConfig},
    mplex, noise,
    swarm::SwarmBuilder,
    tcp::{tokio::Transport as TokioTcpTransport, Config as GenTcpConfig},
    yamux, PeerId, Swarm, Transport,
};
use std::time::Duration;
use tracing::info;

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
        let gossipsub = {
            let gossipsub_config = GossipsubConfigBuilder::default()
                // .heartbeat_interval(Duration::from_secs(10)) // This is set to aid debugging by not cluttering the log space
                .validation_mode(ValidationMode::Strict) // This sets the kind of message validation. The default is Strict (enforce message signing)
                .build()
                .expect("invalid config");
            Gossipsub::new(
                MessageAuthenticity::Signed(id_keys.clone()),
                gossipsub_config,
            )
            .expect("incorrect configuration")
        };

        let mdns = TokioMdns::new(MdnsConfig::default()).expect("mdns initialization failed");

        let identify = Identify::new(IdentifyConfig::new(
            PROTOCOL_FAMILY.into(),
            id_keys.public(),
        ));

        let peer_id = PeerId::from(id_keys.public());
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

pub(crate) fn make_swarm(id_keys: Keypair) -> Swarm<NodeBehaviour> {
    let peer_id = PeerId::from(id_keys.public());
    info!("Local peer id: {:?}", peer_id);

    let noise_keys = noise::Keypair::<noise::X25519Spec>::new()
        .into_authentic(&id_keys)
        .expect("Signing libp2p-noise static DH keypair failed.");

    let transport_conf = GenTcpConfig::default().nodelay(true);
    let transport = TokioTcpTransport::new(transport_conf)
        .upgrade(upgrade::Version::V1)
        .authenticate(noise::NoiseConfig::xx(noise_keys).into_authenticated())
        .multiplex(upgrade::SelectUpgrade::new(
            yamux::YamuxConfig::default(),
            mplex::MplexConfig::default(),
        ))
        .boxed();

    let behaviour = NodeBehaviour::new(id_keys);

    SwarmBuilder::with_tokio_executor(transport, behaviour, peer_id).build()
}
