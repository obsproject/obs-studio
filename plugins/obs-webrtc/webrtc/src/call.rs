use anyhow::Result;

use std::sync::Mutex;
use std::{convert::TryInto, sync::Arc};
use std::{ops::Add, os::raw::c_char};

use tokio::runtime::Runtime;
use tokio::time::Duration;

use webrtc::api::interceptor_registry::register_default_interceptors;
use webrtc::api::media_engine::{MediaEngine, MIME_TYPE_H264, MIME_TYPE_OPUS};
use webrtc::api::APIBuilder;
use webrtc::ice_transport::ice_server::RTCIceServer;
use webrtc::interceptor::registry::Registry;
use webrtc::peer_connection::configuration::RTCConfiguration;
use webrtc::peer_connection::peer_connection_state::RTCPeerConnectionState;
use webrtc::rtcp::payload_feedbacks::picture_loss_indication::PictureLossIndication;
use webrtc::rtp::codecs::h264::H264Packet;
use webrtc::rtp::codecs::opus::OpusPacket;
use webrtc::rtp::packetizer::Depacketizer;
use webrtc::rtp_transceiver::rtp_codec::{
    RTCRtpCodecCapability, RTCRtpCodecParameters, RTPCodecType,
};
use webrtc::rtp_transceiver::rtp_receiver::RTCRtpReceiver;
use webrtc::track::track_remote::TrackRemote;

use std::os::raw::{c_uint, c_void};

use crate::whip;

pub type PacketCallback = unsafe extern "C" fn(CodecContext, *const u8, c_uint, c_uint);

pub struct OBSWebRTCCall {
    runtime: Runtime,
    context: CodecContext,
    cb: PacketCallback,
}
#[derive(Copy, Clone)]
pub struct CodecContext(*const c_void);

unsafe impl Send for CodecContext {}
unsafe impl Sync for CodecContext {}

impl OBSWebRTCCall {
    fn callback(&self, data: bytes::Bytes, video: u32) {
        let length = data.len().try_into().unwrap();
        unsafe {
            (self.cb)(self.context, data.as_ptr(), length, video);
        }
    }
}

#[no_mangle]
pub extern "C" fn obs_webrtc_call_init(
    context: *const c_void,
    cb: PacketCallback,
) -> *mut OBSWebRTCCall {
    Box::into_raw(Box::new(OBSWebRTCCall {
        runtime: tokio::runtime::Runtime::new().unwrap(),
        context: CodecContext(context),
        cb,
    }))
}

#[no_mangle]
pub extern "C" fn obs_webrtc_call_start(obsrtc: *mut OBSWebRTCCall) {
    unsafe {
        let obs = Box::from_raw(obsrtc);
        (*obsrtc).runtime.spawn(async {
            obs_webrtc_call_peer_connect(obs).await;
        });
    }
}

async fn obs_webrtc_call_peer_connect(obsrtc: Box<OBSWebRTCCall>) -> Result<()> {
    // Create a MediaEngine object to configure the supported codec
    let mut m = MediaEngine::default();

    // Setup the codecs you want to use.
    // We'll use a H264 and Opus but you can also define your own
    m.register_codec(
        RTCRtpCodecParameters {
            capability: RTCRtpCodecCapability {
                mime_type: MIME_TYPE_H264.to_owned(),
                clock_rate: 90000,
                channels: 0,
                sdp_fmtp_line:
                "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f"
                    .to_owned(),
                rtcp_feedback: vec![],
            },
            payload_type: 102,
            ..Default::default()
        },
        RTPCodecType::Video,
    )?;

    m.register_codec(
        RTCRtpCodecParameters {
            capability: RTCRtpCodecCapability {
                mime_type: MIME_TYPE_OPUS.to_owned(),
                clock_rate: 48000,
                channels: 2,
                sdp_fmtp_line: "".to_owned(),
                rtcp_feedback: vec![],
            },
            payload_type: 111,
            ..Default::default()
        },
        RTPCodecType::Audio,
    )?;

    // Create a InterceptorRegistry. This is the user configurable RTP/RTCP Pipeline.
    // This provides NACKs, RTCP Reports and other features. If you use `webrtc.NewPeerConnection`
    // this is enabled by default. If you are manually managing You MUST create a InterceptorRegistry
    // for each PeerConnection.
    let mut registry = Registry::new();

    // Use the default set of Interceptors
    registry = register_default_interceptors(registry, &mut m).await?;

    let api = APIBuilder::new()
        .with_media_engine(m)
        .with_interceptor_registry(registry)
        .build();

    let config = RTCConfiguration {
        ice_servers: vec![RTCIceServer {
            urls: vec!["stun:stun.l.google.com:19302".to_owned()],
            ..Default::default()
        }],
        ..Default::default()
    };

    // Create a new RTCPeerConnection
    let peer_connection = Arc::new(api.new_peer_connection(config).await?);

    // Allow us to receive 1 audio track, and 1 video track
    peer_connection
        .add_transceiver_from_kind(RTPCodecType::Audio, &[])
        .await?;
    peer_connection
        .add_transceiver_from_kind(RTPCodecType::Video, &[])
        .await?;

    let pc = Arc::downgrade(&peer_connection);

    let obsarc = Arc::new(obsrtc);

    peer_connection.on_track(Box::new(move |track: Option<Arc<TrackRemote>>, _receiver: Option<Arc<RTCRtpReceiver>>| {
        if let Some(track) = track {
            // Send a PLI on an interval so that the publisher is pushing a keyframe every rtcpPLIInterval
            let media_ssrc = track.ssrc();
            let pc2 = pc.clone();
            tokio::spawn(async move {
                let mut result = Result::<usize>::Ok(0);
                while result.is_ok() {
                    let timeout = tokio::time::sleep(Duration::from_secs(3));
                    tokio::pin!(timeout);

                    tokio::select! {
                            _ = timeout.as_mut() =>{
                                if let Some(pc) = pc2.upgrade(){
                                    result = pc.write_rtcp(&[Box::new(PictureLossIndication{
                                        sender_ssrc: 0,
                                        media_ssrc,
                                    })]).await.map_err(Into::into);
                                }else {
                                    break;
                                }
                            }
                        };
                }
            });


            let obsclone = obsarc.clone();
            //let notify_rx2 = Arc::clone(&notify_rx);

            Box::pin(async move {
                let codec = track.codec().await;
                let mime_type = codec.capability.mime_type.to_lowercase();
                if mime_type == MIME_TYPE_OPUS.to_lowercase() {
                    println!("Got Opus track, saving to disk as output.opus (48 kHz, 2 channels)");
                    tokio::spawn(async move {

                        while let Ok((rtp_packet, _)) = track.read_rtp().await {

                            let mut opus_packet = OpusPacket::default();
                            let payload = opus_packet.depacketize(&rtp_packet.payload);

                            match payload {
                                Ok(tmp) => obsclone.callback(tmp, 0),
                                Err(e) => println!("packet ERROR: {}", e),
                            }
                        }

                    });
                } else if mime_type == MIME_TYPE_H264.to_lowercase() {
                    println!("Got H264 track!");
                    tokio::spawn(async move {

                        let mut cached_packet = H264Packet::default();

                        let mut has_key_frame = false;

                        while let Ok((rtp_packet, _)) = track.read_rtp().await {
                            if rtp_packet.payload.is_empty() {
                                continue;
                            }


                            if !has_key_frame {
                                has_key_frame = is_key_frame(&rtp_packet.payload);
                                if !has_key_frame {
                                    continue;
                                }
                            }

                            let payload = cached_packet.depacketize(&rtp_packet.payload);
                            match payload {
                                Ok(tmp) => obsclone.callback(tmp, 1),
                                Err(e) => println!("packet ERROR: {}", e),
                            }
                        }
                    });
                }
            })
        }else {
            Box::pin(async {})
        }
    })).await;

    let (done_tx, mut done_rx) = tokio::sync::mpsc::channel::<()>(1);

    // Set the handler for Peer connection state
    // This will notify you when the peer has connected/disconnected
    peer_connection
        .on_peer_connection_state_change(Box::new(move |s: RTCPeerConnectionState| {
            println!("Peer Connection State has changed: {}", s);

            if s == RTCPeerConnectionState::Failed {
                // Wait until PeerConnection has had no network activity for 30 seconds or another failure. It may be reconnected using an ICE Restart.
                // Use webrtc.PeerConnectionStateDisconnected if you are interested in detecting faster timeout.
                // Note that the PeerConnection may come back from PeerConnectionStateDisconnected.
                println!("Peer Connection has gone to failed exiting");
                let _ = done_tx.try_send(());
            }

            Box::pin(async {})
        }))
        .await;

    // Create an offer to send to the browser
    let offer = peer_connection.create_offer(None).await?;

    // Create channel that is blocked until ICE Gathering is complete
    let mut gather_complete = peer_connection.gathering_complete_promise().await;

    // Sets the LocalDescription, and starts our UDP listeners
    peer_connection.set_local_description(offer).await?;

    // Block until ICE Gathering is complete, disabling trickle ICE
    // we do this because we only can exchange one signaling message
    // in a production application you should exchange ICE Candidates via OnICECandidate
    let _ = gather_complete.recv().await;

    // Output the answer in base64 so we can paste it in browser
    if let Some(local_desc) = peer_connection.local_description().await {
        let sdp = whip::do_whip(local_desc).await?;

        peer_connection.set_remote_description(sdp).await?;

        println!("Press ctrl-c to stop");
        tokio::select! {
            _ = done_rx.recv() => {
                println!("received done signal!");
            }
            _ = tokio::signal::ctrl_c() => {
                println!("");
            }
        };
    } else {
        println!("generate local_description failed!");
    }

    peer_connection.close().await?;

    Ok(())
}

const NALU_TTYPE_STAP_A: u32 = 24;
const NALU_TTYPE_SPS: u32 = 7;
const NALU_TYPE_BITMASK: u32 = 0x1F;

fn is_key_frame(data: &[u8]) -> bool {
    if data.len() < 4 {
        false
    } else {
        let word = u32::from_be_bytes([data[0], data[1], data[2], data[3]]);
        let nalu_type = (word >> 24) & NALU_TYPE_BITMASK;
        (nalu_type == NALU_TTYPE_STAP_A && (word & NALU_TYPE_BITMASK) == NALU_TTYPE_SPS)
            || (nalu_type == NALU_TTYPE_SPS)
    }
}
