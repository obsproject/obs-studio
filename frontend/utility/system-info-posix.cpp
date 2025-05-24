#include "system-info.hpp"
#include "util/dstr.hpp"
#include "util/platform.h"
#include <sys/utsname.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <regex>

using namespace std;

extern "C" {
#include "pci/pci.h"
}

// Anonymous namespace ensures internal linkage
namespace {
struct drm_card_info {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor;
	uint16_t subsystem_device;

	/* match_count is the number of matches between
	 * the tokenized GPU identification string and
	 * the device_name and vendor_name supplied by
	 * libpci. It is used to associate the GPU that
	 * OBS is using and the cards found in a bus scan
	 * using libpci.
	 */
	uint16_t match_count;

	/* The following 2 fields are easily
	 * obtained for AMD GPUs. Reporting
	 * for NVIDIA GPUs is not working at
	 * the moment.
	 */
	uint64_t dedicated_vram_total;
	uint64_t shared_system_memory_total;

	// PCI domain:bus:slot:function
	std::string dbsf;
	std::string device_name;
	std::string vendor_name;
};

constexpr std::string_view WHITE_SPACE = " \f\n\r\t\v";

// trim_ws() will remove leading and trailing
// white space from the string.
void trim_ws(std::string &s)
{
	// Trim leading whitespace
	size_t pos = s.find_first_not_of(WHITE_SPACE);
	if (pos != std::string::npos)
		s = s.substr(pos);

	// Trim trailing whitespace
	pos = s.find_last_not_of(WHITE_SPACE);
	if (pos != std::string::npos)
		s = s.substr(0, pos + 1);
}

bool compare_match_strength(const drm_card_info &a, const drm_card_info &b)
{
	return a.match_count > b.match_count;
}

void adjust_gpu_model(std::string &model)
{
	/* Use the sub-string between the [] brackets. For example,
	 * the NVIDIA Quadro P4000 model string from PCI ID database
	 * is "GP104GL [Quadro P4000]", and we only want the "Quadro
	 * P4000" sub-string.
	 */
	size_t first = model.find('[');
	size_t last = model.find_last_of(']');
	if ((last - first) > 1) {
		std::string adjusted_model = model.substr(first + 1, last - first - 1);
		model.assign(adjusted_model);
	}
}

bool get_distribution_info(std::string &distro, std::string &version)
{
	ifstream file;
	std::string line;
	const std::string flatpak_file = "/.flatpak-info";
	const std::string systemd_file = "/etc/os-release";

	distro = "";
	version = "";

	if (std::filesystem::exists(flatpak_file)) {
		/* The .flatpak-info file has a line of the form:
		 *
		 * runtime=runtime/org.kde.Platform/x86_64/6.6
		 *
		 * Parse the line into tokens to identify the name and
		 * version, which are "org.kde.Platform" and "6.6" respectively,
		 * in the example above.
		 */
		file.open(flatpak_file);
		if (file.is_open()) {
			while (getline(file, line)) {
				if (line.compare(0, 16, "runtime=runtime/") == 0) {
					size_t pos = line.find('/');
					if (pos != string::npos && line.at(pos + 1) != '\0') {
						line.erase(0, pos + 1);

						/* Split the string into tokens with a regex
						 * of one or more '/' characters'.
						 */
						std::regex fp_reg("[/]+");
						vector<std::string> fp_tokens(
							sregex_token_iterator(line.begin(), line.end(), fp_reg, -1),
							sregex_token_iterator());
						if (fp_tokens.size() >= 2) {
							auto token = begin(fp_tokens);
							distro = "Flatpak " + *token;
							token = next(fp_tokens.end(), -1);
							version = *token;
						} else {
							distro = "Flatpak unknown";
							version = "0";
							blog(LOG_DEBUG,
							     "%s: Format of 'runtime' entry unrecognized in file %s",
							     __FUNCTION__, flatpak_file.c_str());
						}
						break;
					}
				}
			}
			file.close();
		}
	} else if (std::filesystem::exists(systemd_file)) {
		/* systemd-based distributions use /etc/os-release to identify
		 * the OS. For example, the Ubuntu 24.04.1 variant looks like:
		 *
		 * PRETTY_NAME="Ubuntu 24.04.1 LTS"
		 * NAME="Ubuntu"
		 * VERSION_ID="24.04"
		 * VERSION="24.04.1 LTS (Noble Numbat)"
		 * VERSION_CODENAME=noble
		 * ID=ubuntu
		 * ID_LIKE=debian
		 * HOME_URL="https://www.ubuntu.com/"
		 * SUPPORT_URL="https://help.ubuntu.com/"
		 * BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
		 * PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
		 * UBUNTU_CODENAME=noble
		 * LOGO=ubuntu-logo
		 *
		 * Parse the file looking for the NAME and VERSION_ID fields.
		 * Note that some distributions wrap the entry in '"' characters
		 * while others do not, so we need to remove those characters.
		 */
		file.open(systemd_file);
		if (file.is_open()) {
			while (getline(file, line)) {
				if (line.compare(0, 4, "NAME") == 0) {
					size_t pos = line.find('=');
					if (pos != std::string::npos && line.at(pos + 1) != '\0') {
						distro = line.substr(pos + 1);

						// Remove the '"' characters from the string, if any.
						distro.erase(std::remove(distro.begin(), distro.end(), '"'),
							     distro.end());

						trim_ws(distro);
						continue;
					}
				}

				if (line.compare(0, 10, "VERSION_ID") == 0) {
					size_t pos = line.find('=');
					if (pos != std::string::npos && line.at(pos + 1) != '\0') {
						version = line.substr(pos + 1);

						// Remove the '"' characters from the string, if any.
						version.erase(std::remove(version.begin(), version.end(), '"'),
							      version.end());

						trim_ws(version);
						continue;
					}
				}
			}
			file.close();
		}
	} else {
		blog(LOG_DEBUG, "%s: Failed to find host OS or flatpak info", __FUNCTION__);
		return false;
	}

	return true;
}

bool get_cpu_name(optional<std::string> &proc_name)
{
	ifstream file("/proc/cpuinfo");
	std::string line;
	int physical_id = -1;
	bool found_name = false;

	/* Initialize processor name. Some ARM-based hosts do
	 * not output the "model name" field in /proc/cpuinfo.
	 */
	proc_name = "Unavailable";

	if (file.is_open()) {
		while (getline(file, line)) {
			if (line.compare(0, 10, "model name") == 0) {
				size_t pos = line.find(':');
				if (pos != std::string::npos && line.at(pos + 1) != '\0') {
					proc_name = line.substr(pos + 1);
					trim_ws((std::string &)proc_name);
					found_name = true;
					continue;
				}
			}

			if (line.compare(0, 11, "physical id") == 0) {
				size_t pos = line.find(':');
				if (pos != std::string::npos && line.at(pos + 1) != '\0') {
					physical_id = atoi(&line[pos + 1]);
					if (physical_id == 0 && found_name)
						break;
				}
			}
		}
		file.close();
	}
	return true;
}

bool get_cpu_freq(uint32_t &cpu_freq)
{
	ifstream freq_file;
	std::string line;

	/* Look for the sysfs tree "base_frequency" first.
	 * Intel exports "base_frequency, AMD does not.
	 * If not found, use "cpuinfo_max_freq".
	 */
	freq_file.open("/sys/devices/system/cpu/cpu0/cpufreq/base_frequency");
	if (!freq_file.is_open()) {
		freq_file.open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if (!freq_file.is_open())
			return false;
	}

	if (getline(freq_file, line)) {
		trim_ws(line);
		// Convert the CPU frequency string to an integer in MHz
		cpu_freq = atoi(line.c_str()) / 1000;
	} else {
		cpu_freq = 0;
	}
	freq_file.close();

	return true;
}

/* get_drm_cards() will find all render-capable cards
 * in /sys/class/drm and populate a vector of drm_card_info.
 */
void get_drm_cards(std::vector<drm_card_info> &cards)
{
	struct drm_card_info dci;
	char *file_str = NULL;
	char buf[256];
	int len = 0;
	uint val = 0;

	std::string base_path = "/sys/class/drm/";
	std::string drm_path = base_path;
	size_t base_len = base_path.length();
	size_t node_len = 0;

	os_dir_t *dir = os_opendir(base_path.c_str());
	struct os_dirent *ent;

	while ((ent = os_readdir(dir)) != NULL) {
		std::string entry_name = ent->d_name;
		if (ent->directory && (entry_name.find("renderD") != std::string::npos)) {
			dci = {0};
			drm_path.resize(base_len);
			drm_path += ent->d_name;
			drm_path += "/device";

			/* Get the PCI D:B:S:F (domain:bus:slot:function) in string form
			 * by reading the device symlink.
			 */
			if ((len = readlink(drm_path.c_str(), buf, sizeof(buf) - 1)) != -1) {
				// readlink() doesn't null terminate strings, so do it explicitly
				buf[len] = '\0';
				dci.dbsf = buf;

				/* The DBSF string is of the form: "../../../0000:65:00.0/".
				* Remove all the '/' characters, and
				* remove all the leading '.' characters.
				*/
				dci.dbsf.erase(std::remove(dci.dbsf.begin(), dci.dbsf.end(), '/'), dci.dbsf.end());
				dci.dbsf.erase(0, dci.dbsf.find_first_not_of("."));

				node_len = drm_path.length();

				// Get the device_id
				drm_path += "/device";
				file_str = os_quick_read_utf8_file(drm_path.c_str());
				if (!file_str) {
					blog(LOG_ERROR, "Could not read from '%s'", drm_path.c_str());
					dci.device_id = 0;
				} else {
					// Skip over the "0x" and convert
					sscanf(file_str + 2, "%x", &val);
					dci.device_id = val;
					bfree(file_str);
				}

				// Get the vendor_id
				drm_path.resize(node_len);
				drm_path += "/vendor";
				file_str = os_quick_read_utf8_file(drm_path.c_str());
				if (!file_str) {
					blog(LOG_ERROR, "Could not read from '%s'", drm_path.c_str());
					dci.vendor_id = 0;
				} else {
					// Skip over the "0x" and convert
					sscanf(file_str + 2, "%x", &val);
					dci.vendor_id = val;
					bfree(file_str);
				}

				// Get the subsystem_vendor
				drm_path.resize(node_len);
				drm_path += "/subsystem_vendor";
				file_str = os_quick_read_utf8_file(drm_path.c_str());
				if (!file_str) {
					dci.subsystem_vendor = 0;
				} else {
					// Skip over the "0x" and convert
					sscanf(file_str + 2, "%x", &val);
					dci.subsystem_vendor = val;
					bfree(file_str);
				}

				// Get the subsystem_device
				drm_path.resize(node_len);
				drm_path += "/subsystem_device";
				file_str = os_quick_read_utf8_file(drm_path.c_str());
				if (!file_str) {
					dci.subsystem_device = 0;
				} else {
					// Skip over the "0x" and convert
					sscanf(file_str + 2, "%x", &val);
					dci.subsystem_device = val;
					bfree(file_str);
				}

				/* The amdgpu driver exports the GPU memory information
				 * via sysfs nodes. Sadly NVIDIA doesn't have the same
				 * information via sysfs. Read the amdgpu-based nodes
				 * if present and get the required fields.
				 *
				 * Get the GPU total dedicated VRAM, if available
				 */
				drm_path.resize(node_len);
				drm_path += "/mem_info_vram_total";
				file_str = os_quick_read_utf8_file(drm_path.c_str());
				if (!file_str) {
					dci.dedicated_vram_total = 0;
				} else {
					sscanf(file_str, "%lu", &dci.dedicated_vram_total);
					bfree(file_str);
				}

				// Get the GPU total shared system memory, if available
				drm_path.resize(node_len);
				drm_path += "/mem_info_gtt_total";
				file_str = os_quick_read_utf8_file(drm_path.c_str());
				if (!file_str) {
					dci.shared_system_memory_total = 0;
				} else {
					sscanf(file_str, "%lu", &dci.shared_system_memory_total);
					bfree(file_str);
				}

				cards.push_back(dci);
				blog(LOG_DEBUG,
				     "%s: drm_adapter_info: PCI B:D:S:F: %s, device_id:0x%x,"
				     "vendor_id:0x%x, sub_device:0x%x, sub_vendor:0x%x,"
				     "vram_total: %lu, sys_memory: %lu",
				     __FUNCTION__, dci.dbsf.c_str(), dci.device_id, dci.vendor_id, dci.subsystem_device,
				     dci.subsystem_vendor, dci.dedicated_vram_total, dci.shared_system_memory_total);
			} else {
				blog(LOG_DEBUG, "%s: Failed to read symlink for %s", __FUNCTION__, drm_path.c_str());
			}
		}
	}
	os_closedir(dir);
}

/* system_gpu_data() returns a sorted vector of GoLiveApi::Gpu
 * objects needed to build the multitrack video request.
 *
 * When a single GPU is installed the case is trivial. In systems
 * with multiple GPUs (including the CPU iGPU), the GPU that
 * OBS is currently using must be placed first in the list, so
 * that composition_gpu_index=0 is valid. composition_gpu_index
 * is set to to ovi.adapter which is always 0 apparently.
 *
 * system_gpu_data() does the following:
 *   1. Gather information about the GPU being used by OBS
 *   2. Scan the PCIe bus to identify all GPUs
 *   3. Find a best match of the GPU in-use in the list found in 2
 *   4. Generate the sorted list of GoLiveAPI::Gpu entries
 *
 * Note that the PCIe device_id and vendor_id are not available
 * via OpenGL calls, hence the need to match the in-use GPU with
 * the libpci scanned results and extract the PCIe information.
 */
std::optional<std::vector<GoLiveApi::Gpu>> system_gpu_data()
{
	std::vector<GoLiveApi::Gpu> adapter_info;
	GoLiveApi::Gpu gpu;
	std::string gs_driver_version;
	std::string gs_device_renderer;
	uint64_t dedicated_video_memory = 0;
	uint64_t shared_system_memory = 0;

	std::vector<drm_card_info> drm_cards;
	struct pci_access *pacc;
	pacc = pci_alloc();
	pci_init(pacc);
	char slot[256];

	// Obtain GPU information by querying graphics API
	obs_enter_graphics();
	gs_driver_version = gs_get_driver_version();
	gs_device_renderer = gs_get_renderer();
	dedicated_video_memory = gs_get_gpu_dmem() * 1024;
	shared_system_memory = gs_get_gpu_smem() * 1024;
	obs_leave_graphics();

	/* Split the GPU renderer string into tokens with
	 * a regex of one or more white-space characters.
	 */
	std::regex ws_reg("[\\s]+");
	vector<std::string> gpu_tokens(sregex_token_iterator(gs_device_renderer.begin(), gs_device_renderer.end(),
							     ws_reg, -1),
				       sregex_token_iterator());

	// Remove extraneous characters from the tokens
	constexpr std::string_view EXTRA_CHARS = ",()[]{}";
	for (auto token = begin(gpu_tokens); token != end(gpu_tokens); ++token) {
		for (unsigned int i = 0; i < EXTRA_CHARS.size(); ++i) {
			token->erase(std::remove(token->begin(), token->end(), EXTRA_CHARS[i]), token->end());
		}
		// Convert tokens to lower-case
		std::transform(token->begin(), token->end(), token->begin(), ::tolower);
		blog(LOG_DEBUG, "%s: gpu_token: '%s'", __FUNCTION__, token->c_str());
	}

	// Scan the PCI bus once
	pci_scan_bus(pacc);

	// Discover the set of DRM render-capable GPUs
	get_drm_cards(drm_cards);
	blog(LOG_DEBUG, "Number of GPUs detected: %lu", drm_cards.size());

	// Iterate through drm_cards to get the device and vendor names via libpci
	for (auto card = begin(drm_cards); card != end(drm_cards); ++card) {
		struct pci_dev *pdev;
		struct pci_filter pfilter;
		char namebuf[1024];

		/* Get around the 'const char*' vs 'char*'
		 * type issue with pci_filter_parse_slot().
		 */
		strncpy(slot, card->dbsf.c_str(), sizeof(slot));

		// Validate the "slot" string according to libpci
		pci_filter_init(pacc, &pfilter);
		if (pci_filter_parse_slot(&pfilter, slot)) {
			blog(LOG_DEBUG, "%s: pci_filter_parse_slot() failed", __FUNCTION__);
			continue;
		}

		// Get the device name and vendor name from libpci
		for (pdev = pacc->devices; pdev; pdev = pdev->next) {
			if (pci_filter_match(&pfilter, pdev)) {
				pci_fill_info(pdev, PCI_FILL_IDENT);
				card->device_name =
					pci_lookup_name(pacc, namebuf, sizeof(namebuf),
							PCI_LOOKUP_DEVICE | PCI_LOOKUP_CACHE | PCI_LOOKUP_NETWORK,
							pdev->vendor_id, pdev->device_id);
				card->vendor_name =
					pci_lookup_name(pacc, namebuf, sizeof(namebuf),
							PCI_LOOKUP_VENDOR | PCI_LOOKUP_CACHE | PCI_LOOKUP_NETWORK,
							pdev->vendor_id, pdev->device_id);
				blog(LOG_DEBUG, "libpci lookup: device_name: %s, vendor_name: %s",
				     card->device_name.c_str(), card->vendor_name.c_str());
				break;
			}
		}
	}
	pci_cleanup(pacc);

	/* Iterate through drm_cards to determine a string match count
	 * against the GPU string tokens from the OpenGL identification.
	 */
	for (auto card = begin(drm_cards); card != end(drm_cards); ++card) {
		card->match_count = 0;
		for (auto token = begin(gpu_tokens); token != end(gpu_tokens); ++token) {
			std::string card_device_name = card->device_name;
			std::string card_gpu_vendor = card->vendor_name;
			std::transform(card_device_name.begin(), card_device_name.end(), card_device_name.begin(),
				       ::tolower);
			std::transform(card_gpu_vendor.begin(), card_gpu_vendor.end(), card_gpu_vendor.begin(),
				       ::tolower);

			// Compare GPU string tokens to PCI device name
			std::size_t found = card_device_name.find(*token);
			if (found != std::string::npos) {
				card->match_count++;
				blog(LOG_DEBUG, "Found %s in PCI device name", (*token).c_str());
			}
			// Compare GPU string tokens to PCI vendor name
			found = card_gpu_vendor.find(*token);
			if (found != std::string::npos) {
				card->match_count++;
				blog(LOG_DEBUG, "Found %s in PCI vendor name", (*token).c_str());
			}
		}
	}

	/* Sort the cards based on the highest match strength.
	 * In the case of multiple cards and the first one is not a higher
	 * match, there is ambiguity and all we can do is log a warning.
	 * The chance of this happening is low, but not impossible.
	 */
	std::sort(drm_cards.begin(), drm_cards.end(), compare_match_strength);
	if ((drm_cards.size() > 1) && (std::next(begin(drm_cards))->match_count) >= begin(drm_cards)->match_count) {
		blog(LOG_WARNING, "%s: Ambiguous GPU association. Possible incorrect sort order.", __FUNCTION__);
		for (auto card = begin(drm_cards); card != end(drm_cards); ++card) {
			blog(LOG_DEBUG, "Total matches for card %s: %u", card->device_name.c_str(), card->match_count);
		}
	}

	/* Iterate through the sorted list of cards and generate
	 * the GoLiveApi GPU list.
	 */
	for (auto card = begin(drm_cards); card != end(drm_cards); ++card) {
		gpu.device_id = card->device_id;
		gpu.vendor_id = card->vendor_id;
		gpu.model = card->device_name;
		adjust_gpu_model(gpu.model);

		if (card == begin(drm_cards)) {
			/* The first card in the list corresponds to the
			 * driver version and GPU memory information obtained
			 * previously by OpenGL calls into the GPU.
			 */
			gpu.driver_version = gs_driver_version;
			gpu.dedicated_video_memory = dedicated_video_memory;
			gpu.shared_system_memory = shared_system_memory;
		} else {
			/* The driver version for the other device(s)
			 * is not accessible easily in a common location.
			 */
			gpu.driver_version = "Unknown";

			/* Use the GPU memory info discovered with get_drm_card_info()
			 * stored in drm_cards. amdgpu driver exposes the GPU memory
			 * info via sysfs, NVIDIA does not.
			 */
			gpu.dedicated_video_memory = card->dedicated_vram_total;
			gpu.shared_system_memory = card->shared_system_memory_total;
		}
		adapter_info.push_back(gpu);
	}

	return adapter_info;
}
} // namespace

void system_info(GoLiveApi::Capabilities &capabilities)
{
	// Determine the GPU capabilities
	capabilities.gpu = system_gpu_data();

	// Determine the CPU capabilities
	{
		auto &cpu_data = capabilities.cpu;
		cpu_data.physical_cores = os_get_physical_cores();
		cpu_data.logical_cores = os_get_logical_cores();

		if (!get_cpu_name(cpu_data.name)) {
			cpu_data.name = "Unknown";
		}

		uint32_t cpu_freq;
		if (get_cpu_freq(cpu_freq)) {
			cpu_data.speed = cpu_freq;
		} else {
			cpu_data.speed = 0;
		}
	}

	// Determine the memory capabilities
	{
		auto &memory_data = capabilities.memory;
		memory_data.total = os_get_sys_total_size();
		memory_data.free = os_get_sys_free_size();
	}

	// Reporting of gaming features not supported on Linux
	UNUSED_PARAMETER(capabilities.gaming_features);

	// Determine the system capabilities
	{
		auto &system_data = capabilities.system;

		if (!get_distribution_info(system_data.name, system_data.release)) {
			system_data.name = "Linux-based distribution";
			system_data.release = "unknown";
		}

		struct utsname utsinfo;
		static const uint16_t max_sys_data_version_sz = 128;
		if (uname(&utsinfo) == 0) {
			/* To determine if the host is 64-bit, check if the machine
			 * name contains "64", as in "x86_64" or "aarch64".
			 */
			system_data.bits = strstr(utsinfo.machine, "64") ? 64 : 32;

			/* To determine if the host CPU is ARM based, check if the
			 * machine name contains "aarch".
			 */
			system_data.arm = strstr(utsinfo.machine, "aarch") ? true : false;

			/* Send the sysname (usually "Linux"), kernel version and
			 * release reported by utsname as the version string.
			 * The code below will produce something like:
			 *
			 * "Linux 6.5.0-41-generic #41~22.04.2-Ubuntu SMP PREEMPT_DYNAMIC
			 *  Mon Jun  3 11:32:55 UTC 2"
			 */
			system_data.version = utsinfo.sysname;
			system_data.version.append(" ");
			system_data.version.append(utsinfo.release);
			system_data.version.append(" ");
			system_data.version.append(utsinfo.version);
			// Ensure system_data.version string is within the maximum size
			if (system_data.version.size() > max_sys_data_version_sz) {
				system_data.version.resize(max_sys_data_version_sz);
			}
		} else {
			UNUSED_PARAMETER(system_data.bits);
			UNUSED_PARAMETER(system_data.arm);
			system_data.version = "unknown";
		}

		// On Linux-based distros, there's no build or revision info
		UNUSED_PARAMETER(system_data.build);
		UNUSED_PARAMETER(system_data.revision);

		system_data.armEmulation = os_get_emulation_status();
	}
}
