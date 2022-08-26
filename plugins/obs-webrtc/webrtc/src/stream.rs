use anyhow::Result;

use std::boxed::Box;
use std::os::raw::c_char;
use std::slice;
use std::sync::Arc;

use bytes::Bytes;

use tokio::runtime::Runtime;
use tokio::sync::Notify;

use webrtc::api::interceptor_registry::register_default_interceptors;
use webrtc::api::media_engine::{MediaEngine, MIME_TYPE_H264, MIME_TYPE_OPUS};
use webrtc::api::APIBuilder;
use webrtc::ice_transport::ice_connection_state::RTCIceConnectionState;
use webrtc::ice_transport::ice_server::RTCIceServer;
use webrtc::interceptor::registry::Registry;
use webrtc::media::Sample;
use webrtc::peer_connection::configuration::RTCConfiguration;
use webrtc::peer_connection::peer_connection_state::RTCPeerConnectionState;
use webrtc::rtp_transceiver::rtp_codec::RTCRtpCodecCapability;
use webrtc::track::track_local::track_local_static_sample::TrackLocalStaticSample;
use webrtc::track::track_local::TrackLocal;
use webrtc::Error;

use crate::whip;

pub struct OBSWebRTCStream {
    runtime: Runtime,
    video_track: Arc<TrackLocalStaticSample>,
    audio_track: Arc<TrackLocalStaticSample>,
}

async fn connect(
    video_track: Arc<dyn TrackLocal + Send + Sync>,
    audio_track: Arc<dyn TrackLocal + Send + Sync>,
) -> Result<()> {
    println!("Setting up webrtc!");

    let mut m = MediaEngine::default();

    m.register_default_codecs();

    // Create a InterceptorRegistry. This is the user configurable RTP/RTCP Pipeline.
    // This provides NACKs, RTCP Reports and other features. If you use `webrtc.NewPeerConnection`
    // this is enabled by default. If you are manually managing You MUST create a InterceptorRegistry
    // for each PeerConnection.
    let mut registry = Registry::new();

    // Use the default set of Interceptors
    registry = register_default_interceptors(registry, &mut m).await?;

    // Create the API object with the MediaEngine
    let api = APIBuilder::new()
        .with_media_engine(m)
        .with_interceptor_registry(registry)
        .build();

    // Prepare the configuration
    let config = RTCConfiguration {
        ice_servers: vec![RTCIceServer {
            urls: vec!["stun:stun.l.google.com:19302".to_owned()],
            ..Default::default()
        }],
        ..Default::default()
    };

    // Create a new RTCPeerConnection
    let peer_connection = Arc::new(api.new_peer_connection(config).await?);

    let notify_tx = Arc::new(Notify::new());
    let notify_video = notify_tx.clone();
    let notify_audio = notify_tx.clone();

    let (done_tx, mut done_rx) = tokio::sync::mpsc::channel::<()>(1);
    let video_done_tx = done_tx.clone();
    let audio_done_tx = done_tx.clone();

    // Add this newly created track to the PeerConnection
    let rtp_sender = peer_connection
        .add_track(Arc::clone(&video_track) as Arc<dyn TrackLocal + Send + Sync>)
        .await?;

    // Read incoming RTCP packets
    // Before these packets are returned they are processed by interceptors. For things
    // like NACK this needs to be called.
    tokio::spawn(async move {
        let mut rtcp_buf = vec![0u8; 1500];
        while let Ok((_, _)) = rtp_sender.read(&mut rtcp_buf).await {}
        Result::<()>::Ok(())
    });

    // Add this newly created track to the PeerConnection
    let rtp_sender = peer_connection
        .add_track(Arc::clone(&audio_track) as Arc<dyn TrackLocal + Send + Sync>)
        .await?;

    // Read incoming RTCP packets
    // Before these packets are returned they are processed by interceptors. For things
    // like NACK this needs to be called.
    tokio::spawn(async move {
        let mut rtcp_buf = vec![0u8; 1500];
        while let Ok((_, _)) = rtp_sender.read(&mut rtcp_buf).await {}
        Result::<()>::Ok(())
    });

    // Set the handler for ICE connection state
    // This will notify you when the peer has connected/disconnected
    peer_connection
        .on_ice_connection_state_change(Box::new(move |connection_state: RTCIceConnectionState| {
            println!("Connection State has changed {}", connection_state);
            if connection_state == RTCIceConnectionState::Connected {
                notify_tx.notify_waiters();
            }
            Box::pin(async {})
        }))
        .await;

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

#[no_mangle]
pub extern "C" fn obs_webrtc_stream_init(name: *const c_char) -> *mut OBSWebRTCStream {
    let name = unsafe { std::ffi::CStr::from_ptr(name).to_str().unwrap() };
    println!("Hello, {}! I'm Rust!", name);

    let video_track = Arc::new(TrackLocalStaticSample::new(
        RTCRtpCodecCapability {
            mime_type: MIME_TYPE_H264.to_owned(),
            clock_rate: 90000,
            sdp_fmtp_line: "profile-level-id=428014; max-fs=3600; max-mbps=108000; max-br=1400"
                .to_string(),
            //          "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f"
            //.to_string(),
            ..Default::default()
        },
        "video".to_owned(),
        "webrtc-rs".to_owned(),
    ));

    // Create a audio track
    let audio_track = Arc::new(TrackLocalStaticSample::new(
        RTCRtpCodecCapability {
            mime_type: MIME_TYPE_OPUS.to_owned(),
            ..Default::default()
        },
        "audio".to_owned(),
        "webrtc-rs".to_owned(),
    ));

    Box::into_raw(Box::new(OBSWebRTCStream {
        runtime: tokio::runtime::Runtime::new().unwrap(),
        video_track: video_track,
        audio_track: audio_track,
    }))
}

#[no_mangle]
pub extern "C" fn obs_webrtc_stream_connect(obsrtc: *mut OBSWebRTCStream) {
    let obs_webrtc = unsafe { &*obsrtc };
    let video_track = Arc::clone(&obs_webrtc.video_track) as Arc<dyn TrackLocal + Send + Sync>;
    let audio_track = Arc::clone(&obs_webrtc.audio_track) as Arc<dyn TrackLocal + Send + Sync>;
    unsafe {
        (*obsrtc).runtime.spawn(async {
            connect(video_track, audio_track).await;
        });
    }
}

#[no_mangle]
pub extern "C" fn obs_webrtc_stream_data(
    obsrtc: *mut OBSWebRTCStream,
    data: *const u8,
    size: usize,
    duration: u64,
) {
    if obsrtc.is_null() {
        return;
    }

    let slice: &[u8] = unsafe { slice::from_raw_parts(data, size) };

    let sample = Sample {
        data: Bytes::from(slice),
        duration: std::time::Duration::from_nanos(duration),
        ..Default::default()
    };

    unsafe {
        (*obsrtc).runtime.block_on(async {
            (*obsrtc).video_track.write_sample(&sample).await;
        });
    }
}

#[no_mangle]
pub extern "C" fn obs_webrtc_stream_audio(
    obsrtc: *mut OBSWebRTCStream,
    data: *const u8,
    size: usize,
    duration: u64,
) {
    if obsrtc.is_null() {
        return;
    }

    let slice: &[u8] = unsafe { slice::from_raw_parts(data, size) };

    let sample = Sample {
        data: Bytes::from(slice),
        duration: std::time::Duration::from_micros(duration),
        ..Default::default()
    };

    unsafe {
        (*obsrtc).runtime.block_on(async {
            (*obsrtc).audio_track.write_sample(&sample).await;
        });
    }
}
