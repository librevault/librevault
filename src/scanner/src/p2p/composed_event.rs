// use crate::proto::protocol::v1::MeshlanMessage;
use libp2p::gossipsub::GossipsubEvent;
use libp2p::identify::Event as IdentifyEvent;
use libp2p::kad::KademliaEvent;
use libp2p::mdns::Event as MdnsEvent;
use libp2p::request_response::RequestResponseEvent;

#[derive(Debug)]
pub(crate) enum ComposedEvent {
    Mdns(MdnsEvent),
    Gossipsub(GossipsubEvent),
    Identify(IdentifyEvent),
    Kad(KademliaEvent),
    // RequestResponse(RequestResponseEvent<MeshlanMessage, MeshlanMessage>),
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

impl From<IdentifyEvent> for ComposedEvent {
    fn from(event: IdentifyEvent) -> Self {
        ComposedEvent::Identify(event)
    }
}

impl From<KademliaEvent> for ComposedEvent {
    fn from(event: KademliaEvent) -> Self {
        ComposedEvent::Kad(event)
    }
}

// impl From<RequestResponseEvent<MeshlanMessage, MeshlanMessage>> for ComposedEvent {
//     fn from(event: RequestResponseEvent<MeshlanMessage, MeshlanMessage>) -> Self {
//         ComposedEvent::RequestResponse(event)
//     }
// }
