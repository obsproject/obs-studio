use anyhow::Result;

use std::collections::HashMap;

use serde_derive::{Deserialize, Serialize};

use webrtc::peer_connection::sdp::session_description::RTCSessionDescription;

#[derive(Deserialize, Serialize)]
struct WhipBody {
    desc: String,
}

pub async fn do_whip(local_desc: RTCSessionDescription) -> Result<RTCSessionDescription> {
    let json_str = serde_json::to_string(&local_desc)?;
    let b64 = base64::encode(&json_str);
    let mut map = HashMap::new();
    map.insert("desc", b64);

    let client = reqwest::Client::new();
    let res = client
        .post("http://10.42.42.153:8000/whip")
        .json(&map)
        .send()
        .await?;

    let body = res.json::<WhipBody>().await?;

    let sdp = base64::decode(body.desc)?;

    let sdp: RTCSessionDescription = serde_json::from_slice(&sdp)?;
    return Ok(sdp);
}
