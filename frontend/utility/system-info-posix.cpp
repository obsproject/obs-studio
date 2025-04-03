#include "system-info.hpp"
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

GoLiveApi::Gpu get_card_info(struct pci_access *pacc, std::string_view name)
{
	uint32_t domain, bus, device, function;
	int len;
	char buf[256];
	std::string dbdf;
	char *file_str;
	std::string base_path = "/sys/class/drm/";
	std::string drm_path = base_path;

	GoLiveApi::Gpu gpu;
	drm_path += name;
	drm_path += "/device";
	size_t node_len = drm_path.length();

	/* Get the PCI domain,bus,device,function by reading the device symlink.
	 */
	if ((len = readlink(drm_path.c_str(), buf, sizeof(buf) - 1)) != -1) {
		// readlink() doesn't null terminate strings, so do it explicitly
		buf[len] = '\0';
		dbdf = buf;

		/* The DBDF string is of the form: "../../../0000:65:00.0/".
		* Remove all the '/' characters, and
		* remove all the leading '.' characters.
		*/
		dbdf.erase(std::remove(dbdf.begin(), dbdf.end(), '/'), dbdf.end());
		dbdf.erase(0, dbdf.find_first_not_of("."));

		// Extract the integer values for libpci.
		std::string sdomain = dbdf.substr(0, 4);
		std::string sbus = dbdf.substr(5, 2);
		std::string sdevice = dbdf.substr(8, 2);
		std::string sfunction = dbdf.substr(11, 2); // Let cpp clamp it.
		sscanf(sdomain.c_str(), "%x", &domain);
		sscanf(sbus.c_str(), "%x", &bus);
		sscanf(sdevice.c_str(), "%x", &device);
		sscanf(sfunction.c_str(), "%x", &function);

		struct pci_dev *dev = pci_get_dev(pacc, domain, bus, device, function);
		pci_fill_info(dev, PCI_FILL_IDENT);

		gpu.vendor_id = dev->vendor_id;
		gpu.device_id = dev->device_id;
		gpu.model = pci_lookup_name(pacc, buf, sizeof(buf),
					    PCI_LOOKUP_DEVICE | PCI_LOOKUP_CACHE | PCI_LOOKUP_NETWORK, dev->vendor_id,
					    dev->device_id);
		adjust_gpu_model(gpu.model);

		// Get the GPU total vram memory, if available
		drm_path.resize(node_len);
		drm_path += "/mem_info_vram_total";
		file_str = os_quick_read_utf8_file(drm_path.c_str());
		if (!file_str) {
			gpu.dedicated_video_memory = 0;
		} else {
			sscanf(file_str, "%lu", &gpu.dedicated_video_memory);
			bfree(file_str);
		}

		// Get the GPU total shared system memory, if available
		drm_path.resize(node_len);
		drm_path += "/mem_info_gtt_total";
		file_str = os_quick_read_utf8_file(drm_path.c_str());
		if (!file_str) {
			gpu.shared_system_memory = 0;
		} else {
			sscanf(file_str, "%lu", &gpu.shared_system_memory);
			bfree(file_str);
		}
	}
	return gpu;
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
 *   1. Gather information about the GPUs available to OBS
 */
std::optional<std::vector<GoLiveApi::Gpu>> system_gpu_data()
{
	std::vector<GoLiveApi::Gpu> adapter_info;
	std::string renderer_name;
	struct pci_access *pacc = pci_alloc();
	pci_init(pacc);

	// Obtain GPU information by querying graphics API
	obs_enter_graphics();
	auto lambda = [&](const char *name, uint32_t id) -> bool {
		if (id != 0 && name == renderer_name) {
			// The render device will show up twice, see gs_enum_adapters for info why.
			return true;
		}
		std::string_view vname = name;
		std::string node = &name[vname.rfind("/")];
		GoLiveApi::Gpu gpu = get_card_info(pacc, node);

		if (id == 0) {
			// The render device, so we can read from OpenGL.
			gpu.driver_version = gs_get_driver_version();
			gpu.dedicated_video_memory = gs_get_gpu_dmem() * 1024;
			gpu.shared_system_memory = gs_get_gpu_smem() * 1024;
			renderer_name = name;
		} else {
			/* The driver version for the other device(s)
			 * is not accessible easily in a common location.
			 */
			gpu.driver_version = "Unknown";
		}
		adapter_info.push_back(gpu);
		return true;
	};
	gs_enum_adapters([](void *p, const char *name,
			    uint32_t id) -> bool { return (*(decltype(lambda) *)p)(name, id); },
			 &lambda);
	obs_leave_graphics();
	pci_cleanup(pacc);

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
