Librevault configuration
========================

Configuration entries:
----------------------

###device_name
- type: string
- multiple: N
- default: hostname of client device
- affected by: `anonymous_mode`

Defines name of device to be sent to other nodes, so other nodes could identify this node easily.

###net
- type: group
- multiple: N

Group of network properties.

###net.listen
- type: string
- multiple: N
- default: [::]:0

Defines host and port, on which the Librevault application will listen for incoming connections. 0 as port stands for random port.

###net.natpmp
- type: group
- multiple: N

Group of properties, related to NAT-PMP NAT traversal mechanism.

###net.natpmp.enabled
- type: bool
- multiple: N
- default: true

Enables NAT-PMP NAT traversal mechanism.

###net.natpmp.lifetime
- type: integer
- multiple: N
- default: 3600

Lifetime (in seconds) of port mapping.

###discovery
- type: group
- multiple: N

Group of properties, related to discovery of new nodes, participating in Librevault transmissions.

###discovery.multicast4/multicast6
- type: group
- multiple: N

Groups of properties, related to discovery using UDP multicast (somewhat similar to BitTorrent Local Peer Discovery).
`multicast4` is related to IPv4 discovery and `multicast6` is related to IPv6 discovery. `multicast6` discovery works only if `net`.`listen` is bound to IPv6 address.

###discovery.multicast4/multicast6.enabled
- type: bool
- multiple: N
- default: true
- `multicast6`.`enabled` affected by: `net`.`listen`

Enables multicast discovery. If `net`.`listen` is set to IPv4 IP address, then `multicast6`.`enabled` is forced to "false".

###discovery.multicast4/multicast6.local_ip
- type: string
- multiple: N
- default:  
if `net`.`listen` has IPv6 address, then `multicast4`.`local_ip` = '0.0.0.0'; `multicast6`.`local_ip` = host of `net`.`listen`.  
if `net`.`listen` has IPv4 address, then `multicast4`.`local_ip` = host of `net`.`listen`; `multicast6`.`local_ip` is undefined.
- affected by: `multicast4`/`multicast6`.`enabled`

Sets interface, from which UDP multicast datagrams will be sent.

###discovery.multicast4/multicast6.group
- type: string
- multiple: N
- default:  
`multicast4`.`group` = '239.192.152.144:28914'
`multicast6`.`group` = '[ff08::BD02]:28914'
- affected by: `multicast4`/`multicast6`.`enabled`

Specifies multicast group to connect to and receive multicast requests from.

###discovery.multicast4/multicast6.repeat_interval
- type: integer
- multiple: N
- default: 30
- affected by: `multicast4`/`multicast6`.`enabled`

Specifies interval in seconds between sending multicast requests.

###discovery.bttracker
- type: group
- multiple: N

Group of properties, related to discovery using BitTorrent trackers.

###discovery.bttracker.enabled
- type: bool
- multiple: N
- default: true

Enables using BitTorent trackers for discovering new peers.

###discovery.bttracker.udp
- type: group
- multiple: N

Group of properties, related to UDP BitTorrent trackers.

###discovery.bttracker.udp.reconnect_interval
- type: integer
- multiple: N
- default: 120

Specifies interval (in seconds) between sending new connection request to UDP tracker.

###discovery.bttracker.azureus_id
- type: string
- multiple: N
- default: "-LVxxxx-", where xxxx = Librevault version

This option can be used to mask Librevault client as any other torrent-client, that uses Azuerus-style encoding for peer_id.

###discovery.bttracker.min_interval
- type: integer
- multiple: N
- default: 15

Specifies, interval (in seconds) between announces to tracker. Overrides value, received from trackers (to prevent nuking misconfigured trackers).

###discovery.bttracker.tracker
- type: string
- multiple: Y

Specifies URLs of trackers to announce to. Supports UDP trackers using scheme "udp://".

###tor
####UNIMPLEMENTED
- type: group
- multiple: N

Group of properties, related to using Librevault through Tor (The Onion Router).

###tor.enabled
####UNIMPLEMENTED
- type: bool
- multiple: N
- default: false
- affected by: `anonymous_mode`

Enables Tor support in Librevault. Requires running tor daemon.

###UNIMPLEMENTED tor.tor_port
- type: integer
- multiple: N
- default: 9050

Tor daemon port.

###folder
- type: group
- multiple: Y

Group of properties, of a single folder, served by this instance of Librevault.

###folder.key
- type: string
- multiple: N
- default: error if not defined

Specifies Key of directory, served by client.

###folder.open_path
- type: string
- multiple: N
- default: error if not defined

Specifies directory path, where assembled files are supposed to be. This directory will be watched for changes.

###folder.ignore
- type: string
- multiple: Y

Specifies files and directories to skip during watching for changes/indexing/assembling. May contain * and ? wildcard characters.

###folder.node
- type: string
- multiple: Y
- format: ip:port

Explicitly defines nodes to connect for this folder.

###folder.index_event_timeout
- type: integer
- multiple: N
- default: 1000

Specifies timeout (in milliseconds) between receiving directory change and committing it.

###folder.preserve_symlinks
- type: bool
- multiple: N
- default: true

Specifies, whether to preserve symbolic links or not. Used only for indexing.

###folder.preserve_windows_attrib
- type: bool
- multiple: N
- default: false

**WARNING:** This is a special option for special cases; do not enable it unless you really know what you are doing.
*NOTE:*  This option must be enabled on both "indexing node" and "receiving mode" to operate correctly.

Specifies, whether to preserve Windows attributes (Read-Only/Hidden/System/Archive).

###folder.preserve_unix_attrib
- type: bool
- multiple: N
- default: false

**WARNING:** This is a special option for special cases; do not enable it unless you really know what you are doing.
*NOTE:*  This option must be enabled on both "indexing node" and "receiving mode" to operate correctly.

Specifies, whether to preserve Unix attributes stat() attributes (mode, uid, gid). May require root priveleges to run, because default uid/gid for clients without this option equals 0 (root).

###folder.block_strong_hash_type
- type: integer
- multiple: N
- default: 0

Specifies, which hasher to use while creating new FileMaps. It holds the number of hasher in decimal form according to Librevault Protocol specification.