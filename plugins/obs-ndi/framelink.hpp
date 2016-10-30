// FRAME LINK
// - High bandwidth zero-copy audio+video transport
//
// A frame link is a high-bandwidth in-memory a/v transport link utilizing
// shared memory. It provides a data plane for zero-copy processing pipelines.
// It is optimized for live streaming production pipelines and can be used as
// the backbone for data flowing through a media streaming processing pipeline.
// The protocol is purely concerned with data-plane transport of audio and
// video data and does not specify control-plane information.
//
// A frame link consists of a header, a number of stages, and a number of
// pre-allocated frame buffers. A stage is a circular buffer of pointers to
// frame buffers. The frames are preallocated buffers of a size determined by
// the maximum amount of data they will contain. Usually this will be 32 MB,
// which is enough for 4K video frames. The stage circular buffers have 128
// slots. All of this is contained in a single shared memory segment of 4 GB,
// which holds up to 125 32 MB buffers. No more than 128 buffers can be
// allocated, to ensure that insertion into the stages will never block.
//
// Stages act as queues of data to be processed. Workers can pull from and push
// to arbitrary stages, allowing data to flow through the system even when
// workers cannot call eachother directly, e.g. in a shared memory system.
//
// Frame links can support arbitrary data flow patterns. The zeroth stage, by
// convention, has a fixed function , providing unused frame buffers for all
// other workers, and all other stages contain data-carrying frames, but how
// stages are read from and written to is completely application specific. A
// basic pipeline, with one sink, two transformers, and one source, would look
// like this:
//
// +---------+---------+---------+-------+
// | Stage 0 | Stage 1 | Stage 2 | Stage 3
// +---------+---------+---------+-------+
// |       --|->     --|->     --|->     |
// +---------+---------+---------+-------+
//      ^                            v
//      +------- Free buffers -------+
//
// Each arrow represents a processing block of the pipeline and the stages act
// as storage locations for their output. The first arrow, from stage 0 to 1,
// is the source. It acquires an unused frame buffer, fills it with data, and
// saves the output in stage 1. A transformer then takes the buffers in stage
// 1, performs some transformation, and outputs to stage 2. Another transformer
// takes the stage 2 buffers, transform them, and save them to stage 3. Finally
// a sink takes the stage 3 buffers, perform some action, and when it is done
// releases the buffers back to stage 0.
//
// Where the actual processing resides is not dictated, or what order the data
// traverses the stages in. The frame_link data structure is completely thread
// safe once created, and so multiple workers can pull and push from/to each
// stage concurrently. Because the stages are dimensioned such that all frame
// buffers can be placed in each stage, writes to stages never block. Reads are
// blocking.
//
// Because the application has complete freedom in stage-to-stage routing,
// frame links can support arbitrarily complex processing graphs. Even cyclic
// graphs are supported. Multiple, unconnected, graphs can even be expressed
// with the appropriate stages.
//
// The frame link does not specify how the stages of the pipeline communicate
// configuration data, only a/v data.  Frame links provide no provision for
// setting up stage-to-stage routing. This must be done on a per-application
// basis.  To facilitate multiplexing, each frame has a tag value that stages
// can use. This tag is considered opaque to the frame link itself. It has a
// size of 64 bits and can thus hold a pointer on most modern platforms. If a
// pointer is used in the tag value, care must obviously be taken to ensure
// that it is only referenced from within the process that issued the pointer.
//
// An important thing to note is that frame link does not handle crashes. If a
// process holding a frame dies without releasing to another stage, the frame
// will never be reclaimed. In practice this is not a problem, since process
// crashes are expected to be rare, but to avoid the situation frames must
// always be released back to the pool.
//
// Likewise, workers should not hold on to frames for extended periods of time.
// This will starve other workers from getting frames.

#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
#include <vector>

namespace frame_link_detail {

	namespace bi = boost::interprocess;

	struct frame_video_header {

		enum video_format {
			VIDEO_FORMAT_UYVY = 0,
			VIDEO_FORMAT_BGRA = 1
		};

		uint32_t width, height; // Frame dimensions
		video_format format; // Frame color space
		int32_t fps_num, fps_denom; // Framerate as num / denom
		float aspect_ratio; // Picture aspect ratio
		uint32_t stride; // Inter-line stride, in bytes
		// Actual data in specified format follows, size = stride * height

		uint64_t data_size() {
			return this->stride * this->height;
		}

		uint64_t size() {
			return this->data_size() + sizeof(frame_video_header);
		}

		uint8_t* data() {
			return reinterpret_cast<uint8_t*>(this) + sizeof(frame_video_header);
		}
	};

	struct frame_audio_header {
		uint32_t channels, samples; // Number of channels, samples
		uint32_t samplerate; // Audio sample rate in Hz
		uint32_t stride; // Inter-channel stride, in bytes
		// Actual audio data follows, one float per sample, size = stride * channels

		uint64_t data_size() {
			return this->stride * this->channels;
		}

		uint64_t size() {
			return this->data_size() + sizeof(frame_audio_header);
		}

		float* data() {
			return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(this)
					+ sizeof(frame_audio_header));
		}
	};

	enum frame_kind {
		FRAME_KIND_NONE,
		FRAME_KIND_VIDEO,
		FRAME_KIND_AUDIO
	};

	struct frame_header {
		uint64_t length;
		uint64_t tag; // An opaque data field
		uint8_t kind;
		uint8_t version;
		uint16_t reserved;
		uint64_t payload_length;
		uint64_t timestamp;
		// Actual frame content follows in memory

		void zero() {
			this->tag = 0;
			this->kind = 0;
			this->version = 0;
			this->reserved = 0;
			this->payload_length = 0;
		}

		uint8_t* payload() {
			return reinterpret_cast<uint8_t*>(this) + sizeof(frame_header);
		}

		uint64_t max_payload_length() {
			return this->length - sizeof(frame_header);
		}

		frame_video_header* payload_video() {
			if(this->kind != FRAME_KIND_VIDEO) return NULL;
			return reinterpret_cast<frame_video_header*>(this->payload());
		}

		frame_audio_header* payload_audio() {
			if(this->kind != FRAME_KIND_AUDIO) return NULL;
			return reinterpret_cast<frame_audio_header*>(this->payload());
		}
	};

#define STAGE_NUM_SLOTS 128
	struct stage_queue {
		stage_queue() : msg_max(0), msg_cur(0) { }

		void push(frame_header* frame) {
			assert(frame != NULL);
			bi::scoped_lock<bi::interprocess_mutex> lock(this->mut);
			assert(!this->full());
			this->frames[this->offset(msg_max++)] = frame;
			this->cond.notify_one();
		}

		frame_header* pop() {
			bi::scoped_lock<bi::interprocess_mutex> lock(this->mut);
			this->cond.wait(lock, [&]{ return !this->empty(); });
			return this->frames[this->offset(msg_cur++)].get();
		}

		template<typename Rep, typename Period>
			frame_header* pop_timed(const std::chrono::duration<Rep, Period>& rel_time) {
				using namespace std;
				using namespace boost;
				bi::scoped_lock<bi::interprocess_mutex> lock(this->mut);
				auto timeout = get_system_time() + posix_time::microseconds(
						chrono::duration_cast<chrono::microseconds>(rel_time).count());
				if(!this->cond.timed_wait(lock, timeout, [&]{ return !this->empty(); })) {
					return NULL;
				}
				return this->frames[this->offset(msg_cur++)].get();
			}

		frame_header* try_pop() {
			bi::scoped_lock<bi::interprocess_mutex> lock(this->mut);
			if(this->empty()) return NULL;
			return this->frames[this->offset(msg_cur++)].get();
		}

		private:
		bi::interprocess_mutex mut;
		bi::interprocess_condition cond;
		uint64_t msg_max;
		uint64_t msg_cur;
		bi::offset_ptr<frame_header> frames[STAGE_NUM_SLOTS];

		uint64_t offset(uint64_t index) {
			return index % STAGE_NUM_SLOTS;
		}

		// The following helpers MUST be called under lock!
		bool empty() {
			return msg_max == msg_cur;
		}

		bool full() {
			return msg_cur == msg_max - STAGE_NUM_SLOTS;
		}
	};

	inline std::string random_name() {
		srand(time(nullptr));
		static const char chars[] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789";
		std::string tmp = "fl-";
		for(int i = 0; i < 8; i++) {
			tmp += chars[rand() % (sizeof(chars) - 1)];
		}
		return tmp;
	}

	struct segment_header {
		uint32_t version; // Version, must be set to 0.
		bi::offset_ptr<stage_queue> firststage; // Byte-index to the first stage descriptor
		uint32_t stages; // Number of stages
		bi::offset_ptr<frame_header> firstframe; // Byte-index to the first frame
		uint32_t framesize; // Size of each frame buffer
		uint32_t framecount; // Total number of frame buffers
	};

	enum frame_link_mode {
		FRAME_LINK_MODE_NULL, // Null-value for a link i.e. cannot do anything
		FRAME_LINK_MODE_MASTER, // We are the link master, i.e. frame provider
		FRAME_LINK_MODE_SLAVE // We are a link slave, i.e. not the frame provider
	};

	struct frame_link {

		typedef frame_header frame_t;
		typedef frame_kind kind;
		typedef frame_video_header frame_video;
		typedef frame_audio_header frame_audio;

		frame_link() : mode(FRAME_LINK_MODE_NULL), shm() { }

		frame_link(int depth, unsigned int maxframes = STAGE_NUM_SLOTS)
			: mode(FRAME_LINK_MODE_MASTER),
				name(random_name())
		{
			bi::shared_memory_object::remove(name.c_str());
			shm = bi::shared_memory_object(bi::create_only,
				name.c_str(), bi::read_write);
			shm.truncate(4L * 1024L * 1024L * 1024L);
			region = bi::mapped_region(shm, bi::read_write);
			this->shm_init();
			this->alloc_structures(depth, maxframes);
		}

		frame_link(const std::string& name_) : mode(FRAME_LINK_MODE_SLAVE),
				name(name_)
		{
			shm = bi::shared_memory_object(bi::open_only,
				name.c_str(), bi::read_write);
			region = bi::mapped_region(shm, bi::read_write);
			this->shm_init();
			this->join_link();
		}

		~frame_link() {
			if(mode == FRAME_LINK_MODE_MASTER) {
				bi::shared_memory_object::remove(name.c_str());
			}
		}

		frame_link& operator=(frame_link&& other) {
			mode = other.mode;
			base = other.base; end = other.end;
			header = other.header;
			stages = other.stages;
			loc = other.loc;
			name = other.name;
			shm = std::move(other.shm);
			region = std::move(other.region);
			other.mode = FRAME_LINK_MODE_NULL;
			return *this;
		}

		bool is_null() {
			return this->mode == FRAME_LINK_MODE_NULL;
		}

		// Retrives the name of the frame link. This name must be provided to
		// slaves, such that they can correctly join this frame link. The
		// application should treat this name as an opaque identifier.
		const std::string& get_name() {
			return this->name;
		}

		// Pushes a frame onto a stage buffer. Never blocks. Pushing to stage 0 will
		// reset the frame.
		void push(int stage, frame_header *frame) {
			this->get_stage(stage)->push(frame);
			if(stage == 0) frame->zero();
		}

		// Pops a frame from a specific stage buffer. Blocks until a frame is
		// available.
		frame_header *pop(int stage) {
			return this->get_stage(stage)->pop();
		}

		// Pops a frame from a specific stage buffer. Blocks until a frame is
		// available or the timeout has passed.
		template<typename Rep, typename Period>
			frame_header* pop_timed(int stage, const std::chrono::duration<Rep, Period>& rel_time) {
				return this->get_stage(stage)->pop_timed(rel_time);
			}

		// Tries to pop a frame from a specific stage buffer. Never blocks, but
		// returns NULL if no frame could be popped.
		frame_header *try_pop(int stage) {
			return this->get_stage(stage)->try_pop();
		}

		private:

		void shm_init() {
			this->base = this->loc = reinterpret_cast<char*>(this->region.get_address());
			this->end = this->base + this->region.get_size();
		}

		// Link allocation routines

		void alloc_header() {
			// Allocate and initialize header
			this->header = new (this->loc) segment_header;
			memset(this->header, 0, sizeof(segment_header));
			this->header->version = 0;
			this->loc += sizeof(segment_header);
		}

		// Allocate stages. There are N+1 stages, where N is pipeline depth.
		void alloc_stage_queues(int depth) {
			this->header->firststage = reinterpret_cast<stage_queue*>(this->loc);
			this->header->stages = depth+1;
			for(size_t i = 0; i < this->header->stages; i++) {
				new (this->loc) stage_queue;
				this->loc += sizeof(stage_queue);
			}
		}

		// The rest of the segment is filled with frame buffers.
		// Initially all frame buffers are inserted into the zeroth stage.
		void alloc_frame_buffers(unsigned int maxframes) {
			header->framesize = 32 * 1042 * 1024;
			header->firstframe = reinterpret_cast<frame_header*>(this->loc);
			header->framecount = 0;
			stage_queue* stage = get_stage(0);
			while(loc < end
					&& loc+header->framesize < end
					&& header->framecount < STAGE_NUM_SLOTS
					&& header->framecount < maxframes)
			{
				frame_header *frame = new (loc) frame_header;
				frame->length = header->framesize;
				frame->zero();
				stage->push(frame);
				loc += header->framesize;
				++header->framecount;
			}
		}

		void alloc_structures(int depth, unsigned int maxframes) {
			this->alloc_header();
			this->alloc_stage_queues(depth);
			this->cache_stages();
			this->alloc_frame_buffers(maxframes);
		}

		// Link joining routines

		void join_link() {
			this->header = reinterpret_cast<segment_header*>(this->base);
			this->cache_stages();
		}

		// Shared routines

		void cache_stages() {
			this->stages.reserve(this->header->stages);
			stage_queue* sq = this->header->firststage.get();
			for(uint32_t i = 0; i < this->header->stages; i++) {
				this->stages.push_back(sq++);
			}
		}

		stage_queue* get_stage(int stage) {
			return this->stages.at(stage);
		}

		// Generic data
		frame_link_mode mode;
		char *base, *end;
		segment_header *header;
		std::vector<stage_queue*> stages;

		// Master specific
		char *loc; // Next location to put newly allocated structures

		// Slave specific

		// Shared-memory management
		std::string name;
		bi::shared_memory_object shm;
		bi::mapped_region region;
	};

};

typedef frame_link_detail::frame_link frame_link;
