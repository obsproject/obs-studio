#include <rtc/rtc.hpp>
#include <iostream>
#include <stdexcept>
// Exercise the installed, patched libdatachannel, including WHIP's MID/RID extensions.
// Synthetic NAL/OBU payloads exercise fragmentation, not codec decoding.
using namespace rtc;
int main()
{
	for (const std::string codec : {"H264", "H265", "AV1"}) {
		size_t maximum = 0, count = 0;
		for (size_t length : {1, 1198, 1199, 1200, 1201, 2400, 65536, 1048576}) {
			auto config = std::make_shared<RtpPacketizationConfig>(123, "whip", 96, 90000);
			config->midId = 1;
			config->mid = "1";
			config->ridId = 2;
			config->rid = "0";
			std::unique_ptr<RtpPacketizer> packetizer;
			binary frame;
			if (codec == "H264") {
				packetizer = std::make_unique<H264RtpPacketizer>(NalUnit::Separator::StartSequence,
										 config, WHIP_FRAGMENT_LIMIT);
				frame = {byte(0), byte(0), byte(0), byte(1), byte(0x65)};
			} else if (codec == "H265") {
				packetizer = std::make_unique<H265RtpPacketizer>(NalUnit::Separator::StartSequence,
										 config, WHIP_FRAGMENT_LIMIT);
				frame = {byte(0), byte(0), byte(0), byte(1), byte(0x26), byte(1)};
			} else {
				packetizer = std::make_unique<AV1RtpPacketizer>(
					AV1RtpPacketizer::Packetization::TemporalUnit, config, WHIP_FRAGMENT_LIMIT);
				frame = {byte(0x32)};
				size_t n = length;
				do {
					auto v = n & 127;
					n >>= 7;
					frame.push_back(byte(v | (n ? 128 : 0)));
				} while (n);
			}
			frame.insert(frame.end(), length, byte(0xaa));
			message_vector packets = {make_message(std::move(frame))};
			packetizer->outgoing(packets, [](message_ptr) {});
			if (packets.empty()) {
				throw std::runtime_error("No packets");
			}
			for (const auto &packet : packets) {
				maximum = std::max(maximum, packet->size());
				count++;
				if (packet->size() + 16 + 8 + 40 + 4 > 1400) {
					throw std::runtime_error("Exceeded 1400-byte IPv6/SRTP/TURN-channel budget");
				}
			}
		}
		std::cout << codec << ": " << count << " packets, maximum RTP " << maximum
			  << ", IPv6+UDP+SRTP(16)+TURN channel(4) " << maximum + 68 << " bytes\n";
	}
	// RFC 6716: a single frame can contain 1275 bytes, plus its one-byte TOC.
	auto config = std::make_shared<RtpPacketizationConfig>(124, "whip", 111, 48000);
	OpusRtpPacketizer opus(config);
	message_vector packets = {make_message(binary(1276, byte(0xaa)))};
	opus.outgoing(packets, [](message_ptr) {});
	if (packets.size() != 1 || packets.front()->size() + 68 > 1400) {
		throw std::runtime_error("Exceeded stereo Opus packet budget");
	}
	std::cout << "Opus: maximum single-frame RTP " << packets.front()->size()
		  << ", IPv6+UDP+SRTP(16)+TURN channel(4) " << packets.front()->size() + 68 << " bytes\n";
}
